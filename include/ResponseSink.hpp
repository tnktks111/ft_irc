#ifndef RESPONSESINK_HPP
#define RESPONSESINK_HPP

#include <string>

class Client;
class Channel;

class ResponseSink {
private:
  ResponseSink(const ResponseSink &other);
  ResponseSink &operator=(const ResponseSink &other);

  void _appendLine(Client &client, const std::string &msg);

public:
  ResponseSink();
  ~ResponseSink();

  void reply(Client &client, const std::string &msg);
  void direct(Client &client, const std::string &msg);
  void broadcast(Channel &channel, const std::string &msg);
  void broadcastExcept(Channel &channel, const std::string &msg,
                       Client &excludeClient);
};

#endif
