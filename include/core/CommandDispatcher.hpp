#ifndef COMMANDDISPATCHER_HPP
#define COMMANDDISPATCHER_HPP

#include <map>
#include <string>
#include "ACommand.hpp"
#include "ServerContext.hpp"

class CommandDispatcher {
 private:
  ServerContext& _serverCtx;
  std::map<std::string, ACommand*> _commands;

  CommandDispatcher();
  CommandDispatcher(const CommandDispatcher& other);
  CommandDispatcher& operator=(const CommandDispatcher& other);

  bool _isPreRegistrationCommand(const std::string& command) const;

 public:
  CommandDispatcher(ServerContext& serverCtx);
  ~CommandDispatcher();

  bool dispatch(CommandContext& ctx);
};

#endif
