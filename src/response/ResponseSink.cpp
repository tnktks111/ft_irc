#include "ResponseSink.hpp"
#include "Channel.hpp"
#include "Client.hpp"

ResponseSink::ResponseSink() {}
ResponseSink::~ResponseSink() {}

void ResponseSink::_appendLine(Client &client, const std::string &msg) {
  client.appendSendBuffer(msg + "\r\n");
}

void ResponseSink::reply(Client &client, const std::string &msg) {
  _appendLine(client, msg);
}

void ResponseSink::direct(Client &client, const std::string &msg) {
  _appendLine(client, msg);
}

void ResponseSink::broadcast(Channel &channel, const std::string &msg) {
  const std::map<int, Client *> &members = channel.getMembers();
  for (std::map<int, Client *>::const_iterator it = members.begin();
       it != members.end(); ++it) {
    _appendLine(*(it->second), msg);
  }
}

void ResponseSink::broadcastExcept(Channel &channel, const std::string &msg,
                                   Client &excludeClient) {
  const std::map<int, Client *> &members = channel.getMembers();
  for (std::map<int, Client *>::const_iterator it = members.begin();
       it != members.end(); ++it) {
    if (it->second != &excludeClient) {
      _appendLine(*(it->second), msg);
    }
  }
}
