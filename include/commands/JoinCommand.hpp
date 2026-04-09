#ifndef JOINCOMMAND_HPP
#define JOINCOMMAND_HPP

#include <string>
#include "ACommand.hpp"
#include "ServerContext.hpp"

class JoinCommand : public ACommand {
 private:
  ServerContext& _serverCtx;

  JoinCommand(const JoinCommand& other);
  JoinCommand& operator=(const JoinCommand& other);

  static std::string _generateChannelMemberStr(const Channel& channel);

 public:
  JoinCommand(ServerContext& serverCtx);
  virtual ~JoinCommand();

  virtual bool execute(CommandContext& ctx);
};

#endif
