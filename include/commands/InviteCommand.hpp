#ifndef INVITECOMMAND_HPP
#define INVITECOMMAND_HPP

#include "ACommand.hpp"
#include "ServerContext.hpp"

class InviteCommand : public ACommand {
 private:
  ServerContext& _serverCtx;

  InviteCommand(const InviteCommand& other);
  InviteCommand& operator=(const InviteCommand& other);

 public:
  InviteCommand(ServerContext& serverCtx);
  virtual ~InviteCommand();

  virtual bool execute(CommandContext& ctx);
};

#endif
