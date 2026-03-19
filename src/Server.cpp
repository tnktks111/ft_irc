#include "../include/Server.hpp"
#include "../include/CommandContext.hpp"
#include "../include/Message.hpp"
#include "../include/ReplyBuilder.hpp"
#include "../include/ServerContext.hpp"
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
  ServerContext serverCtx(_clients, _channels, _responseSink, _password);
  CommandContext ctx(*client, msg, serverCtx);

  if (cmd == "QUIT") {
    _handleQuit(ctx);
    return false;
  } else if (cmd == "PASS") {
    _handlePass(ctx);
  } else if (cmd == "NICK") {
    _handleNick(ctx);
  } else if (cmd == "USER") {
    _handleUser(ctx);
  } else {
    if (!ctx.isRegistered()) {
      ctx.reply(ReplyBuilder::errNotRegistered());
    } else {
      if (cmd == "JOIN") {
        _handleJoin(ctx);
      } else if (cmd == "PRIVMSG") {
        _handlePrivMsg(ctx);
      } else if (cmd == "PART") {
        _handlePart(ctx);
      } else if (cmd == "TOPIC") {
        _handleTopic(ctx);
      } else if (cmd == "KICK") {
        _handleKick(ctx);
      } else if (cmd == "INVITE") {
        _handleInvite(ctx);
      } else if (cmd == "MODE") {
        _handleMode(ctx);
      } else if (cmd == "PING") {
        _handlePing(ctx);
      } else {
        std::cout << "Command from an authenticated User: " << cmd << std::endl;
      }
    }
  }
  return true;
}

// PASS <password>
void Server::_handlePass(CommandContext &ctx) {
  if (ctx.isRegistered()) {
    ctx.reply(ReplyBuilder::errAlreadyRegistered(ctx.nick()));
    return;
  }
  if (ctx.params().empty()) {
    ctx.reply(ReplyBuilder::errNeedMoreParams("*", "PASS"));
    return;
  }

  if (ctx.params()[0] == ctx.serverCtx().password()) {
    ctx.client().setPassChecked(true);
  } else {
    ctx.reply(ReplyBuilder::errIncorrectPassword());
  }
}

// NICK <nickname>
void Server::_handleNick(CommandContext &ctx) {
  if (ctx.params().empty()) {
    ctx.reply(ReplyBuilder::errNoNicknameGiven());
    return;
  }
  std::string newNick = ctx.params()[0];

  if (ctx.serverCtx().hasNick(newNick, ctx.client())) {
    ctx.reply(ReplyBuilder::errNickNameInUse(newNick));
    return;
  }

  ctx.client().setNickName(newNick);
  _checkRegistration(&ctx.client());
}

// USER <username> <hostname> <servername> <realname>
void Server::_handleUser(CommandContext &ctx) {
  if (ctx.isRegistered()) {
    ctx.reply(ReplyBuilder::errAlreadyRegistered(ctx.nick()));
    return;
  }
  if (ctx.params().size() < 4) {
    ctx.reply(ReplyBuilder::errNeedMoreParams("*", "USER"));
    return;
  }

  ctx.client().setUserName(ctx.params()[0]);
  _checkRegistration(&ctx.client());
}

