#include "ServerContext.hpp"
#include <algorithm>
#include <cctype>
#include <ctime>
#include <iostream>
#include <vector>
#include "HostCaseMapping.hpp"
#include "IrcCaseMapping.hpp"
#include "ReplyBuilder.hpp"

namespace {
std::string formatCreatedAt() {
  std::time_t now = std::time(NULL);
  std::tm* tm = std::localtime(&now);
  if (tm == NULL)
    return std::string("unknown");
  char buf[64];
  std::size_t n = std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z", tm);
  if (n == 0)
    return std::string("unknown");
  std::string result(buf, n);
  while (!result.empty() && result[result.size() - 1] == ' ')
    result.erase(result.size() - 1);
  return result;
}
}  // namespace

ServerContext::ServerContext(std::map<int, Client*>& clients,
                             std::map<std::string, Channel*>& channels,
                             ResponseSink& responseSink,
                             const std::string& password)
    : _clients(clients),
      _channels(channels),
      _responseSink(responseSink),
      _password(password),
      _createdAt(formatCreatedAt()) {}

ServerContext::~ServerContext() {}

Client* ServerContext::findClientByNick(const std::string& nick) const {
  for (std::map<int, Client*>::const_iterator it = _clients.begin();
       it != _clients.end(); ++it) {
    if (IrcCaseMapping::equals(it->second->getNickName(), nick))
      return it->second;
  }
  return NULL;
}

std::vector<Client*> ServerContext::findClientsByUserName(
    const std::string& userName) const {
  std::vector<Client*> result;

  for (std::map<int, Client*>::const_iterator it = _clients.begin();
       it != _clients.end(); ++it) {
    if (it->second->getUserName() == userName)
      result.push_back(it->second);
  }
  return result;
}

std::vector<Client*> ServerContext::findClientsByUserHost(
    const std::string& userName, const std::string& host) const {
  std::vector<Client*> result;

  for (std::map<int, Client*>::const_iterator it = _clients.begin();
       it != _clients.end(); ++it) {
    if (it->second->getUserName() == userName &&
        HostCaseMapping::equals(it->second->getHost(), host))
      result.push_back(it->second);
  }
  return result;
}

Client* ServerContext::findClientByNickMask(const std::string& nick,
                                            const std::string& userName,
                                            const std::string& host) const {

  for (std::map<int, Client*>::const_iterator it = _clients.begin();
       it != _clients.end(); ++it) {
    if (IrcCaseMapping::equals(it->second->getNickName(), nick) &&
        it->second->getUserName() == userName &&
        HostCaseMapping::equals(it->second->getHost(), host))
      return it->second;
  }
  return NULL;
}

bool ServerContext::hasNick(const std::string& nick,
                            const Client& exceptClient) const {
  return hasNick(nick, exceptClient.getFd());
}

bool ServerContext::hasNick(const std::string& nick, int exceptFd) const {
  for (std::map<int, Client*>::const_iterator it = _clients.begin();
       it != _clients.end(); ++it) {
    if (it->first != exceptFd &&
        IrcCaseMapping::equals(it->second->getNickName(), nick))
      return true;
  }
  return false;
}

Channel* ServerContext::findChannel(const std::string& name) const {
  std::map<std::string, Channel*>::const_iterator it =
      _channels.find(IrcCaseMapping::normalize(name));
  if (it != _channels.end())
    return it->second;
  return NULL;
}

ServerContext::ChannelSlot ServerContext::getOrCreateChannel(
    const std::string& name) {
  std::string normalizedName = IrcCaseMapping::normalize(name);
  std::map<std::string, Channel*>::iterator it = _channels.find(normalizedName);
  if (it != _channels.end()) {
    return ChannelSlot(it->second, false);
  } else {
    Channel* newChannel = new Channel(name);
    _channels[normalizedName] = newChannel;
    return ChannelSlot(newChannel, true);
  }
}

void ServerContext::removeChannel(const std::string& name) {
  std::map<std::string, Channel*>::iterator it =
      _channels.find(IrcCaseMapping::normalize(name));
  if (it == _channels.end())
    return;
  delete it->second;
  _channels.erase(it);
}

bool ServerContext::tryCompleteRegistration(Client& client) {
  if (client.isRegistered())
    return false;

  if (client.isPassChecked() && !client.getNickName().empty() &&
      !client.getUserName().empty()) {
    client.setRegistered(true);

    const std::string& nick = client.getNickName();
    const std::string& serverName = _responseSink.getServerName();
    const std::string version = "0.1";
    const std::string umodes = "iwoOars";
    const std::string cmodes = "itklo";

    _responseSink.reply(client,
                        ReplyBuilder::rplWelcome(nick, client.getPrefix()));
    _responseSink.reply(client,
                        ReplyBuilder::rplYourHost(nick, serverName, version));
    _responseSink.reply(client, ReplyBuilder::rplCreated(nick, _createdAt));
    _responseSink.reply(
        client, ReplyBuilder::rplMyInfo(nick, serverName, version, umodes,
                                        cmodes));

    std::cout << "[+] Client(FD: " << client.getFd()
              << ") has successfully logged in." << std::endl;
    return true;
  }
  return false;
}

void ServerContext::leaveAllChannels(Client& client) {
  std::vector<std::string> emptyChannels;

  for (std::map<std::string, Channel*>::iterator it = _channels.begin();
       it != _channels.end(); ++it) {
    Channel* channel = it->second;
    if (channel->hasMember(client)) {
      std::string partMsg =
          ":" + client.getPrefix() + " PART " + channel->getName();
      _responseSink.broadcast(*channel, partMsg);
      channel->removeMember(client);

      if (channel->getMemberCount() == 0)
        emptyChannels.push_back(channel->getName());
    }
  }

  for (std::vector<std::string>::iterator it = emptyChannels.begin();
       it != emptyChannels.end(); ++it) {
    removeChannel(*it);
    std::cout << "[-] Channel deleted (no member): " << *it << std::endl;
  }
}

void ServerContext::removeClientFromAllChannels(Client& client,
                                                const std::string& quitMsg) {
  std::vector<std::string> emptyChannels;

  for (std::map<std::string, Channel*>::iterator it = _channels.begin();
       it != _channels.end(); ++it) {
    Channel* channel = it->second;
    if (channel->hasMember(client)) {
      _responseSink.broadcastExcept(*channel, quitMsg, client);
      channel->removeMember(client);

      if (channel->getMemberCount() == 0)
        emptyChannels.push_back(channel->getName());
    }
  }

  for (std::vector<std::string>::iterator it = emptyChannels.begin();
       it != emptyChannels.end(); ++it) {
    removeChannel(*it);
    std::cout << "[-] Channel deleted (no member): " << *it << std::endl;
  }
}

ResponseSink& ServerContext::responseSink() {
  return _responseSink;
}

const ResponseSink& ServerContext::responseSink() const {
  return _responseSink;
}

const std::string& ServerContext::password() const {
  return _password;
}

const std::string& ServerContext::createdAt() const {
  return _createdAt;
}
