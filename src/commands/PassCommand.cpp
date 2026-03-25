#include "PassCommand.hpp"
#include "ReplyBuilder.hpp"

PassCommand::PassCommand(ServerContext &serverCtx) : _serverCtx(serverCtx) {}
PassCommand::~PassCommand() {}

bool PassCommand::execute(CommandContext &ctx) {
  if (ctx.isRegistered()) {
    ctx.reply(ReplyBuilder::errAlreadyRegistered(ctx.nick()));
    return true;
  }
  if (ctx.params().empty()) {
    ctx.reply(ReplyBuilder::errNeedMoreParams("*", "PASS"));
    return true;
  }

  if (ctx.params()[0] == _serverCtx.password()) {
    ctx.client().setPassChecked(true);
    _serverCtx.tryCompleteRegistration(ctx.client());
  } else {
    ctx.reply(ReplyBuilder::errIncorrectPassword());
  }
  return true;
}
