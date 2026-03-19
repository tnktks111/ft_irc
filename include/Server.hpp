#ifndef SERVER_HPP
#define SERVER_HPP

#include "Channel.hpp"
#include "Client.hpp"
#include "Message.hpp"
#include "ResponseSink.hpp"
#include <map>
#include <poll.h>
#include <string>
#include <vector>

class CommandContext;

class Server {
private:
  int _port;
  std::string _password;
  int _serverFd;
  std::vector<struct pollfd> _pollFds;
  std::map<int, Client *> _clients;
  std::map<std::string, Channel *> _channels;
  ResponseSink _responseSink;

  enum ConnectionStatus { KEEP_ALIVE, DISCONNECT };

  Server();
  Server(const Server &other);
  Server &operator=(const Server &other);
  void _setupServerSocket();
  void _setNonBlocking(int fd);
  void _waitForEvents();
  void _processActiveConnections();
  void _acceptNewConnection();
  ConnectionStatus _handleClientMessage(struct pollfd &clientPollFd);
  void _updatePollEvents();

  // Commands
  // 1. configulation commands
  bool _executeCommand(Client *client, const Message &msg);
  void _handlePass(CommandContext &ctx);
  void _handleNick(CommandContext &ctx);
  void _handleUser(CommandContext &ctx);

  void _checkRegistration(Client *client);

  // 2. channel commands
  static std::string _generateChannelMemberStr(const Channel *channel);
  void _handleJoin(CommandContext &ctx);
  void _handlePrivMsg(CommandContext &ctx);
  void _handlePart(CommandContext &ctx);
  void _handleQuit(CommandContext &ctx);
  void _removeClientFromAllChannels(Client &client, const std::string &quitMsg);
  void _removeClientFromAllChannels(int fd, const std::string &quitMsg);
  void _handleTopic(CommandContext &ctx);
  void _handleKick(CommandContext &ctx);
  void _handleInvite(CommandContext &ctx);
  void _handleMode(CommandContext &ctx);
  void _handlePing(CommandContext &ctx);

public:
  Server(int port, const std::string &password);
  ~Server();

  void start();
  //   void acceptNewClient();
  //   void handleClientData(int fd);
  //   void disconnectClient(int fd);
};

#endif
