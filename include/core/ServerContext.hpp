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
  std::string _createdAt;

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
  // Deliver `msg` to `client` itself and to every other member that shares
  // at least one channel with `client`, each exactly once (deduped by fd).
  // Used by NICK/QUIT-style notifications where the same event must be
  // observed by anyone who "sees" this client on the network.
  void broadcastToVisibleMembers(Client& client, const std::string& msg);

  ResponseSink& responseSink();
  const ResponseSink& responseSink() const;
  const std::string& password() const;
  const std::string& createdAt() const;
};

#endif
