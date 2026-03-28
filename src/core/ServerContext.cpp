#include "ServerContext.hpp"
#include "ReplyBuilder.hpp"
#include <cctype>
#include <iostream>
#include <vector>

ServerContext::ServerContext(std::map<int, Client *> &clients,
                             std::map<std::string, Channel *> &channels,
                             ResponseSink &responseSink,
                             const std::string &password)
    : _clients(clients), _channels(channels), _responseSink(responseSink),
      _password(password) {}

ServerContext::~ServerContext() {}

char ServerContext::_normalizeNickChar(char c) {
  if (c >= 'A' && c <= 'Z') {
    return c + ('a' - 'A');
  }
  switch (c) {
  case '[':
    return '{';
  case ']':
    return '}';
  case '\\':
    return '|';
  case '~':
    return '^';
  default:
    return c;
  }
}

const std::string ServerContext::_normalizeNickStr(const std::string &name) {
  std::string normalizedName = name;
  std::transform(normalizedName.begin(), normalizedName.end(),
                 normalizedName.begin(), ServerContext::_normalizeNickChar);
  return normalizedName;
}

Client *ServerContext::findClientByNick(const std::string &nick) const {
  std::string normalizedNick = _normalizeNickStr(nick);
  for (std::map<int, Client *>::const_iterator it = _clients.begin();
       it != _clients.end(); ++it) {
    if (_normalizeNickStr(it->second->getNickName()) == normalizedNick)
      return it->second;
  }
  return NULL;
}

bool ServerContext::hasNick(const std::string &nick,
                            const Client &exceptClient) const {
  return hasNick(nick, exceptClient.getFd());
}

bool ServerContext::hasNick(const std::string &nick, int exceptFd) const {
  std::string normalizedNick = _normalizeNickStr(nick);
  for (std::map<int, Client *>::const_iterator it = _clients.begin();
       it != _clients.end(); ++it) {
    if (it->first != exceptFd &&
        _normalizeNickStr(it->second->getNickName()) == normalizedNick)
      return true;
  }
  return false;
}

Channel *ServerContext::findChannel(const std::string &name) const {
  std::map<std::string, Channel *>::const_iterator it = _channels.find(name);
  if (it != _channels.end())
    return it->second;
  return NULL;
}

ServerContext::ChannelSlot
ServerContext::getOrCreateChannel(const std::string &name) {
  std::map<std::string, Channel *>::iterator it = _channels.find(name);
  if (it != _channels.end()) {
    return ChannelSlot(it->second, false);
  } else {
    Channel *newChannel = new Channel(name);
    _channels[name] = newChannel;
    return ChannelSlot(newChannel, true);
  }
}

void ServerContext::removeChannel(const std::string &name) {
  std::map<std::string, Channel *>::iterator it = _channels.find(name);
  if (it == _channels.end())
    return;
  delete it->second;
  _channels.erase(it);
}

bool ServerContext::tryCompleteRegistration(Client &client) {
  if (client.isRegistered())
    return false;

  if (client.isPassChecked() && !client.getNickName().empty() &&
      !client.getUserName().empty()) {
    client.setRegistered(true);
    _responseSink.reply(client, ReplyBuilder::rplWelcome(client.getNickName(),
                                                         client.getPrefix()));

    std::cout << "[+] Client(FD: " << client.getFd()
              << ") has successfully logged in." << std::endl;
    return true;
  }
  return false;
}

void ServerContext::removeClientFromAllChannels(Client &client,
                                                const std::string &quitMsg) {
  std::vector<std::string> emptyChannels;

  for (std::map<std::string, Channel *>::iterator it = _channels.begin();
       it != _channels.end(); ++it) {
    Channel *channel = it->second;
    if (channel->hasMember(client)) {
      _responseSink.broadcastExcept(*channel, quitMsg, client);
      channel->removeMember(client);

      if (channel->getMemberCount() == 0)
        emptyChannels.push_back(it->first);
    }
  }

  for (std::vector<std::string>::iterator it = emptyChannels.begin();
       it != emptyChannels.end(); ++it) {
    removeChannel(*it);
    std::cout << "[-] Channel deleted (no member): " << *it << std::endl;
  }
}

ResponseSink &ServerContext::responseSink() { return _responseSink; }

const ResponseSink &ServerContext::responseSink() const {
  return _responseSink;
}

const std::string &ServerContext::password() const { return _password; }
