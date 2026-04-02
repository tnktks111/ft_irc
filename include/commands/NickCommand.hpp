#ifndef NICKCOMMAND_HPP
#define NICKCOMMAND_HPP

#include "ACommand.hpp"
#include "ServerContext.hpp"

class NickCommand : public ACommand {
 private:
  ServerContext& _serverCtx;

  NickCommand(const NickCommand& other);
  NickCommand& operator=(const NickCommand& other);

 public:
  NickCommand(ServerContext& serverCtx);
  virtual ~NickCommand();

  virtual bool execute(CommandContext& ctx);
};

#endif
