#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

enum UserMode {
  UMODE_NONE = 0,
  UMODE_AWAY = 1 << 0,   // a - away
  UMODE_INVIS = 1 << 1,  // i - invisible
  UMODE_WALLO = 1 << 2,  // w - wallops
  UMODE_RESTR = 1 << 3,  // r - restricted
  UMODE_OPER = 1 << 4,   // o - global operator
  UMODE_LOPER = 1 << 5,  // O - local operator
  UMODE_SNICE = 1 << 6   // s - server notices
};

class Client {
 private:
  Client();
  Client(const Client& other);
  Client& operator=(const Client& other);

  int _fd;
  const std::string _host;
  std::string _nickName;
  std::string _userName;
  std::string _realName;
  unsigned int _mode;

  std::string _recvBuffer;
  std::string _sendBuffer;

  bool _isPassChecked;
  bool _isRegistered;

  // validation helper
  static bool _isLetter(char c);
  static bool _isDigit(char c);
  static bool _isSpecial(char c);
  static bool _isProhibittedCharInUserName(char c);
  static bool _isShortName(const std::string& token);

 public:
  Client(int fd, const std::string& host);
  ~Client();

  // validator
  static bool isValidNickName(const std::string& token);
  static bool isValidUserName(const std::string& token);
  static bool isValidHostName(const std::string& token);
  static bool isValidHostAddr(const std::string& token);
  static bool isValidHost(const std::string& token);

  int getFd() const;
  void appendRecvBuffer(const std::string& data);
  std::string extractMessage();

  // sendBuffer
  void appendSendBuffer(const std::string& msg);
  std::string& getSendBuffer();
  void eraseSendBuffer(size_t length);

  // Getter
  const std::string& getNickName() const;
  const std::string& getUserName() const;
  const std::string& getRealName() const;
  const std::string& getHost() const;
  bool isPassChecked() const;
  bool isRegistered() const;

  // Setter
  void setNickName(const std::string& nickName);
  void setUserName(const std::string& userName);
  void setRealName(const std::string& realName);
  void addMode(UserMode mode);
  void setPassChecked(bool status);
  void setRegistered(bool status);
  std::string getPrefix() const;
};

#endif
