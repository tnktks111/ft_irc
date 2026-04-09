#ifndef WHOCOMMAND_HPP
#define WHOCOMMAND_HPP

#include "ACommand.hpp"
#include "ServerContext.hpp"

class WhoCommand : public ACommand {
 private:
  ServerContext& _serverCtx;

  WhoCommand(const WhoCommand& other);
  WhoCommand& operator=(const WhoCommand& other);

 public:
  WhoCommand(ServerContext& serverCtx);
  virtual ~WhoCommand();

  virtual bool execute(CommandContext& ctx);
};

#endif
