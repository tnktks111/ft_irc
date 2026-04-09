#include "KickCommand.hpp"
#include <iostream>
#include "ReplyBuilder.hpp"

KickCommand::KickCommand(ServerContext& serverCtx) : _serverCtx(serverCtx) {}
KickCommand::~KickCommand() {}

bool KickCommand::execute(CommandContext& ctx) {
  if (ctx.params().size() < 2) {
    ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "KICK"));
    return true;
  }

  std::string chName = ctx.params()[0];
  std::string targetNick = ctx.params()[1];
  std::string reason = (ctx.params().size() > 2) ? ctx.params()[2] : targetNick;

  Channel* channel = _serverCtx.findChannel(chName);
  if (channel == NULL) {
    ctx.reply(ReplyBuilder::errNoSuchChannel(ctx.nick(), chName));
    return true;
  }
  if (!channel->hasMember(ctx.client())) {
    ctx.reply(ReplyBuilder::errNotOnChannel(ctx.nick(), chName));
    return true;
  }
  if (!ctx.isOperatorOf(*channel)) {
    ctx.reply(ReplyBuilder::errChanOPrivsNeeded(ctx.nick(), chName));
    return true;
  }

  Client* targetClient = _serverCtx.findClientByNick(targetNick);
  if (targetClient == NULL || !channel->hasMember(*targetClient)) {
    ctx.reply(
        ReplyBuilder::errUserNotInChannel(ctx.nick(), targetNick, chName));
    return true;
  }

  ctx.broadcast(*channel, ":" + ctx.prefix() + " KICK " + chName + " " +
                              targetNick + " :" + reason);

  channel->removeMember(*targetClient);
  if (channel->getMemberCount() == 0) {
    _serverCtx.removeChannel(chName);
    std::cout << "[-] Channel deleted (no member): " << chName << std::endl;
  }

  return true;
}
