#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <vector>

class Message {
 private:
  std::string _prefix;
  std::string _command;
  std::vector<std::string> _params;

  void _parse(const std::string& rawMessage);

 public:
  Message(const std::string& rawMessage);
  ~Message();

  const std::string& getPrefix() const;
  const std::string& getCommand() const;
  const std::vector<std::string>& getParams() const;
};

#endif
