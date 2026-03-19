#include "../include/Server.hpp"
#include "../include/Message.hpp"
#include "../include/ReplyBuilder.hpp"
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
    : _port(port), _password(password), _serverFd(-1) {}

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
        _removeClientFromAllChannels(_pollFds[i].fd, quitMsg);

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
          _removeClientFromAllChannels(_pollFds[i].fd, quitMsg);

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

void Server::_sendMessage(int fd, const std::string &msg) {
  std::string fullMsg = msg + "\r\n";
  if (_clients.find(fd) != _clients.end()) {
    _clients[fd]->appendSendBuffer(fullMsg);
    std::cout << "[Queued for Fd: " << fd << "] " << msg << std::endl;
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
  const std::string &cmd = msg.getCommand();
  if (cmd == "QUIT") {
    _handleQuit(client, msg);
    return false;
  } else if (cmd == "PASS") {
    _handlePass(client, msg);
  } else if (cmd == "NICK") {
    _handleNick(client, msg);
  } else if (cmd == "USER") {
    _handleUser(client, msg);
  } else {
    if (!client->isRegistered()) {
      _sendMessage(client->getFd(), ReplyBuilder::errNotRegistered());
    } else {
      if (cmd == "JOIN") {
        _handleJoin(client, msg);
      } else if (cmd == "PRIVMSG") {
        _handlePrivMsg(client, msg);
      } else if (cmd == "PART") {
        _handlePart(client, msg);
      } else if (cmd == "TOPIC") {
        _handleTopic(client, msg);
      } else if (cmd == "KICK") {
        _handleKick(client, msg);
      } else if (cmd == "INVITE") {
        _handleInvite(client, msg);
      } else if (cmd == "MODE") {
        _handleMode(client, msg);
      } else if (cmd == "PING") {
        _handlePing(client, msg);
      } else {
        std::cout << "Command from an authenticated User: " << cmd << std::endl;
      }
    }
  }
  return true;
}

// PASS <password>
void Server::_handlePass(Client *client, const Message &msg) {
  if (client->isRegistered()) {
    _sendMessage(client->getFd(),
                 ReplyBuilder::errAlreadyRegistered(client->getNickName()));
    return;
  }
  if (msg.getParams().empty()) {
    _sendMessage(client->getFd(), ReplyBuilder::errNeedMoreParams("*", "PASS"));
    return;
  }

  if (msg.getParams()[0] == _password) {
    client->setPassChecked(true);
  } else {
    _sendMessage(client->getFd(), ReplyBuilder::errIncorrectPassword());
  }
}

// NICK <nickname>
void Server::_handleNick(Client *client, const Message &msg) {
  if (msg.getParams().empty()) {
    _sendMessage(client->getFd(), ReplyBuilder::errNoNicknameGiven());
    return;
  }
  std::string newNick = msg.getParams()[0];

  for (std::map<int, Client *>::iterator it = _clients.begin();
       it != _clients.end(); ++it) {
    if (it->first != client->getFd() && it->second->getNickName() == newNick) {
      _sendMessage(client->getFd(), ReplyBuilder::errNickNameInUse(newNick));
      return;
    }
  }

  client->setNickName(newNick);
  _checkRegistration(client);
}

// USER <username> <hostname> <servername> <realname>
void Server::_handleUser(Client *client, const Message &msg) {
  if (client->isRegistered()) {
    _sendMessage(client->getFd(),
                 ReplyBuilder::errAlreadyRegistered(client->getNickName()));
    return;
  }
  if (msg.getParams().size() < 4) {
    _sendMessage(client->getFd(), ReplyBuilder::errNeedMoreParams("*", "USER"));
    return;
  }

  client->setUserName(msg.getParams()[0]);
  _checkRegistration(client);
}

void Server::_checkRegistration(Client *client) {
  if (client->isRegistered())
    return;

  if (client->isPassChecked() && !client->getNickName().empty() &&
      !client->getUserName().empty()) {
    client->setRegistered(true);

    _sendMessage(
        client->getFd(),
        ReplyBuilder::rplWelcome(client->getNickName(), client->getPrefix()));

    std::cout << "[+] Client(FD: " << client->getFd()
              << ") has successfully logged in." << std::endl;
  }
}

std::string Server::_generateChannelMemberStr(const Channel *channel) {
  std::string namesList;

  const std::map<int, Client *> &members = channel->getMembers();
  for (std::map<int, Client *>::const_iterator it = members.begin();
       it != members.end(); ++it) {
    Client *member = it->second;
    if (!namesList.empty())
      namesList += " ";

    if (channel->isOperator(it->first))
      namesList += "@";

    namesList += member->getNickName();
  }

  return namesList;
}

// JOIN <channel>
void Server::_handleJoin(Client *client, const Message &msg) {
  if (msg.getParams().empty()) {
    _sendMessage(client->getFd(), ReplyBuilder::errNeedMoreParams(
                                      client->getNickName(), "JOIN"));
    return;
  }

  std::string chName = msg.getParams()[0];
  bool isNewChannel = false;

  if (_channels.find(chName) == _channels.end()) {
    _channels[chName] = new Channel(chName);
    isNewChannel = true;
    std::cout << "[+] Channel created: " << chName << std::endl;
  }

  Channel *channel = _channels[chName];

  if (!isNewChannel && channel->isInviteOnly()) {
    if (!channel->isInvited(client->getNickName())) {
      _sendMessage(client->getFd(), ReplyBuilder::errInviteOnlyChan(
                                        client->getNickName(), chName));
      return;
    }
  }

  if (!isNewChannel && !channel->getPassword().empty()) {
    std::string providedKey =
        (msg.getParams().size() > 1) ? msg.getParams()[1] : "";

    if (providedKey != channel->getPassword()) {
      _sendMessage(client->getFd(), ReplyBuilder::errBadChannelKey(
                                        client->getNickName(), chName));
      return;
    }
  }

  if (!isNewChannel && channel->getUserLimit() > 0) {
    if (channel->getMemberCount() >= channel->getUserLimit()) {
      _sendMessage(client->getFd(), ReplyBuilder::errChannelIsFull(
                                        client->getNickName(), chName));
      return;
    }
  }

  if (!channel->hasMember(client->getFd())) {
    channel->addMember(client);

    if (isNewChannel) {
      channel->addOperator(client->getFd());
      std::cout << "[*] " << client->getNickName() << " is now the operator of "
                << chName << std::endl;
    }

    channel->removeInvite(client->getNickName());

    std::string joinMsg = ":" + client->getPrefix() + " JOIN :" + chName;
    _sendMessage(client->getFd(), joinMsg);
    channel->broadcastMessage(joinMsg, client->getFd());

    if (!channel->getTopic().empty()) {
      _sendMessage(client->getFd(),
                   ReplyBuilder::rplTopic(client->getNickName(), chName,
                                          channel->getTopic()));
    } else {
      _sendMessage(client->getFd(),
                   ReplyBuilder::rplNoTopic(client->getNickName(), chName));
    }

    std::string namesList = _generateChannelMemberStr(channel);
    _sendMessage(
        client->getFd(),
        ReplyBuilder::rplNamReply(client->getNickName(), chName, namesList));
    _sendMessage(client->getFd(),
                 ReplyBuilder::rplEndOfNames(client->getNickName(), chName));
  }
}

// PRIVMSG <target> <message>
void Server::_handlePrivMsg(Client *client, const Message &msg) {
  if (msg.getParams().size() < 2) {
    _sendMessage(client->getFd(),
                 ReplyBuilder::errNoTextToSend(client->getNickName()));
    return;
  }

  std::string target = msg.getParams()[0];
  std::string text = msg.getParams()[1];
  std::string fullMsg =
      ":" + client->getPrefix() + " PRIVMSG " + target + " :" + text;

  if (target[0] == '#') {
    if (_channels.find(target) != _channels.end()) {
      Channel *channel = _channels[target];
      if (channel->hasMember(client->getFd())) {
        channel->broadcastMessage(fullMsg, client->getFd());
      } else {
        _sendMessage(client->getFd(), ReplyBuilder::errCantSendToChannel(
                                          client->getNickName(), target));
      }
    } else {
      _sendMessage(client->getFd(), ReplyBuilder::errNoSuchNickChannel(
                                        client->getNickName(), target));
    }
  } else {
    for (std::map<int, Client *>::iterator it = _clients.begin();
         it != _clients.end(); ++it) {
      if (it->second->getNickName() == target) {
        _sendMessage(it->second->getFd(), fullMsg);
        return;
      }
    }
    _sendMessage(client->getFd(), ReplyBuilder::errNoSuchNickChannel(
                                      client->getNickName(), target));
  }
}

// PART <channel> *( "," <channel> ) [ <Part Message> ]
void Server::_handlePart(Client *client, const Message &msg) {
  if (msg.getParams().empty()) {
    _sendMessage(client->getFd(), ReplyBuilder::errNeedMoreParams(
                                      client->getNickName(), "PART"));
    return;
  }

  std::string chName = msg.getParams()[0];
  std::string partMsg =
      (msg.getParams().size() > 1) ? msg.getParams()[1] : "Leaving";

  if (_channels.find(chName) == _channels.end()) {
    _sendMessage(client->getFd(),
                 ReplyBuilder::errNoSuchChannel(client->getNickName(), chName));
    return;
  }

  Channel *channel = _channels[chName];
  if (!channel->hasMember(client->getFd())) {
    _sendMessage(client->getFd(),
                 ReplyBuilder::errNotOnChannel(client->getNickName(), chName));
    return;
  }

  std::string fullMsg =
      ":" + client->getPrefix() + " PART " + chName + " :" + partMsg;
  _sendMessage(client->getFd(), fullMsg);
  channel->broadcastMessage(fullMsg, client->getFd());

  channel->removeMember(client->getFd());

  if (channel->getMemberCount() == 0) {
    delete channel;
    _channels.erase(chName);
    std::cout << "[-] Channel deleted (no member): " << chName << std::endl;
  }
}

// QUIT [ < Quit Message > ]
void Server::_handleQuit(Client *client, const Message &msg) {
  std::string reason =
      msg.getParams().empty() ? "Client Quit" : msg.getParams()[0];
  std::string quitMsg = ":" + client->getPrefix() + " QUIT :" + reason;

  _removeClientFromAllChannels(client->getFd(), quitMsg);
}

void Server::_removeClientFromAllChannels(int fd, const std::string &quitMsg) {
  std::vector<std::string> emptyChannels;

  for (std::map<std::string, Channel *>::iterator it = _channels.begin();
       it != _channels.end(); ++it) {
    Channel *channel = it->second;
    if (channel->hasMember(fd)) {
      channel->broadcastMessage(quitMsg, fd);
      channel->removeMember(fd);

      if (channel->getMemberCount() == 0) {
        emptyChannels.push_back(it->first);
      }
    }
  }

  for (std::vector<std::string>::iterator it = emptyChannels.begin();
       it != emptyChannels.end(); ++it) {
    delete _channels[*it];
    _channels.erase(*it);
    std::cout << "[-] Channel deleted (no member): " << *it << std::endl;
  }
}

// TOPIC <channel> [ <topic> ]
void Server::_handleTopic(Client *client, const Message &msg) {
  if (msg.getParams().empty()) {
    _sendMessage(client->getFd(), ReplyBuilder::errNeedMoreParams(
                                      client->getNickName(), "TOPIC"));
    return;
  }

  std::string chName = msg.getParams()[0];

  if (_channels.find(chName) == _channels.end()) {
    _sendMessage(client->getFd(),
                 ReplyBuilder::errNoSuchChannel(client->getNickName(), chName));
    return;
  }

  Channel *channel = _channels[chName];

  if (!channel->hasMember(client->getFd())) {
    _sendMessage(client->getFd(),
                 ReplyBuilder::errNotOnChannel(client->getNickName(), chName));
    return;
  }

  if (msg.getParams().size() == 1) {
    if (channel->getTopic().empty()) {
      _sendMessage(client->getFd(),
                   ReplyBuilder::rplNoTopic(client->getNickName(), chName));
    } else {
      _sendMessage(client->getFd(),
                   ReplyBuilder::rplTopic(client->getNickName(), chName,
                                          channel->getTopic()));
    }
    return;
  }

  std::string newTopic = msg.getParams()[1];

  if (channel->isTopicProtected() && !channel->isOperator(client->getFd())) {
    _sendMessage(client->getFd(), ReplyBuilder::errChanOPrivsNeeded(
                                      client->getNickName(), chName));
    return;
  }

  channel->setTopic(newTopic);

  std::string topicMsg =
      ":" + client->getPrefix() + " TOPIC " + chName + " :" + newTopic;
  _sendMessage(client->getFd(), topicMsg);
  channel->broadcastMessage(topicMsg, client->getFd());
}

Client *Server::_getClientByNickname(const std::string &nickname) {
  for (std::map<int, Client *>::iterator it = _clients.begin();
       it != _clients.end(); ++it) {
    if (it->second->getNickName() == nickname) {
      return it->second;
    }
  }
  return NULL;
}

// KICK <channel> <user> [ <comment> ]
void Server::_handleKick(Client *client, const Message &msg) {
  if (msg.getParams().size() < 2) {
    _sendMessage(client->getFd(), ReplyBuilder::errNeedMoreParams(
                                      client->getNickName(), "KICK"));
    return;
  }

  std::string chName = msg.getParams()[0];
  std::string targetNick = msg.getParams()[1];
  std::string reason =
      (msg.getParams().size() > 2) ? msg.getParams()[2] : "Kicked by operator";

  if (_channels.find(chName) == _channels.end()) {
    _sendMessage(client->getFd(),
                 ReplyBuilder::errNoSuchChannel(client->getNickName(), chName));
    return;
  }

  Channel *channel = _channels[chName];

  if (!channel->hasMember(client->getFd())) {
    _sendMessage(client->getFd(),
                 ReplyBuilder::errNotOnChannel(client->getNickName(), chName));
    return;
  }

  if (!channel->isOperator(client->getFd())) {
    _sendMessage(client->getFd(), ReplyBuilder::errChanOPrivsNeeded(
                                      client->getNickName(), chName));
    return;
  }

  Client *targetClient = _getClientByNickname(targetNick);
  if (targetClient == NULL) {
    _sendMessage(client->getFd(), ReplyBuilder::errNoSuchNickChannel(
                                      client->getNickName(), targetNick));
    return;
  }

  if (!channel->hasMember(targetClient->getFd())) {
    _sendMessage(client->getFd(),
                 ReplyBuilder::errUserNotInChannel(client->getNickName(),
                                                   targetNick, chName));
    return;
  }

  std::string kickMsg = ":" + client->getPrefix() + " KICK " + chName + " " +
                        targetNick + " :" + reason;
  channel->broadcastMessage(kickMsg, client->getFd());
  _sendMessage(client->getFd(), kickMsg);

  channel->removeMember(targetClient->getFd());

  if (channel->getMemberCount() == 0) {
    delete channel;
    _channels.erase(chName);
    std::cout << "[-] Channel deleted (no member): " << chName << std::endl;
  }
}

// INVITE <nickname> <channel>
void Server::_handleInvite(Client *client, const Message &msg) {
  if (msg.getParams().size() < 2) {
    _sendMessage(client->getFd(), ReplyBuilder::errNeedMoreParams(
                                      client->getNickName(), "INVITE"));
    return;
  }

  std::string targetNick = msg.getParams()[0];
  std::string chName = msg.getParams()[1];

  Client *targetClient = _getClientByNickname(targetNick);
  if (targetClient == NULL) {
    _sendMessage(client->getFd(), ReplyBuilder::errNoSuchNickChannel(
                                      client->getNickName(), targetNick));
    return;
  }

  if (_channels.find(chName) == _channels.end()) {
    _sendMessage(client->getFd(),
                 ReplyBuilder::errNoSuchChannel(client->getNickName(), chName));
    return;
  }

  Channel *channel = _channels[chName];

  if (!channel->hasMember(client->getFd())) {
    _sendMessage(client->getFd(),
                 ReplyBuilder::errNotOnChannel(client->getNickName(), chName));
    return;
  }

  if (channel->hasMember(targetClient->getFd())) {
    _sendMessage(client->getFd(),
                 ReplyBuilder::errUserOnChannel(client->getNickName(),
                                                targetNick, chName));
    return;
  }

  if (channel->isInviteOnly() && !channel->isOperator(client->getFd())) {
    _sendMessage(client->getFd(), ReplyBuilder::errChanOPrivsNeeded(
                                      client->getNickName(), chName));
    return;
  }

  channel->addInvite(targetNick);

  std::string inviteMsg =
      ":" + client->getPrefix() + " INVITE " + targetNick + " :" + chName;
  _sendMessage(targetClient->getFd(), inviteMsg);
  _sendMessage(client->getFd(), ReplyBuilder::rplInviting(client->getNickName(),
                                                          chName, targetNick));
}

void Server::_handleMode(Client *client, const Message &msg) {
  if (msg.getParams().empty()) {
    _sendMessage(client->getFd(), ReplyBuilder::errNeedMoreParams(
                                      client->getNickName(), "MODE"));
    return;
  }

  std::string target = msg.getParams()[0];

  if (target[0] != '#')
    return;

  if (_channels.find(target) == _channels.end()) {
    _sendMessage(client->getFd(),
                 ReplyBuilder::errNoSuchChannel(client->getNickName(), target));
    return;
  }

  Channel *channel = _channels[target];

  if (msg.getParams().size() == 1) {
    std::string currentMode = "+";
    std::string modeParams = "";

    if (channel->isInviteOnly())
      currentMode += "i";
    if (channel->isTopicProtected())
      currentMode += "t";
    if (!channel->getPassword().empty()) {
      currentMode += "k";
      modeParams += channel->getPassword();
    }
    if (channel->getUserLimit() > 0) {
      currentMode += "l";

      if (!modeParams.empty())
        modeParams += " ";
      std::ostringstream oss;
      oss << channel->getUserLimit();
      modeParams += oss.str();
    }

    _sendMessage(client->getFd(),
                 ReplyBuilder::rplChannelModeIs(client->getNickName(), target,
                                                currentMode, modeParams));
    return;
  }

  if (!channel->isOperator(client->getFd())) {
    _sendMessage(client->getFd(), ReplyBuilder::errChanOPrivsNeeded(
                                      client->getNickName(), target));
    return;
  }

  bool isAdding = true;
  std::string appliedModes = "";
  std::string appliedParams = "";
  char lastSign = '\0';

  std::string modeString = msg.getParams()[1];

  size_t argIdx = 2;

  for (std::string::iterator it = modeString.begin(); it != modeString.end();
       ++it) {
    if (*it == '+') {
      isAdding = true;
    } else if (*it == '-') {
      isAdding = false;
    } else if (*it == 'i') {
      if (isAdding == !channel->isInviteOnly()) {
        channel->setInviteOnly(isAdding);

        if (lastSign != (isAdding ? '+' : '-')) {
          lastSign = isAdding ? '+' : '-';
          appliedModes += lastSign;
        }
        appliedModes += "i";
      }
    } else if (*it == 't') {
      if (isAdding == !channel->isTopicProtected()) {
        channel->setTopicProtected(isAdding);

        if (lastSign != (isAdding ? '+' : '-')) {
          lastSign = isAdding ? '+' : '-';
          appliedModes += lastSign;
        }
        appliedModes += "t";
      }
    } else if (*it == 'k') {

      if (isAdding) {
        if (argIdx >= msg.getParams().size()) {
          _sendMessage(client->getFd(), ReplyBuilder::errNeedMoreParams(
                                            client->getNickName(), "MODE"));
          break;
        }
        std::string key = msg.getParams()[argIdx++];
        channel->setPassword(key);
        if (lastSign != '+') {
          lastSign = '+';
          appliedModes += "+";
        }
        appliedModes += "k";
        appliedParams += " " + key;
      } else {
        if (!channel->getPassword().empty()) {
          channel->setPassword("");
          if (lastSign != '-') {
            lastSign = '-';
            appliedModes += "-";
          }
          appliedModes += "k";
        }
      }
    } else if (*it == 'l') {
      if (isAdding) {
        if (argIdx >= msg.getParams().size()) {
          _sendMessage(client->getFd(), ReplyBuilder::errNeedMoreParams(
                                            client->getNickName(), "MODE"));
          break;
        }
        std::string limitStr = msg.getParams()[argIdx++];
        std::istringstream iss(limitStr);
        int limit;

        if ((iss >> limit) && limit > 0 &&
            channel->getUserLimit() != (size_t)limit) {
          channel->setUserLimit(limit);
          if (lastSign != '+') {
            lastSign = '+';
            appliedModes += "+";
          }
          appliedModes += "l";
          appliedParams += " " + limitStr;
        }
      } else {
        if (channel->getUserLimit() > 0) {
          channel->setUserLimit(0);
          if (lastSign != '-') {
            lastSign = '-';
            appliedModes += "-";
          }
          appliedModes += "l";
        }
      }
    } else if (*it == 'o') {
      if (argIdx >= msg.getParams().size()) {
        _sendMessage(client->getFd(), ReplyBuilder::errNeedMoreParams(
                                          client->getNickName(), "MODE"));
        break;
      }
      std::string targetNick = msg.getParams()[argIdx++];
      Client *targetClient = _getClientByNickname(targetNick);
      if (targetClient && channel->hasMember(targetClient->getFd())) {
        if (isAdding != channel->isOperator(targetClient->getFd())) {
          if (isAdding) {
            channel->addOperator(targetClient->getFd());
          } else {
            channel->removeOperator(targetClient->getFd());
          }
        }

        if (lastSign != (isAdding ? '+' : '-')) {
          lastSign = isAdding ? '+' : '-';
          appliedModes += lastSign;
        }
        appliedModes += "o";
        appliedParams += " " + targetNick;
      } else {
        _sendMessage(client->getFd(),
                     ReplyBuilder::errUserNotInChannel(client->getNickName(),
                                                       targetNick, target));
      }
    }

    else {
      _sendMessage(client->getFd(), ReplyBuilder::errUnknownMode(
                                        client->getNickName(), *it, target));
    }
  }

  if (!appliedModes.empty()) {
    std::string modeMsg = ":" + client->getPrefix() + " MODE " + target + " " +
                          appliedModes + appliedParams;
    channel->broadcastMessage(modeMsg, client->getFd());
    _sendMessage(client->getFd(), modeMsg);
  }
}

void Server::_handlePing(Client *client, const Message &msg) {
  if (msg.getParams().empty()) {
    _sendMessage(client->getFd(),
                 ReplyBuilder::errNoOrigin(client->getNickName()));
    return;
  }

  std::string token = msg.getParams()[0];
  _sendMessage(client->getFd(), "PONG " + token);
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
