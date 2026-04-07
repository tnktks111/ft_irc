#ifndef SERVERCONTEXT_HPP
#define SERVERCONTEXT_HPP

#include <map>
#include <string>
#include <utility>
#include "Channel.hpp"
#include "Client.hpp"
#include "ResponseSink.hpp"

class ServerContext {

 private:
  ServerContext();
  ServerContext(const ServerContext& other);
  ServerContext& operator=(const ServerContext& other);

  std::map<int, Client*>& _clients;
  std::map<std::string, Channel*>& _channels;
  ResponseSink& _responseSink;
  const std::string& _password;

 public:
  typedef std::pair<Channel*, bool> ChannelSlot;
  ServerContext(std::map<int, Client*>& clients,
                std::map<std::string, Channel*>& channels,
                ResponseSink& responseSink, const std::string& password);
  ~ServerContext();

  Client* findClientByNick(const std::string& nick) const;
  std::vector<Client*> findClientsByUserName(const std::string& userName) const;
  std::vector<Client*> findClientsByUserHost(const std::string& userName,
                                             const std::string& host) const;
  Client* findClientByNickMask(const std::string& nick,
                               const std::string& userName,
                               const std::string& host) const;

  bool hasNick(const std::string& nick, const Client& exceptClient) const;
  bool hasNick(const std::string& nick, int exceptFd) const;

  Channel* findChannel(const std::string& name) const;
  ChannelSlot getOrCreateChannel(const std::string& name);
  void removeChannel(const std::string& name);
  bool tryCompleteRegistration(Client& client);
  void leaveAllChannels(Client& client);
  void removeClientFromAllChannels(Client& client, const std::string& quitMsg);

  ResponseSink& responseSink();
  const ResponseSink& responseSink() const;
  const std::string& password() const;
};

#endif
