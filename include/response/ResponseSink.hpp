#ifndef RESPONSESINK_HPP
#define RESPONSESINK_HPP

#include <string>

class Client;
class Channel;

class ResponseSink {
private:
  std::string _serverName;

  ResponseSink(const ResponseSink &other);
  ResponseSink &operator=(const ResponseSink &other);

  void _appendLine(Client &client, const std::string &msg);

public:
  ResponseSink();
  explicit ResponseSink(const std::string &serverName);
  ~ResponseSink();

  const std::string &getServerName() const;

  // reply() は「サーバー発の応答」用。msg が ":" prefix を持たない場合
  // 自動的に ":<serverName> " を先頭に付与する (RFC 2812 §2.3)。
  void reply(Client &client, const std::string &msg);
  // direct/broadcast は「他 client の行為を中継」用。msg は既に
  // ":nick!user@host CMD ..." 形式で来る前提のため prefix は付与しない。
  void direct(Client &client, const std::string &msg);
  void broadcast(Channel &channel, const std::string &msg);
  void broadcastExcept(Channel &channel, const std::string &msg,
                       Client &excludeClient);
};

#endif
