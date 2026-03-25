#include "TopicCommand.hpp"
#include "ReplyBuilder.hpp"

TopicCommand::TopicCommand(ServerContext &serverCtx) : _serverCtx(serverCtx) {}
TopicCommand::~TopicCommand() {}

bool TopicCommand::execute(CommandContext &ctx) {
  if (ctx.params().empty()) {
    ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "TOPIC"));
    return true;
  }

  std::string chName = ctx.params()[0];
  Channel *channel = _serverCtx.findChannel(chName);
  if (channel == NULL) {
    ctx.reply(ReplyBuilder::errNoSuchChannel(ctx.nick(), chName));
    return true;
  }
  if (!channel->hasMember(ctx.client())) {
    ctx.reply(ReplyBuilder::errNotOnChannel(ctx.nick(), chName));
    return true;
  }

  if (ctx.params().size() == 1) {
    if (channel->getTopic().empty())
      ctx.reply(ReplyBuilder::rplNoTopic(ctx.nick(), chName));
    else
      ctx.reply(ReplyBuilder::rplTopic(ctx.nick(), chName, channel->getTopic()));
    return true;
  }

  if (channel->isTopicProtected() && !ctx.isOperatorOf(*channel)) {
    ctx.reply(ReplyBuilder::errChanOPrivsNeeded(ctx.nick(), chName));
    return true;
  }

  channel->setTopic(ctx.params()[1]);
  ctx.broadcast(*channel,
                ":" + ctx.prefix() + " TOPIC " + chName + " :" +
                    channel->getTopic());
  return true;
}
