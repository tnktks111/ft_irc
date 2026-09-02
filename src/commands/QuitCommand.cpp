#include "QuitCommand.hpp"

QuitCommand::QuitCommand(ServerContext& serverCtx) : _serverCtx(serverCtx) {}
QuitCommand::~QuitCommand() {}

bool QuitCommand::execute(CommandContext& ctx) {
  std::string reason = (ctx.params().empty()) ? "Client Quit" : ctx.params()[0];
  std::string quitMsg = ":" + ctx.prefix() + " QUIT :" + reason;

  _serverCtx.removeClientFromAllChannels(ctx.client(), quitMsg);
  return false;
}
