#include "NickCommand.hpp"
#include "Client.hpp"
#include "ReplyBuilder.hpp"

NickCommand::NickCommand(ServerContext& serverCtx) : _serverCtx(serverCtx) {}
NickCommand::~NickCommand() {}

bool NickCommand::execute(CommandContext& ctx) {
  if (ctx.params().empty()) {
    ctx.reply(ReplyBuilder::errNoNicknameGiven());
    return true;
  }

  std::string newNick = ctx.params()[0];

  if (Client::isValidNickName(newNick) == false) {
    ctx.reply(ReplyBuilder::erroneusNickName(newNick));
    return true;
  }

  if (_serverCtx.hasNick(newNick, ctx.client())) {
    ctx.reply(ReplyBuilder::errNickNameInUse(newNick));
    return true;
  }

  ctx.client().setNickName(newNick);
  _serverCtx.tryCompleteRegistration(ctx.client());
  return true;
}