void Server::_checkRegistration(Client *client) {
  if (client->isRegistered())
    return;

  if (client->isPassChecked() && !client->getNickName().empty() &&
      !client->getUserName().empty()) {
    client->setRegistered(true);

    _responseSink.reply(*client, ReplyBuilder::rplWelcome(client->getNickName(),
                                                          client->getPrefix()));

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
void Server::_handleJoin(CommandContext &ctx) {
  if (ctx.params().empty()) {
    ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "JOIN"));
    return;
  }

  std::string chName = ctx.params()[0];
  ServerContext::ChannelSlot channelSlot = ctx.serverCtx().getOrCreateChannel(chName);
  Channel *channel = channelSlot.first;
  bool isNewChannel = channelSlot.second;

  if (isNewChannel) {
    std::cout << "[+] Channel created: " << chName << std::endl;
  }

  if (!isNewChannel && channel->isInviteOnly()) {
    if (!channel->isInvited(ctx.nick())) {
      ctx.reply(ReplyBuilder::errInviteOnlyChan(ctx.nick(), chName));
      return;
    }
  }

  if (!isNewChannel && !channel->getPassword().empty()) {
    std::string providedKey = (ctx.params().size() > 1) ? ctx.params()[1] : "";

    if (providedKey != channel->getPassword()) {
      ctx.reply(ReplyBuilder::errBadChannelKey(ctx.nick(), chName));
      return;
    }
  }

  if (!isNewChannel && channel->getUserLimit() > 0) {
    if (channel->getMemberCount() >= channel->getUserLimit()) {
      ctx.reply(ReplyBuilder::errChannelIsFull(ctx.nick(), chName));
      return;
    }
  }

  if (!channel->hasMember(ctx.client())) {
    channel->addMember(ctx.client());

    if (isNewChannel) {
      channel->addOperator(ctx.client());
      std::cout << "[*] " << ctx.nick() << " is now the operator of " << chName
                << std::endl;
    }

    channel->removeInvite(ctx.nick());

    std::string joinMsg = ":" + ctx.prefix() + " JOIN :" + chName;
    ctx.broadcast(*channel, joinMsg);

    if (!channel->getTopic().empty()) {
      ctx.reply(ReplyBuilder::rplTopic(ctx.nick(), chName, channel->getTopic()));
    } else {
      ctx.reply(ReplyBuilder::rplNoTopic(ctx.nick(), chName));
    }

    std::string namesList = _generateChannelMemberStr(channel);
    ctx.reply(ReplyBuilder::rplNamReply(ctx.nick(), chName, namesList));
    ctx.reply(ReplyBuilder::rplEndOfNames(ctx.nick(), chName));
  }
}

// PRIVMSG <target> <message>
void Server::_handlePrivMsg(CommandContext &ctx) {
  if (ctx.params().size() < 2) {
    ctx.reply(ReplyBuilder::errNoTextToSend(ctx.nick()));
    return;
  }

  std::string target = ctx.params()[0];
  std::string text = ctx.params()[1];
  std::string fullMsg = ":" + ctx.prefix() + " PRIVMSG " + target + " :" + text;

  if (target[0] == '#') {
    Channel *channel = ctx.serverCtx().findChannel(target);
    if (channel != NULL) {
      if (channel->hasMember(ctx.client())) {
        ctx.broadcastExcept(*channel, fullMsg, ctx.client());
      } else {
        ctx.reply(ReplyBuilder::errCantSendToChannel(ctx.nick(), target));
      }
    } else {
      ctx.reply(ReplyBuilder::errNoSuchNick(ctx.nick(), target));
    }
  } else {
    Client *targetClient = ctx.serverCtx().findClientByNick(target);
    if (targetClient != NULL) {
      ctx.direct(*targetClient, fullMsg);
      return;
    }
    ctx.reply(ReplyBuilder::errNoSuchNick(ctx.nick(), target));
  }
}

// PART <channel> *( "," <channel> ) [ <Part Message> ]
void Server::_handlePart(CommandContext &ctx) {
  if (ctx.params().empty()) {
    ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "PART"));
    return;
  }

  std::string chName = ctx.params()[0];
  std::string partMsg =
      (ctx.params().size() > 1) ? ctx.params()[1] : "Leaving";

  Channel *channel = ctx.serverCtx().findChannel(chName);
  if (channel == NULL) {
    ctx.reply(ReplyBuilder::errNoSuchChannel(ctx.nick(), chName));
    return;
  }

  if (!channel->hasMember(ctx.client())) {
    ctx.reply(ReplyBuilder::errNotOnChannel(ctx.nick(), chName));
    return;
  }

  std::string fullMsg =
      ":" + ctx.prefix() + " PART " + chName + " :" + partMsg;
  ctx.broadcast(*channel, fullMsg);

  channel->removeMember(ctx.client());

  if (channel->getMemberCount() == 0) {
    ctx.serverCtx().removeChannel(chName);
    std::cout << "[-] Channel deleted (no member): " << chName << std::endl;
  }
}

