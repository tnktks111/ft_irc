#include "QuitCommand.hpp"

QuitCommand::QuitCommand(ServerContext &serverCtx) : _serverCtx(serverCtx) {}
QuitCommand::~QuitCommand() {}

bool QuitCommand::execute(CommandContext &ctx) {
  std::string reason =
      (ctx.params().empty()) ? "Client Quit" : ctx.params()[0];

  ctx.reply("ERROR :Closing Link: " + ctx.client().getHost() + " (Quit: " +
            reason + ")");

  _serverCtx.removeClientFromAllChannels(ctx.client(), reason);
  return false;
}
