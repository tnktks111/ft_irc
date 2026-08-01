#include "PingCommand.hpp"
#include "ReplyBuilder.hpp"

PingCommand::PingCommand(ServerContext &serverCtx) : _serverCtx(serverCtx) {}
PingCommand::~PingCommand() {}

bool PingCommand::execute(CommandContext &ctx) {
  if (ctx.params().empty()) {
    ctx.reply(ReplyBuilder::errNoOrigin(ctx.nick()));
    return true;
  }

  // trailing パラメータとして token をエコーする (RFC 2812 §3.7.3)。
  // leading server prefix (":<servername> ") は #56 で ResponseSink 側に一括
  // 導入する予定のためここでは付与しない。
  ctx.reply("PONG :" + ctx.params()[0]);
  return true;
}