// QUIT [ < Quit Message > ]
void Server::_handleQuit(CommandContext &ctx) {
  std::string reason = ctx.params().empty() ? "Client Quit" : ctx.params()[0];
  std::string quitMsg = ":" + ctx.prefix() + " QUIT :" + reason;

  _removeClientFromAllChannels(ctx.client(), quitMsg);
}

void Server::_removeClientFromAllChannels(Client &client,
                                          const std::string &quitMsg) {
  _removeClientFromAllChannels(client.getFd(), quitMsg);
}

void Server::_removeClientFromAllChannels(int fd, const std::string &quitMsg) {
  std::vector<std::string> emptyChannels;

  for (std::map<std::string, Channel *>::iterator it = _channels.begin();
       it != _channels.end(); ++it) {
    Channel *channel = it->second;
    if (channel->hasMember(fd)) {
      _responseSink.broadcastExcept(*channel, quitMsg, *_clients[fd]);
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
void Server::_handleTopic(CommandContext &ctx) {
  if (ctx.params().empty()) {
    ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "TOPIC"));
    return;
  }

  std::string chName = ctx.params()[0];

  Channel *channel = ctx.serverCtx().findChannel(chName);
  if (channel == NULL) {
    ctx.reply(ReplyBuilder::errNoSuchChannel(ctx.nick(), chName));
    return;
  }

  if (!ctx.isMemberOf(*channel)) {
    ctx.reply(ReplyBuilder::errNotOnChannel(ctx.nick(), chName));
    return;
  }

  if (ctx.params().size() == 1) {
    if (channel->getTopic().empty()) {
      ctx.reply(ReplyBuilder::rplNoTopic(ctx.nick(), chName));
    } else {
      ctx.reply(ReplyBuilder::rplTopic(ctx.nick(), chName, channel->getTopic()));
    }
    return;
  }

  std::string newTopic = ctx.params()[1];

  if (channel->isTopicProtected() && !ctx.isOperatorOf(*channel)) {
    ctx.reply(ReplyBuilder::errChanOPrivsNeeded(ctx.nick(), chName));
    return;
  }

  channel->setTopic(newTopic);

  std::string topicMsg =
      ":" + ctx.prefix() + " TOPIC " + chName + " :" + newTopic;
  ctx.broadcast(*channel, topicMsg);
}

// KICK <channel> <user> [ <comment> ]
void Server::_handleKick(CommandContext &ctx) {
  if (ctx.params().size() < 2) {
    ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "KICK"));
    return;
  }

  std::string chName = ctx.params()[0];
  std::string targetNick = ctx.params()[1];
  std::string reason =
      (ctx.params().size() > 2) ? ctx.params()[2] : "Kicked by operator";

  Channel *channel = ctx.serverCtx().findChannel(chName);
  if (channel == NULL) {
    ctx.reply(ReplyBuilder::errNoSuchChannel(ctx.nick(), chName));
    return;
  }

  if (!ctx.isMemberOf(*channel)) {
    ctx.reply(ReplyBuilder::errNotOnChannel(ctx.nick(), chName));
    return;
  }

  if (!ctx.isOperatorOf(*channel)) {
    ctx.reply(ReplyBuilder::errChanOPrivsNeeded(ctx.nick(), chName));
    return;
  }

  Client *targetClient = ctx.serverCtx().findClientByNick(targetNick);
  if (targetClient == NULL) {
    ctx.reply(ReplyBuilder::errNoSuchNick(ctx.nick(), targetNick));
    return;
  }

  if (!channel->hasMember(*targetClient)) {
    ctx.reply(ReplyBuilder::errUserNotInChannel(ctx.nick(), targetNick, chName));
    return;
  }

  std::string kickMsg = ":" + ctx.prefix() + " KICK " + chName + " " +
                        targetNick + " :" + reason;
  ctx.broadcast(*channel, kickMsg);

  channel->removeMember(*targetClient);

  if (channel->getMemberCount() == 0) {
    ctx.serverCtx().removeChannel(chName);
    std::cout << "[-] Channel deleted (no member): " << chName << std::endl;
  }
}

