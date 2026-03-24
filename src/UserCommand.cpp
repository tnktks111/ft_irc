#include "../include/UserCommand.hpp"
#include "../include/ReplyBuilder.hpp"

UserCommand::UserCommand(ServerContext &serverCtx) : _serverCtx(serverCtx) {}
UserCommand::~UserCommand() {}

bool UserCommand::execute(CommandContext &ctx) {
  if (ctx.isRegistered()) {
    ctx.reply(ReplyBuilder::errAlreadyRegistered(ctx.nick()));
    return true;
  }
  if (ctx.params().size() < 4) {
    ctx.reply(ReplyBuilder::errNeedMoreParams("*", "USER"));
    return true;
  }

  ctx.client().setUserName(ctx.params()[0]);
  _serverCtx.tryCompleteRegistration(ctx.client());
  return true;
}
