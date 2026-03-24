#include "../include/PingCommand.hpp"
#include "../include/ReplyBuilder.hpp"

PingCommand::PingCommand(ServerContext &serverCtx) : _serverCtx(serverCtx) {}
PingCommand::~PingCommand() {}

bool PingCommand::execute(CommandContext &ctx) {
  if (ctx.params().empty()) {
    ctx.reply(ReplyBuilder::errNoOrigin(ctx.nick()));
    return true;
  }

  ctx.reply("PONG " + ctx.params()[0]);
  return true;
}
