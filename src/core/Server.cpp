#include "Server.hpp"
#include "CommandContext.hpp"
#include "Message.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

volatile sig_atomic_t Server::_shouldStop = 0;

Server::Server(int port, const std::string &password)
    : _port(port), _password(password), _serverFd(-1), _responseSink(),
      _serverCtx(_clients, _channels, _responseSink, _password),
      _dispatcher(_serverCtx) {}

Server::~Server() {
  for (std::map<int, Client *>::iterator it = _clients.begin();
       it != _clients.end(); ++it) {
    delete it->second;
  }

  for (std::map<std::string, Channel *>::iterator it = _channels.begin();
       it != _channels.end(); ++it) {
    delete it->second;
  }

  if (_serverFd != -1) {
    close(_serverFd);
    std::cout << "Server socket closed." << std::endl;
  }
}

void Server::_handleSignal(int signo) {
  (void)signo;
  _shouldStop = 1;
}

void Server::setupSignalHandlers() {
  struct sigaction sa;
  if (sigemptyset(&sa.sa_mask) == -1) {
    throw std::runtime_error("Error: sigemptyset() failed.");
  }
  sa.sa_handler = Server::_handleSignal;
  sa.sa_flags = 0;
  if (sigaction(SIGINT, &sa, NULL) == -1 ||
      sigaction(SIGTERM, &sa, NULL) == -1) {
    throw std::runtime_error("Error: sigaction() failed.");
  }

  struct sigaction sa_pipe;
  if (sigemptyset(&sa_pipe.sa_mask) == -1) {
    throw std::runtime_error("Error: sigemptyset() failed.");
  }
  sa_pipe.sa_handler = SIG_IGN;
  sa_pipe.sa_flags = 0;
  if (sigaction(SIGPIPE, &sa_pipe, NULL) == -1) {
    throw std::runtime_error("Error: sigaction() failed.");
  }
}

void Server::_setNonBlocking(int fd) {
  if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
    throw std::runtime_error("Error: failed to setup non-blocking mode.");
  }
}

void Server::_setupServerSocket() {
  _serverFd = socket(AF_INET, SOCK_STREAM, 0);
  if (_serverFd == -1) {
    throw std::runtime_error("Error: failed to create server socket.");
  }

  int opt = 1;
  if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ==
      -1) {
    throw std::runtime_error("Error: setsockopt() failed.");
  }

  _setNonBlocking(_serverFd);

  struct sockaddr_in serverAddr;
  std::memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(_port);

  if (bind(_serverFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) ==
      -1) {
    throw std::runtime_error("Error: failed to bind port.");
  }

  if (listen(_serverFd, SOMAXCONN) == -1) {
    throw std::runtime_error("Error: listen() failed.");
  }

  struct pollfd serverPollFd;
  serverPollFd.fd = _serverFd;
  serverPollFd.events = POLLIN;
  serverPollFd.revents = 0;
  _pollFds.push_back(serverPollFd);
}

bool Server::_waitForEvents() {
  if (poll(&_pollFds[0], _pollFds.size(), -1) == -1) {
    if (errno == EINTR)
      return false;
    throw std::runtime_error("Error: poll() failed.");
  }
  return true;
}

