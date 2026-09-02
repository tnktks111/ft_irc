#include "ResponseSink.hpp"
#include "Channel.hpp"
#include "Client.hpp"
#include "IrcLimits.hpp"

ResponseSink::ResponseSink() : _serverName("ircserv") {}
ResponseSink::ResponseSink(const std::string& serverName)
    : _serverName(serverName) {}
ResponseSink::~ResponseSink() {}

const std::string& ResponseSink::getServerName() const {
  return _serverName;
}

void ResponseSink::_appendLine(Client& client, const std::string& msg) {
  if (msg.length() > IrcLimits::MAX_MSG_LEN)
    client.appendSendBuffer(msg.substr(0, IrcLimits::MAX_MSG_LEN) + "\r\n");
  else
    client.appendSendBuffer(msg + "\r\n");
}

void ResponseSink::reply(Client& client, const std::string& msg) {
  if (msg.empty())
    return;
  if (msg[0] == ':')
    _appendLine(client, msg);
  else
    _appendLine(client, ":" + _serverName + " " + msg);
}

void ResponseSink::direct(Client& client, const std::string& msg) {
  _appendLine(client, msg);
}

void ResponseSink::broadcast(Channel& channel, const std::string& msg) {
  const std::map<int, Client*>& members = channel.getMembers();
  for (std::map<int, Client*>::const_iterator it = members.begin();
       it != members.end(); ++it) {
    _appendLine(*(it->second), msg);
  }
}

void ResponseSink::broadcastExcept(Channel& channel, const std::string& msg,
                                   Client& excludeClient) {
  const std::map<int, Client*>& members = channel.getMembers();
  for (std::map<int, Client*>::const_iterator it = members.begin();
       it != members.end(); ++it) {
    if (it->second != &excludeClient) {
      _appendLine(*(it->second), msg);
    }
  }
}
