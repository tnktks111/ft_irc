#include "Server.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include "CommandContext.hpp"
#include "Message.hpp"

volatile sig_atomic_t Server::_shouldStop = 0;

// Constructor & Destructor
Server::Server(int port, const std::string& password)
    : _port(port),
      _password(password),
      _serverFd(-1),
      _responseSink(),
      _serverCtx(_clients, _channels, _responseSink, _password),
      _dispatcher(_serverCtx) {}

Server::~Server() {
  for (std::map<int, Client*>::iterator it = _clients.begin();
       it != _clients.end(); ++it) {
    delete it->second;
  }

  for (std::map<std::string, Channel*>::iterator it = _channels.begin();
       it != _channels.end(); ++it) {
    delete it->second;
  }

  if (_serverFd != -1) {
    close(_serverFd);
    std::cout << "Server socket closed." << std::endl;
  }
}

// Errno Message Builder
std::string Server::_buildErrnoMessage(const std::string& context) {
  std::ostringstream oss;
  oss << "Error: " << context << " (errno = " << errno << ")";
  return oss.str();
}

std::string Server::_buildFdErrnoMessage(const std::string& context, int fd) {
  std::ostringstream oss;
  oss << "Error: " << context << " on Fd " << fd << " (errno = " << errno
      << ")";
  return oss.str();
}

// Signal Handler
void Server::_handleSignal(int signo) {
  (void)signo;
  _shouldStop = 1;
}

void Server::setupSignalHandlers() {
  struct sigaction sa;
  if (sigemptyset(&sa.sa_mask) == -1) {
    throw std::runtime_error(_buildErrnoMessage("sigemptyset() failed"));
  }
  sa.sa_handler = Server::_handleSignal;
  sa.sa_flags = 0;
  if (sigaction(SIGINT, &sa, NULL) == -1 ||
      sigaction(SIGTERM, &sa, NULL) == -1) {
    throw std::runtime_error(_buildErrnoMessage("sigaction() failed"));
  }

  struct sigaction sa_pipe;
  if (sigemptyset(&sa_pipe.sa_mask) == -1) {
    throw std::runtime_error(_buildErrnoMessage("sigemptyset() failed"));
  }
  sa_pipe.sa_handler = SIG_IGN;
  sa_pipe.sa_flags = 0;
  if (sigaction(SIGPIPE, &sa_pipe, NULL) == -1) {
    throw std::runtime_error(_buildErrnoMessage("sigaction() failed"));
  }
}

void Server::start() {
  _setupServerSocket();

  std::cout << "Server is running on port " << _port << "..." << std::endl;

  while (!_shouldStop) {
    _updatePollEvents();
    if (_waitForEvents())
      _processActiveConnections();
  }

  std::cout << "Server shutdown..." << std::endl;
}

// start() helper functions: _setupServerSocket(), _updatePollEvents(),
// _wailForEvents(), and _processActiveConnections()
void Server::_setupServerSocket() {
  _serverFd = socket(AF_INET, SOCK_STREAM, 0);
  if (_serverFd == -1) {
    throw std::runtime_error(_buildErrnoMessage("socket() failed"));
  }

  int opt = 1;
  if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ==
      -1) {
    throw std::runtime_error(_buildErrnoMessage("setsockopt() failed"));
  }

  _setNonBlocking(_serverFd);

  struct sockaddr_in serverAddr;
  std::memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(_port);

  if (bind(_serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) ==
      -1) {
    throw std::runtime_error(_buildErrnoMessage("bind() failed"));
  }

  if (listen(_serverFd, SOMAXCONN) == -1) {
    throw std::runtime_error(_buildErrnoMessage("listen() failed"));
  }

  struct pollfd serverPollFd;
  serverPollFd.fd = _serverFd;
  serverPollFd.events = POLLIN;
  serverPollFd.revents = 0;
  _pollFds.push_back(serverPollFd);
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

bool Server::_waitForEvents() {
  if (poll(&_pollFds[0], _pollFds.size(), -1) == -1) {
    if (errno == EINTR)
      return false;
    std::string errMsg = _buildErrnoMessage("poll() failed");
    throw std::runtime_error(errMsg);
  }
  return true;
}