// INVITE <nickname> <channel>
void Server::_handleInvite(CommandContext &ctx) {
  if (ctx.params().size() < 2) {
    ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "INVITE"));
    return;
  }

  std::string targetNick = ctx.params()[0];
  std::string chName = ctx.params()[1];

  Client *targetClient = ctx.serverCtx().findClientByNick(targetNick);
  if (targetClient == NULL) {
    ctx.reply(ReplyBuilder::errNoSuchNick(ctx.nick(), targetNick));
    return;
  }

  Channel *channel = ctx.serverCtx().findChannel(chName);
  if (channel == NULL) {
    ctx.reply(ReplyBuilder::errNoSuchChannel(ctx.nick(), chName));
    return;
  }

  if (!ctx.isMemberOf(*channel)) {
    ctx.reply(ReplyBuilder::errNotOnChannel(ctx.nick(), chName));
    return;
  }

  if (channel->hasMember(*targetClient)) {
    ctx.reply(ReplyBuilder::errUserOnChannel(ctx.nick(), targetNick, chName));
    return;
  }

  if (channel->isInviteOnly() && !ctx.isOperatorOf(*channel)) {
    ctx.reply(ReplyBuilder::errChanOPrivsNeeded(ctx.nick(), chName));
    return;
  }

  channel->addInvite(targetNick);

  std::string inviteMsg =
      ":" + ctx.prefix() + " INVITE " + targetNick + " :" + chName;
  ctx.direct(*targetClient, inviteMsg);
  ctx.reply(ReplyBuilder::rplInviting(ctx.nick(), chName, targetNick));
}

void Server::_handleMode(CommandContext &ctx) {
  if (ctx.params().empty()) {
    ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "MODE"));
    return;
  }

  std::string target = ctx.params()[0];

  if (target[0] != '#')
    return;

  Channel *channel = ctx.serverCtx().findChannel(target);
  if (channel == NULL) {
    ctx.reply(ReplyBuilder::errNoSuchChannel(ctx.nick(), target));
    return;
  }

  if (ctx.params().size() == 1) {
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

    ctx.reply(ReplyBuilder::rplChannelModeIs(ctx.nick(), target, currentMode,
                                             modeParams));
    return;
  }

  if (!ctx.isOperatorOf(*channel)) {
    ctx.reply(ReplyBuilder::errChanOPrivsNeeded(ctx.nick(), target));
    return;
  }

  bool isAdding = true;
  std::string appliedModes = "";
  std::string appliedParams = "";
  char lastSign = '\0';

  std::string modeString = ctx.params()[1];

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
        if (argIdx >= ctx.params().size()) {
          ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "MODE"));
          break;
        }
        std::string key = ctx.params()[argIdx++];
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
        if (argIdx >= ctx.params().size()) {
          ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "MODE"));
          break;
        }
        std::string limitStr = ctx.params()[argIdx++];
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
      if (argIdx >= ctx.params().size()) {
        ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "MODE"));
        break;
      }
      std::string targetNick = ctx.params()[argIdx++];
      Client *targetClient = ctx.serverCtx().findClientByNick(targetNick);
      if (targetClient && channel->hasMember(*targetClient)) {
        if (isAdding != channel->isOperator(*targetClient)) {
          if (isAdding) {
            channel->addOperator(*targetClient);
          } else {
            channel->removeOperator(*targetClient);
          }
        }

        if (lastSign != (isAdding ? '+' : '-')) {
          lastSign = isAdding ? '+' : '-';
          appliedModes += lastSign;
        }
        appliedModes += "o";
        appliedParams += " " + targetNick;
      } else {
        ctx.reply(ReplyBuilder::errUserNotInChannel(ctx.nick(), targetNick,
                                                    target));
      }
    } else {
      ctx.reply(ReplyBuilder::errUnknownMode(ctx.nick(), *it, target));
    }
  }

  if (!appliedModes.empty()) {
    std::string modeMsg = ":" + ctx.prefix() + " MODE " + target + " " +
                          appliedModes + appliedParams;
    ctx.broadcast(*channel, modeMsg);
  }
}

void Server::_handlePing(CommandContext &ctx) {
  if (ctx.params().empty()) {
    ctx.reply(ReplyBuilder::errNoOrigin(ctx.nick()));
    return;
  }

  std::string token = ctx.params()[0];
  ctx.reply("PONG " + token);
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
