#ifndef SERVER_HPP
#define SERVER_HPP

#include "Channel.hpp"
#include "Client.hpp"
#include "CommandDispatcher.hpp"
#include "Message.hpp"
#include "ResponseSink.hpp"
#include "ServerContext.hpp"
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
  ResponseSink _responseSink;
  ServerContext _serverCtx;
  CommandDispatcher _dispatcher;

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

  bool _executeCommand(Client *client, const Message &msg);

public:
  Server(int port, const std::string &password);
  ~Server();

  void start();
  //   void acceptNewClient();
  //   void handleClientData(int fd);
  //   void disconnectClient(int fd);
};

#endif