void Server::_processActiveConnections() {
  for (size_t i = 0; i < _pollFds.size(); ++i) {
    if (_pollFds[i].revents == 0)
      continue;

    if (_pollFds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
      if (_pollFds[i].fd == _serverFd) {
        throw std::runtime_error("Fatal error on server socket.");
      } else {
        std::cout << "[!] Socket error or hangup detected. Fd: "
                  << _pollFds[i].fd << std::endl;
        std::string quitMsg = ":" + _clients[_pollFds[i].fd]->getPrefix() +
                              " QUIT :Connection closed (Error/Hangup)";
        _serverCtx.removeClientFromAllChannels(*_clients[_pollFds[i].fd],
                                               quitMsg);

        delete _clients[_pollFds[i].fd];
        _clients.erase(_pollFds[i].fd);
        _pollFds.erase(_pollFds.begin() + i);
        --i;
        continue;
      }
    }

    if (_pollFds[i].revents & POLLIN) {
      if (_pollFds[i].fd == _serverFd) {
        _acceptNewConnection();
      } else {
        if (_handleClientMessage(_pollFds[i]) == DISCONNECT) {
          std::string quitMsg = ":" + _clients[_pollFds[i].fd]->getPrefix() +
                                " QUIT :Client disconnected";
          _serverCtx.removeClientFromAllChannels(*_clients[_pollFds[i].fd],
                                                 quitMsg);

          delete _clients[_pollFds[i].fd];
          _clients.erase(_pollFds[i].fd);
          _pollFds.erase(_pollFds.begin() + i);
          --i;
        };
      }
    }

    if (_pollFds[i].revents & POLLOUT) {
      if (_pollFds[i].fd != _serverFd) {
        Client *client = _clients[_pollFds[i].fd];
        std::string &sendBuf = client->getSendBuffer();

        if (!sendBuf.empty()) {
          int bytesSent =
              send(_pollFds[i].fd, sendBuf.c_str(), sendBuf.length(), 0);
          if (bytesSent > 0) {
            client->eraseSendBuffer(bytesSent);
          } else if (bytesSent == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
              std::cerr << "Error: send() failed on Fd " << _pollFds[i].fd
                        << std::endl;
            }
          }
        }

        if (client->getSendBuffer().empty()) {
          _pollFds[i].events &= ~POLLOUT;
        }
      }
    }
  }
}

void Server::_acceptNewConnection() {
  struct sockaddr_in clientAddr;
  socklen_t clientLen = sizeof(clientAddr);

  int clientFd = accept(_serverFd, (struct sockaddr *)&clientAddr, &clientLen);
  if (clientFd >= 0) {
    char str[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &clientAddr.sin_addr, str, sizeof(str)) == NULL) {
      close(clientFd);
      std::cerr << "Error: inet_ntop() failed" << std::endl;
      return;
    }

    _setNonBlocking(clientFd);

    struct pollfd clientPollFd;
    clientPollFd.fd = clientFd;
    clientPollFd.events = POLLIN;
    clientPollFd.revents = 0;
    _pollFds.push_back(clientPollFd);

    _clients.insert(
        std::make_pair(clientFd, new Client(clientFd, std::string(str))));

    std::cout << "[+] A new client has connected. Fd: " << clientFd
              << std::endl;
  }
}

Server::ConnectionStatus
Server::_handleClientMessage(struct pollfd &clientPollFd) {
  char buffer[1024];

  int bytesRead = recv(clientPollFd.fd, buffer, sizeof(buffer), 0);

  if (bytesRead > 0) {

    _clients[clientPollFd.fd]->appendRecvBuffer(std::string(buffer, bytesRead));

    std::string rawMsg;

    while ((rawMsg = _clients[clientPollFd.fd]->extractMessage()) != "") {
      Message msg(rawMsg);
      if (!_executeCommand(_clients[clientPollFd.fd], msg)) {
        return DISCONNECT;
      }
    }
    return KEEP_ALIVE;
  } else if (bytesRead == 0) {
    std::cout << "[+] A client has disconnected. Fd: " << clientPollFd.fd
              << std::endl;
    return DISCONNECT;
  } else {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
      return KEEP_ALIVE;
    }
    std::cout << "Error: recv failed(). (errno = " << errno << ")" << std::endl;
    std::cout << "[+] A client has disconnected. Fd: " << clientPollFd.fd
              << std::endl;
    return DISCONNECT;
  }
}

void Server::_updatePollEvents() {
  for (size_t i = 0; i < _pollFds.size(); ++i) {
    int fd = _pollFds[i].fd;
    if (fd == _serverFd)
      continue;
    if (_clients.find(fd) != _clients.end()) {
      if (!_clients[fd]->getSendBuffer().empty())
        _pollFds[i].events |= POLLOUT;
      else
        _pollFds[i].events &= ~POLLOUT;
    }
  }
}

bool Server::_executeCommand(Client *client, const Message &msg) {
  CommandContext ctx(*client, msg, _serverCtx);
  return _dispatcher.dispatch(ctx);
}

void Server::start() {
  _setupServerSocket();

  std::cout << "Server is running on port " << _port << "..." << std::endl;

  while (!_shouldStop) {
    _updatePollEvents();
    if (_waitForEvents() == false)
      continue;
    _processActiveConnections();
  }

  std::cout << "Server shutdown..." << std::endl;
}
