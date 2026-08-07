#include "ResponseSink.hpp"
#include "Channel.hpp"
#include "Client.hpp"

ResponseSink::ResponseSink() : _serverName("ircserv") {}
ResponseSink::ResponseSink(const std::string &serverName)
    : _serverName(serverName) {}
ResponseSink::~ResponseSink() {}

const std::string &ResponseSink::getServerName() const {
  return _serverName;
}

void ResponseSink::_appendLine(Client &client, const std::string &msg) {
  client.appendSendBuffer(msg + "\r\n");
}

void ResponseSink::reply(Client &client, const std::string &msg) {
  // 空文字列は "空の応答" として無意味 (":ircserv \r\n" になる) ので無視。
  if (msg.empty())
    return;
  // 既に ":" prefix を持つメッセージ (稀な pre-formatted ケース) はそのまま。
  // それ以外の numeric 応答 / PONG / ERROR 等は ":<serverName> " を先付け。
  if (msg[0] == ':')
    _appendLine(client, msg);
  else
    _appendLine(client, ":" + _serverName + " " + msg);
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
