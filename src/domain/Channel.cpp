#include "Channel.hpp"
#include <algorithm>

Channel::Channel(const std::string& name)
    : _name(name),
      _topic(""),
      _inviteOnly(false),
      _topicProtected(true),
      _password(""),
      _userLimit(0) {}

Channel::~Channel() {}

// we don't support '!' prefix.
bool Channel::isChannelPrefix(char c) {
  return (c == '&' || c == '#' || c == '+');
}

// any octet except NUL, BELL, CR, LF, " ", "," and ":"
bool Channel::_isValidChannelNameChar(char c) {
  return (c != 0x00 && c != 0x07 && c != 0x0d && c != 0x0a && c != ' ' &&
          c != ',' && c != ':');
}

// we don't support ':', because we must not handle multiple servers.
bool Channel::isValidChannelName(const std::string& name) {
  if (name.empty() || name.length() > 50)
    return false;
  if (isChannelPrefix(*name.begin()) == false)
    return false;
  for (std::string::const_iterator it = name.begin(); it != name.end(); ++it) {
    if (_isValidChannelNameChar(*it) == false)
      return false;
  }
  return true;
}

const std::string& Channel::getName() const {
  return _name;
}

const std::string& Channel::getTopic() const {
  return _topic;
}

void Channel::setTopic(const std::string& topic) {
  _topic = topic;
}

const std::map<int, Client*>& Channel::getMembers() const {
  return _members;
}

// flag getter
bool Channel::isInviteOnly() const {
  return _inviteOnly;
}

bool Channel::isTopicProtected() const {
  return _topicProtected;
}

const std::string& Channel::getPassword() const {
  return _password;
}

size_t Channel::getUserLimit() const {
  return _userLimit;
}

// flag setter
void Channel::setInviteOnly(bool status) {
  _inviteOnly = status;
}

void Channel::setTopicProtected(bool status) {
  _topicProtected = status;
}

void Channel::setPassword(const std::string& password) {
  _password = password;
}

void Channel::setUserLimit(size_t limit) {
  _userLimit = limit;
}

size_t Channel::getMemberCount() const {
  return _members.size();
}

void Channel::addMember(Client& client) {
  addMember(&client);
}

void Channel::addMember(Client* client) {
  _members.insert(std::make_pair(client->getFd(), client));
}

void Channel::removeMember(const Client& client) {
  removeMember(client.getFd());
}

void Channel::removeMember(int fd) {
  _members.erase(fd);
  removeOperator(fd);
}

bool Channel::hasMember(const Client& client) const {
  return hasMember(client.getFd());
}

bool Channel::hasMember(int fd) const {
  return _members.find(fd) != _members.end();
}

void Channel::addOperator(const Client& client) {
  addOperator(client.getFd());
}

void Channel::addOperator(int fd) {
  if (!isOperator(fd)) {
    _operators.push_back(fd);
  }
}

void Channel::removeOperator(const Client& client) {
  removeOperator(client.getFd());
}

void Channel::removeOperator(int fd) {
  std::vector<int>::iterator it =
      std::find(_operators.begin(), _operators.end(), fd);
  if (it != _operators.end()) {
    _operators.erase(it);
  }
}

bool Channel::isOperator(const Client& client) const {
  return isOperator(client.getFd());
}

bool Channel::isOperator(int fd) const {
  return std::find(_operators.begin(), _operators.end(), fd) !=
         _operators.end();
}

void Channel::addInvite(const std::string& nickName) {
  if (!isInvited(nickName)) {
    _invitedUserNicks.push_back(nickName);
  }
}

bool Channel::isInvited(const std::string& nickName) const {
  return std::find(_invitedUserNicks.begin(), _invitedUserNicks.end(),
                   nickName) != _invitedUserNicks.end();
}

void Channel::removeInvite(const std::string& nickName) {
  std::vector<std::string>::iterator it =
      std::find(_invitedUserNicks.begin(), _invitedUserNicks.end(), nickName);
  if (it != _invitedUserNicks.end()) {
    _invitedUserNicks.erase(it);
  }
}
