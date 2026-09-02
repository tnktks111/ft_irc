#ifndef PASSCOMMAND_HPP
#define PASSCOMMAND_HPP

#include "ACommand.hpp"
#include "ServerContext.hpp"

class PassCommand : public ACommand {
 private:
  ServerContext& _serverCtx;

  PassCommand(const PassCommand& other);
  PassCommand& operator=(const PassCommand& other);

 public:
  PassCommand(ServerContext& serverCtx);
  virtual ~PassCommand();

  virtual bool execute(CommandContext& ctx);
};

#endif
