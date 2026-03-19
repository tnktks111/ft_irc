#include "../include/Client.hpp"
#include <unistd.h>

Client::Client(int fd)
    : _fd(fd), _nickName(""), _userName(""), _recvBuffer(""), _sendBuffer(""),
      _isPassChecked(false), _isRegistered(false) {}

Client::~Client() { close(_fd); }

int Client::getFd() const { return _fd; }

void Client::appendRecvBuffer(const std::string &data) { _recvBuffer += data; }

std::string Client::extractMessage() {
  // macのncは\nしかいれてくれないので仮で
  // あとで絶対戻すうおうおうおうおうおう
  std::string::size_type pos = _recvBuffer.find("\n");
  //   std::string::size_type pos = _recvBuffer.find("\r\n");

  if (pos == std::string::npos)
    return "";

  std::string msg = _recvBuffer.substr(0, pos);
  _recvBuffer.erase(0, pos + 2);

  return msg;
}

// Getter
const std::string &Client::getNickName() const { return _nickName; }
const std::string &Client::getUserName() const { return _userName; }
bool Client::isPassChecked() const { return _isPassChecked; }
bool Client::isRegistered() const { return _isRegistered; }

// Setter
void Client::setNickName(const std::string &nickName) { _nickName = nickName; }
void Client::setUserName(const std::string &userName) { _userName = userName; }
void Client::setPassChecked(bool status) { _isPassChecked = status; }
void Client::setRegistered(bool status) { _isRegistered = status; }

std::string Client::generatePrefix() const {
  return _nickName + "!" + _userName + "@localhost";
}
