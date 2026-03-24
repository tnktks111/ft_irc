#include "../include/InviteCommand.hpp"
#include "../include/ReplyBuilder.hpp"

InviteCommand::InviteCommand(ServerContext &serverCtx) : _serverCtx(serverCtx) {}
InviteCommand::~InviteCommand() {}

bool InviteCommand::execute(CommandContext &ctx) {
  if (ctx.params().size() < 2) {
    ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "INVITE"));
    return true;
  }

  std::string targetNick = ctx.params()[0];
  std::string chName = ctx.params()[1];

  Channel *channel = _serverCtx.findChannel(chName);
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

  Client *targetClient = _serverCtx.findClientByNick(targetNick);
  if (targetClient == NULL) {
    ctx.reply(ReplyBuilder::errNoSuchNick(ctx.nick(), targetNick));
    return true;
  }
  if (channel->hasMember(*targetClient)) {
    ctx.reply(ReplyBuilder::errUserOnChannel(ctx.nick(), targetNick, chName));
    return true;
  }

  channel->addInvite(targetNick);
  ctx.reply(ReplyBuilder::rplInviting(ctx.nick(), chName, targetNick));
  ctx.direct(*targetClient,
             ":" + ctx.prefix() + " INVITE " + targetNick + " :" + chName);
  return true;
}
