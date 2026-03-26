#ifndef SERVER_HPP
#define SERVER_HPP

#include "Channel.hpp"
#include "Client.hpp"
#include "CommandDispatcher.hpp"
#include "Message.hpp"
#include "ResponseSink.hpp"
#include "ServerContext.hpp"
#include <csignal>
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

  static volatile sig_atomic_t _shouldStop;

  enum ConnectionStatus { KEEP_ALIVE, DISCONNECT };
  enum LoopAction { CONTINUE_LOOP, KEEP_PROCESSING };

  Server();
  Server(const Server &other);
  Server &operator=(const Server &other);

  static std::string _buildErrnoMessage(const std::string &context);
  static std::string _buildFdErrnoMessage(const std::string &context, int fd);

  void _setupServerSocket();
  void _setNonBlocking(int fd);
  bool _waitForEvents();
  void _registerClient(int clientFd, const std::string &host);
  void _disconnectClient(size_t &index, const std::string &quitMsg);
  std::string _makeQuitMessage(const Client &client, const std::string &reason) const;

  void _processPollError(size_t &index);
  LoopAction _processReadableEvent(size_t &index);
  void _flushSendBuffer(size_t index);

  void _processActiveConnections();
  void _acceptNewConnection();
  ConnectionStatus _handleClientMessage(struct pollfd &clientPollFd);
  void _updatePollEvents();

  bool _executeCommand(Client *client, const Message &msg);

  static void _handleSignal(int signo);

public:
  Server(int port, const std::string &password);
  ~Server();

  void start();

  static void setupSignalHandlers();
};

#endif
