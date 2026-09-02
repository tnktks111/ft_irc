#ifndef COMMANDCONTEXT_HPP
#define COMMANDCONTEXT_HPP

#include <string>
#include <vector>
#include "Client.hpp"
#include "Message.hpp"
#include "ServerContext.hpp"

class CommandContext {
 private:
  Client& _client;
  const Message& _message;
  ServerContext& _serverCtx;

  CommandContext();
  CommandContext(const CommandContext& other);
  CommandContext& operator=(const CommandContext& other);

 public:
  CommandContext(Client& client, const Message& message,
                 ServerContext& serverCtx);
  ~CommandContext();

  Client& client();
  const Client& client() const;

  const Message& message() const;

  ServerContext& serverCtx();
  const ServerContext& serverCtx() const;

  const std::string& command() const;
  const std::vector<std::string>& params() const;

  const std::string& nick() const;
  const std::string& user() const;
  std::string prefix() const;
  bool isRegistered() const;
  bool isMemberOf(const Channel& channel) const;
  bool isOperatorOf(const Channel& channel) const;

  void reply(const std::string& msg);
  void direct(Client& target, const std::string& msg);
  void broadcast(Channel& channel, const std::string& msg);
  void broadcastExcept(Channel& channel, const std::string& msg,
                       Client& excludeClient);
};

#endif
