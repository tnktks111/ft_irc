#include "../include/Server.hpp"
#include "../include/CommandContext.hpp"
#include "../include/Message.hpp"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

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

void Server::_setNonBlocking(int fd) {
  /*
  fcntl(fd, F_SETFL, O_NONBLOCK):
  fcntl は、ファイル記述子（ソケット）の動作を制御（File
  CoNTroL）するシステムコールです。 F_SETFL は「フラグを設定（SET
  FLags）する」という命令です。 O_NONBLOCK
  が今回の肝で、「このソケットでの入出力操作を待機させない（非ブロックにする）」というフラグです。
  OS側で設定に失敗すると -1
  が返されるため、その場合は例外（std::runtime_error）を投げてエラーを知らせます。

  具体的なネットワーク処理での挙動：
  通常（ブロック）：
  データが届いていない状態で
  recv()（受信）を呼ぶと、データが届くまでプログラムがそこでピタッと止まります。

  非ブロック：
  データが届いていない状態で recv()
  を呼ぶと、止まらずにすぐ「今はデータがないよ（EAGAIN /
  EWOULDBLOCK）」というエラーを返して次の処理へ進ませてくれます。
  */
  if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
    throw std::runtime_error("Error: failed to setup non-blocking mode.");
  }
}

void Server::_setupServerSocket() {
  // 通信を行うためのソケットの作成(socket(プロトコルファミリー、ソケットのタイプ、使用するプロトコル))
  _serverFd = socket(AF_INET, SOCK_STREAM, 0);
  if (_serverFd == -1) {
    throw std::runtime_error("Error: failed to create server socket.");
  }

  /*
  サーバーを再起動したときに、同じポート番号をすぐに再利用できるようにする設定
  Address already in
  useというエラーでサーバーが起動できなくなるのを防ぐためのおまじない
  通常、ネットワークサーバーを終了させた直後、OSはそのポート（例：8080番など）を「念のため」しばらくの間（TIME_WAIT状態といいます）ロックします。

  これがない場合：
  サーバーを停止してすぐに再起動しようとすると、「さっきまで使っていたポートがまだ解放されていない」とOSに怒られ、サーバーの起動に失敗します（通常、数分待つ必要があります）。

  これがある場合：
  OSに対し「さっき使っていたポートだけど、すぐに使い回して大丈夫だよ！」と伝えるため、停止後すぐに再起動が可能になります。
  開発中にサーバーを何度も「停止→修正→起動」と繰り返す場合、この設定がないと非常に不便なため、ほぼ全てのサーバープログラムで書かれる定番の処理です。
  */
  int opt = 1;
  if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ==
      -1) {
    throw std::runtime_error("Error: setsockopt() failed.");
  }

  _setNonBlocking(_serverFd);

  /*
  AF_INET...「IPv4」を使うことを指定しています（AF = Address Family）
  INADDR_ANY...どのIPアドレスからの接続も受け付ける」という設定です。サーバーに複数のLANカードやIPがあっても、全部で待ち受けます。
  htons...リトルエンディアン→ビッグエンディアンへ変換
  */
  struct sockaddr_in serverAddr;
  std::memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(_port);

  // socketを特定のIPアドレスとポートに紐づけ(bind(ソケット、ソケットに割り当てるアドレスやポート番号の情報、←のサイズ（バイト数）))
  if (bind(_serverFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) ==
      -1) {
    throw std::runtime_error("Error: failed to bind port.");
  }

  // 接続の待受を開始(listen(接続を待つソケット、接続要求を保持する数))
  if (listen(_serverFd, SOMAXCONN) == -1) {
    throw std::runtime_error("Error: listen() failed.");
  }

  /*
  struct pollfd server PollFd;
  - poll関数で監視したい情報を入れるための構造体を作ります。
  serverPollFd.fd = _serverFd;
  - 監視対象のソケット（サーバーの窓口）を指定します。
  serverPollFd.events = POLLIN;
  - 「何が起きたら教えてほしいか」を設定します。POLLIN
  は「データが入ってきた（IN）」＝「新しい接続が来た」ことを指します。
  serverPollFd.revents = 0;
  - 「実際に何が起きたか」をOSが書き込む場所です。最初は 0
  でクリアしておきます。_pollFds.push_back(...);
  - 監視対象リスト（vector など）に追加します。
  */
  struct pollfd serverPollFd;
  serverPollFd.fd = _serverFd;
  serverPollFd.events = POLLIN;
  serverPollFd.revents = 0;
  _pollFds.push_back(serverPollFd);
}

void Server::_waitForEvents() {
  if (poll(&_pollFds[0], _pollFds.size(), -1) == -1) {
    throw std::runtime_error("Error: poll() failed.");
  }
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
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
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
    _setNonBlocking(clientFd);

    struct pollfd clientPollFd;
    clientPollFd.fd = clientFd;
    clientPollFd.events = POLLIN;
    clientPollFd.revents = 0;
    _pollFds.push_back(clientPollFd);

    _clients.insert(std::make_pair(clientFd, new Client(clientFd)));

    std::cout << "[+] A new client has connected. Fd: " << clientFd
              << std::endl;
  }
}

Server::ConnectionStatus
Server::_handleClientMessage(struct pollfd &clientPollFd) {
  char buffer[1024];
  std::memset(buffer, 0, sizeof(buffer));

  int bytesRead = recv(clientPollFd.fd, buffer, sizeof(buffer) - 1, 0);

  if (bytesRead > 0) {

    _clients[clientPollFd.fd]->appendRecvBuffer(buffer);

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
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
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

  while (true) {
    _updatePollEvents();
    _waitForEvents();
    _processActiveConnections();
  }
}
