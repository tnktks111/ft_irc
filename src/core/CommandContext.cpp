#include "CommandContext.hpp"

CommandContext::CommandContext(Client& client, const Message& message,
                               ServerContext& serverCtx)
    : _client(client), _message(message), _serverCtx(serverCtx) {}

CommandContext::~CommandContext() {}

Client& CommandContext::client() {
  return _client;
}
const Client& CommandContext::client() const {
  return _client;
}

const Message& CommandContext::message() const {
  return _message;
}

ServerContext& CommandContext::serverCtx() {
  return _serverCtx;
}
const ServerContext& CommandContext::serverCtx() const {
  return _serverCtx;
}

const std::string& CommandContext::command() const {
  return _message.getCommand();
}
const std::vector<std::string>& CommandContext::params() const {
  return _message.getParams();
}

const std::string& CommandContext::nick() const {
  return _client.getNickName();
}
const std::string& CommandContext::user() const {
  return _client.getUserName();
}
std::string CommandContext::prefix() const {
  return _client.getPrefix();
}
bool CommandContext::isRegistered() const {
  return _client.isRegistered();
}
bool CommandContext::isMemberOf(const Channel& channel) const {
  return channel.hasMember(_client);
}
bool CommandContext::isOperatorOf(const Channel& channel) const {
  return channel.isOperator(_client);
}

void CommandContext::reply(const std::string& msg) {
  _serverCtx.responseSink().reply(_client, msg);
}
void CommandContext::direct(Client& target, const std::string& msg) {
  _serverCtx.responseSink().direct(target, msg);
}
void CommandContext::broadcast(Channel& channel, const std::string& msg) {
  _serverCtx.responseSink().broadcast(channel, msg);
}
void CommandContext::broadcastExcept(Channel& channel, const std::string& msg,
                                     Client& excludeClient) {
  _serverCtx.responseSink().broadcastExcept(channel, msg, excludeClient);
}
