#ifndef CAPCOMMAND_HPP
#define CAPCOMMAND_HPP

#include "ACommand.hpp"
#include "ServerContext.hpp"

class CapCommand : public ACommand {
 private:
  ServerContext &_serverCtx;

  CapCommand(const CapCommand &other);
  CapCommand &operator=(const CapCommand &other);

 public:
  CapCommand(ServerContext &serverCtx);
  virtual ~CapCommand();

  virtual bool execute(CommandContext &ctx);
};

#endif
