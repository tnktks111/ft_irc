#ifndef QUITCOMMAND_HPP
#define QUITCOMMAND_HPP

#include "ACommand.hpp"
#include "ServerContext.hpp"

class QuitCommand : public ACommand {
 private:
  ServerContext& _serverCtx;

  QuitCommand(const QuitCommand& other);
  QuitCommand& operator=(const QuitCommand& other);

 public:
  QuitCommand(ServerContext& serverCtx);
  virtual ~QuitCommand();

  virtual bool execute(CommandContext& ctx);
};

#endif
