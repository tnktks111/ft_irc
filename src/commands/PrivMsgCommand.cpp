#include "PrivMsgCommand.hpp"
#include "ReplyBuilder.hpp"

PrivMsgCommand::PrivMsgCommand(ServerContext& serverCtx)
    : _serverCtx(serverCtx) {}
PrivMsgCommand::~PrivMsgCommand() {}

bool PrivMsgCommand::execute(CommandContext& ctx) {
  if (ctx.params().empty()) {
    ctx.reply(ReplyBuilder::errNoRecipient("PRIVMSG"));
    return true;
  }
  if (ctx.params().size() < 2 || ctx.params()[1].empty()) {
    ctx.reply(ReplyBuilder::errNoTextToSend(ctx.nick()));
    return true;
  }

  std::string target = ctx.params()[0];
  std::string text = ctx.params()[1];
  std::string fullMsg = ":" + ctx.prefix() + " PRIVMSG " + target + " :" + text;

  if (!target.empty() && Channel::isChannelPrefix(*target.begin())) {
    Channel* channel = _serverCtx.findChannel(target);
    if (channel == NULL) {
      ctx.reply(ReplyBuilder::errNoSuchNick(ctx.nick(), target));
      return true;
    }
    if (!channel->hasMember(ctx.client())) {
      ctx.reply(ReplyBuilder::errCantSendToChannel(ctx.nick(), target));
      return true;
    }
    ctx.broadcastExcept(*channel, fullMsg, ctx.client());
    return true;
  }

  Client* targetClient = _serverCtx.findClientByNick(target);
  if (targetClient == NULL) {
    ctx.reply(ReplyBuilder::errNoSuchNick(ctx.nick(), target));
    return true;
  }

  ctx.direct(*targetClient, fullMsg);
  return true;
}
