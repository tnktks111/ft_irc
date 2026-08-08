#ifndef PINGCOMMAND_HPP
#define PINGCOMMAND_HPP

#include "ACommand.hpp"
#include "ServerContext.hpp"

class PingCommand : public ACommand {
 private:
  ServerContext& _serverCtx;

  PingCommand(const PingCommand& other);
  PingCommand& operator=(const PingCommand& other);

 public:
  PingCommand(ServerContext& serverCtx);
  virtual ~PingCommand();

  virtual bool execute(CommandContext& ctx);
};

#endif