void Server::_processActiveConnections() {
  for (size_t i = 0; i < _pollFds.size(); ++i) {
    if (_pollFds[i].revents == 0)
      continue;
    if (_pollFds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
      _processPollError(i);
      continue;
    }
    if (_pollFds[i].revents & POLLIN) {
      if (_processReadableEvent(i) == CONTINUE_LOOP)
        continue;
    }
    if (_pollFds[i].revents & POLLOUT) {
      _flushSendBuffer(i);
    }
  }
}

// helpers for process active connections
void Server::_processPollError(size_t& index) {
  if (_pollFds[index].fd == _serverFd) {
    throw std::runtime_error("Fatal error on server socket.");
  } else {
    std::cout << "[!] Socket error or hangup detected. Fd: "
              << _pollFds[index].fd << std::endl;

    _disconnectClient(index,
                      _makeQuitMessage(*_clients[_pollFds[index].fd],
                                       "Connection closed (Error/Hangup)"));
  }
}

Server::LoopAction Server::_processReadableEvent(size_t& index) {
  if (_pollFds[index].fd == _serverFd) {
    _acceptNewConnection();
    return KEEP_PROCESSING;
  }
  if (_handleClientMessage(_pollFds[index]) == DISCONNECT) {
    _disconnectClient(index, _makeQuitMessage(*_clients[_pollFds[index].fd],
                                              "Client disconnected"));
    return CONTINUE_LOOP;
  }
  return KEEP_PROCESSING;
}

void Server::_flushSendBuffer(size_t index) {
  if (_pollFds[index].fd != _serverFd) {
    Client* client = _clients[_pollFds[index].fd];
    std::string& sendBuf = client->getSendBuffer();

    if (!sendBuf.empty()) {
      int bytesSent =
          send(_pollFds[index].fd, sendBuf.c_str(), sendBuf.length(), 0);
      if (bytesSent > 0) {
        client->eraseSendBuffer(bytesSent);
      } else if (bytesSent == -1) {

        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {

          std::cerr << _buildFdErrnoMessage("send() failed", _pollFds[index].fd)
                    << std::endl;
        }
      }
    }

    if (client->getSendBuffer().empty()) {
      _pollFds[index].events &= ~POLLOUT;
    }
  }
}

void Server::_acceptNewConnection() {
  struct sockaddr_in clientAddr;
  socklen_t clientLen = sizeof(clientAddr);

  int clientFd = accept(_serverFd, (struct sockaddr*)&clientAddr, &clientLen);
  if (clientFd == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
      return;

    std::string errMsg = _buildErrnoMessage("accept() failed");
    throw std::runtime_error(errMsg);
  }

  char host[INET_ADDRSTRLEN];
  if (inet_ntop(AF_INET, &clientAddr.sin_addr, host, sizeof(host)) == NULL) {
    close(clientFd);
    std::cerr << _buildErrnoMessage("inet_ntop() failed") << std::endl;
    return;
  }

  _setNonBlocking(clientFd);
  _registerClient(clientFd, host);
}

Server::ConnectionStatus Server::_handleClientMessage(
    struct pollfd& clientPollFd) {
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

    std::cerr << _buildErrnoMessage("recv() failed") << std::endl;
    std::cout << "[+] A client has disconnected. Fd: " << clientPollFd.fd
              << std::endl;
    return DISCONNECT;
  }
}

bool Server::_executeCommand(Client* client, const Message& msg) {
  CommandContext ctx(*client, msg, _serverCtx);
  return _dispatcher.dispatch(ctx);
}

void Server::_registerClient(int clientFd, const std::string& host) {
  struct pollfd clientPollFd;
  clientPollFd.fd = clientFd;
  clientPollFd.events = POLLIN;
  clientPollFd.revents = 0;
  _pollFds.push_back(clientPollFd);

  _clients.insert(std::make_pair(clientFd, new Client(clientFd, host)));
  std::cout << "[+] A new client has connected. Fd: " << clientFd << std::endl;
}

void Server::_disconnectClient(size_t& index, const std::string& quitMsg) {
  int fd = _pollFds[index].fd;
  _serverCtx.removeClientFromAllChannels(*_clients[fd], quitMsg);
  delete _clients[fd];
  _clients.erase(fd);
  _pollFds.erase(_pollFds.begin() + index);
  --index;
}

std::string Server::_makeQuitMessage(const Client& client,
                                     const std::string& reason) const {
  return ":" + client.getPrefix() + " QUIT :" + reason;
}

void Server::_setNonBlocking(int fd) {
  if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
    throw std::runtime_error(_buildErrnoMessage("fcntl() failed"));
  }
}
