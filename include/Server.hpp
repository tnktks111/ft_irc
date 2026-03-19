#ifndef SERVER_HPP
#define SERVER_HPP

#include "Channel.hpp"
#include "Client.hpp"
#include "Message.hpp"
#include <map>
#include <poll.h>
#include <string>
#include <vector>

class Server {
private:
  int _port;
  std::string _password;
  int _serverFd;
  std::vector<struct pollfd> _pollFds;
  std::map<int, Client *> _clients;
  std::map<std::string, Channel *> _channels;

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
  void _sendMessage(int fd, const std::string &msg);
  void _updatePollEvents();

  // Commands
  // 1. configulation commands
  bool _executeCommand(Client *client, const Message &msg);
  void _handlePass(Client *client, const Message &msg);
  void _handleNick(Client *client, const Message &msg);
  void _handleUser(Client *client, const Message &msg);

  void _checkRegistration(Client *client);

  // 2. channel commands
  void _handleJoin(Client *client, const Message &msg);
  void _handlePrivMsg(Client *client, const Message &msg);
  void _handlePart(Client *client, const Message &msg);
  void _handleQuit(Client *client, const Message &msg);
  void _removeClientFromAllChannels(int fd, const std::string &quitMsg);
  void _handleTopic(Client *client, const Message &msg);
  void _handleKick(Client *client, const Message &msg);
  Client *_getClientByNickname(const std::string &nickname);
  void _handleInvite(Client *client, const Message &msg);
  void _handleMode(Client *client, const Message &msg);
  void _handlePing(Client *client, const Message &msg);

public:
  Server(int port, const std::string &password);
  ~Server();

  void start();
  //   void acceptNewClient();
  //   void handleClientData(int fd);
  //   void disconnectClient(int fd);
};

#endif
