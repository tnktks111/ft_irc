#include "Client.hpp"
#include <unistd.h>

Client::Client(int fd, const std::string& host)
    : _fd(fd),
      _host(host),
      _nickName(""),
      _userName(""),
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
void Client::setPassChecked(bool status) {
  _isPassChecked = status;
}
void Client::setRegistered(bool status) {
  _isRegistered = status;
}

std::string Client::getPrefix() const {
  return _nickName + "!" + _userName + "@" + _host;
}
