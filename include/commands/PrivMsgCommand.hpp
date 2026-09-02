#ifndef PRIVMSGCOMMAND_HPP
#define PRIVMSGCOMMAND_HPP

#include "ACommand.hpp"
#include "ServerContext.hpp"

class PrivMsgCommand : public ACommand {
 private:
  ServerContext& _serverCtx;

  PrivMsgCommand(const PrivMsgCommand& other);
  PrivMsgCommand& operator=(const PrivMsgCommand& other);

 public:
  PrivMsgCommand(ServerContext& serverCtx);
  virtual ~PrivMsgCommand();

  virtual bool execute(CommandContext& ctx);
};

#endif
