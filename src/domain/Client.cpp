#include "Client.hpp"
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>

Client::Client(int fd, const std::string& host)
    : _fd(fd),
      _host(host),
      _nickName(""),
      _userName(""),
      _realName(""),
      _mode(0),
      _recvBuffer(""),
      _sendBuffer(""),
      _isPassChecked(false),
      _isRegistered(false) {}

Client::~Client() {
  close(_fd);
}

// validation helper
bool Client::_isLetter(char c) {
  return ((0x41 <= c && c <= 0x5a) || (0x61 <= c && c <= 0x7a));
}

bool Client::_isDigit(char c) {
  return (0x30 <= c && c <= 0x39);
}

bool Client::_isSpecial(char c) {
  return ((0x5b <= c && c <= 0x60) || (0x7b <= c && c <= 0x7d));
}

bool Client::isValidNickName(const std::string& token) {
  if (token.length() > 9 || token.empty())
    return false;
  if (_isLetter(*token.begin()) == false && _isSpecial(*token.begin()) == false)
    return false;
  for (std::string::const_iterator it = token.begin() + 1; it != token.end();
       ++it) {
    if (!_isLetter(*it) && !_isDigit(*it) && !_isSpecial(*it) && *it != '-')
      return false;
  }
  return true;
}

bool Client::_isProhibittedCharInUserName(char c) {
  return (c == 0x00 || c == 0x0D || c == 0x0A || c == ' ' || c == '@');
}

bool Client::isValidUserName(const std::string& token) {
  for (std::string::const_iterator it = token.begin(); it != token.end();
       ++it) {
    if (_isProhibittedCharInUserName(*it))
      return false;
  }
  return true;
}

bool Client::_isShortName(const std::string& token) {
  if (token.empty() || token.length() > 63)
    return false;
  for (std::string::const_iterator it = token.begin(); it != token.end();
       ++it) {
    if (!_isLetter(*it) && !_isDigit(*it) && *it != '-')
      return false;
  }
  return (*token.begin() != '-' && *token.rbegin() != '-');
}

bool Client::isValidHostName(const std::string& token) {
  if (token.empty() || token.length() > 255)
    return false;
  if (*token.rbegin() == '.')
    return false;

  std::istringstream iss(token);
  std::string item;
  while (std::getline(iss, item, '.')) {
    if (_isShortName(item) == false)
      return false;
  }
  return true;
}

bool Client::isValidHostAddr(const std::string& token) {
  struct in_addr ipv4_addr;
  struct in6_addr ipv6_addr;

  if (inet_pton(AF_INET, token.c_str(), &ipv4_addr) == 1)
    return true;

  if (inet_pton(AF_INET6, token.c_str(), &ipv6_addr) == 1)
    return true;

  return false;
}

bool Client::isValidHost(const std::string& token) {
  return (isValidHostName(token) || isValidHostAddr(token));
}

int Client::getFd() const {
  return _fd;
}

void Client::appendRecvBuffer(const std::string& data) {
  _recvBuffer += data;
}

std::string Client::extractMessage() {
  // macのncは\nしかいれてくれないので仮で
  // あとで絶対戻すうおうおうおうおうおう
  std::string::size_type pos = _recvBuffer.find("\n");
  //   std::string::size_type pos = _recvBuffer.find("\r\n");

  if (pos == std::string::npos)
    return "";

  std::string msg = _recvBuffer.substr(0, pos);

  _recvBuffer.erase(0, pos + 1);
  // ここも戻すうおうおうおう
  // _recvBuffer.erase(0, pos + 2);

  return msg;
}

// sendBuffer
void Client::appendSendBuffer(const std::string& msg) {
  _sendBuffer += msg;
}
std::string& Client::getSendBuffer() {
  return _sendBuffer;
}
void Client::eraseSendBuffer(size_t length) {
  _sendBuffer.erase(0, length);
}

// Getter
const std::string& Client::getNickName() const {
  return _nickName;
}
const std::string& Client::getUserName() const {
  return _userName;
}
const std::string& Client::getRealName() const {
  return _realName;
}
const std::string& Client::getHost() const {
  return _host;
}
bool Client::isPassChecked() const {
  return _isPassChecked;
}
bool Client::isRegistered() const {
  return _isRegistered;
}

// Setter
void Client::setNickName(const std::string& nickName) {
  _nickName = nickName;
}
void Client::setUserName(const std::string& userName) {
  _userName = userName;
}
void Client::setRealName(const std::string& realName) {
  _realName = realName;
}
void Client::addMode(UserMode mode) {
  _mode |= static_cast<unsigned int>(mode);
}
void Client::setPassChecked(bool status) {
  _isPassChecked = status;
}
void Client::setRegistered(bool status) {
  _isRegistered = status;
}

std::string Client::getPrefix() const {
  return _nickName + "!" + _userName + "@" + _host;
}
