#include "QuitCommand.hpp"

QuitCommand::QuitCommand(ServerContext &serverCtx) : _serverCtx(serverCtx) {}
QuitCommand::~QuitCommand() {}

bool QuitCommand::execute(CommandContext &ctx) {
  std::string quitMsg = (ctx.params().empty()) ? "Client Quit" : ctx.params()[0];
  _serverCtx.removeClientFromAllChannels(ctx.client(), quitMsg);
  return false;
}
