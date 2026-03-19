#include "../include/Message.hpp"
#include <sstream>
Message::Message(const std::string &rawMessage) { _parse(rawMessage); }
Message::~Message() {}

void Message::_parse(const std::string &rawMessage) {
  if (rawMessage.empty())
    return;

  std::string msg = rawMessage;

  if (msg[0] == ':') {
    std::string::size_type spacePos = rawMessage.find(' ');
    if (spacePos != std::string::npos) {
      _prefix = msg.substr(1, spacePos - 1);
      msg = msg.substr(spacePos + 1);
    } else {
      _prefix = msg.substr(1);
      return;
    }
  }

  std::string::size_type startPos = msg.find_first_not_of(' ');
  if (startPos != std::string::npos) {
    msg = msg.substr(startPos);
  }

  std::string::size_type trailingPos = msg.find(" :");
  std::string trailing = "";
  bool hasTrailing = false;

  if (msg[0] == ':') {
    trailing = msg.substr(1);
    msg = "";
    hasTrailing = true;
  } else if (trailingPos != std::string::npos) {
    trailing = msg.substr(trailingPos + 2);
    msg = msg.substr(0, trailingPos);
    hasTrailing = true;
  }

  std::istringstream iss(msg);
  if (!(iss >> _command))
    return;

  std::string param;
  while (iss >> param) {
    _params.push_back(param);
  }

  if (hasTrailing) {
    _params.push_back(trailing);
  }
}

const std::string &Message::getPrefix() const { return _prefix; }
const std::string &Message::getCommand() const { return _command; }
const std::vector<std::string> &Message::getParams() const { return _params; }
