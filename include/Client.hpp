#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
private:
  Client();
  Client(const Client &other);
  Client &operator=(const Client &other);

  int _fd;
  std::string _nickName;
  std::string _userName;

  std::string _recvBuffer;
  std::string _sendBuffer;

  bool _isPassChecked;
  bool _isRegistered;

public:
  Client(int fd);
  ~Client();

  int getFd() const;
  void appendRecvBuffer(const std::string &data);
  std::string extractMessage();

  // sendBuffer
  void appendSendBuffer(const std::string &msg);
  std::string &getSendBuffer();
  void eraseSendBuffer(size_t length);

  // Getter
  const std::string &getNickName() const;
  const std::string &getUserName() const;
  bool isPassChecked() const;
  bool isRegistered() const;
  std::string getMembersString() const;

  // Setter
  void setNickName(const std::string &nickName);
  void setUserName(const std::string &userName);
  void setPassChecked(bool status);
  void setRegistered(bool status);

  std::string getPrefix() const;
};

#endif
