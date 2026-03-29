#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <map>
#include <string>
#include <vector>
#include "Client.hpp"

class Channel {
 private:
  std::string _name;
  std::string _topic;

  std::map<int, Client*> _members;
  std::vector<int> _operators;
  std::vector<std::string> _invitedUserNicks;

  // channel mode flags
  bool _inviteOnly;       // +i
  bool _topicProtected;   // +t
  std::string _password;  // +k
  size_t _userLimit;      // +l

  Channel();
  Channel(const Channel& other);
  Channel& operator=(const Channel& other);

  static bool _isValidChannelNameChar(char c);

 public:
  Channel(const std::string& name);
  ~Channel();

  static bool isChannelPrefix(char c);
  static bool isValidChannelName(const std::string& name);

  const std::string& getName() const;
  const std::string& getTopic() const;
  void setTopic(const std::string& topic);
  const std::map<int, Client*>& getMembers() const;

  // flag getter
  bool isInviteOnly() const;
  bool isTopicProtected() const;
  const std::string& getPassword() const;
  size_t getUserLimit() const;

  // flag setter
  void setInviteOnly(bool status);
  void setTopicProtected(bool status);
  void setPassword(const std::string& password);
  void setUserLimit(size_t limit);

  // method for member, operator, and invitations.
  void addMember(Client& client);
  void addMember(Client* client);
  void removeMember(const Client& client);
  void removeMember(int fd);
  bool hasMember(const Client& client) const;
  bool hasMember(int fd) const;
  size_t getMemberCount() const;

  void addOperator(const Client& client);
  void addOperator(int fd);
  void removeOperator(const Client& client);
  void removeOperator(int fd);
  bool isOperator(const Client& client) const;
  bool isOperator(int fd) const;

  void addInvite(const std::string& nickName);
  bool isInvited(const std::string& nickName) const;
  void removeInvite(const std::string& nickName);
};

#endif
