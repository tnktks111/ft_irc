#include "ResponseSink.hpp"
#include "Channel.hpp"
#include "Client.hpp"
#include "IrcLimits.hpp"

ResponseSink::ResponseSink() {}
ResponseSink::~ResponseSink() {}

void ResponseSink::_appendLine(Client &client, const std::string &msg) {
  // relay の際に prefix 付与や join で長くなり得るため送信側でも
  // IrcLimits::MAX_MSG_LEN で truncate してから CRLF を付け、
  // 必ず 512 byte 以内に収める (RFC 2812 §2.3)。
  if (msg.length() > IrcLimits::MAX_MSG_LEN)
    client.appendSendBuffer(msg.substr(0, IrcLimits::MAX_MSG_LEN) + "\r\n");
  else
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
