#ifndef WHOISCOMMAND_HPP
#define WHOISCOMMAND_HPP

#include "ACommand.hpp"
#include "ServerContext.hpp"

class WhoisCommand : public ACommand {
 private:
  ServerContext &_serverCtx;

  WhoisCommand(const WhoisCommand &other);
  WhoisCommand &operator=(const WhoisCommand &other);

 public:
  WhoisCommand(ServerContext &serverCtx);
  virtual ~WhoisCommand();

  virtual bool execute(CommandContext &ctx);
};

#endif