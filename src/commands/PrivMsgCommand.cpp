#include "PrivMsgCommand.hpp"
#include "MsgTargetResolver.hpp"
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

  const std::string& rawTargets = ctx.params()[0];
  const std::string& text = ctx.params()[1];

  MsgTargetResolver resolver(_serverCtx);
  MsgTargetResolution resolution = resolver.resolve(rawTargets);

  for (std::vector<MsgTargetEntry>::const_iterator it =
           resolution.entries.begin();
       it != resolution.entries.end(); ++it) {
    const MsgTargetEntry& entry = *it;

    switch (entry.result) {
      case MSGTARGET_RESOLVE_NO_SUCH_NICK:
        ctx.reply(ReplyBuilder::errNoSuchNick(ctx.nick(), entry.raw));
        break;
      case MSGTARGET_RESOLVE_NO_SUCH_CHANNEL:
        ctx.reply(ReplyBuilder::errNoSuchChannel(ctx.nick(), entry.raw));
        break;
      case MSGTARGET_RESOLVE_AMBIGUOUS:
        ctx.reply(ReplyBuilder::errTooManyTargets(
            ctx.nick(), entry.raw, "Duplicate", "No message delivered"));
        break;
      case MSGTARGET_RESOLVE_OK:
        switch (entry.target.kind) {
          case MSGTARGET_CHANNEL: {
            if (entry.target.channel == NULL)
              break;
            if (!entry.target.channel->hasMember(ctx.client())) {
              ctx.reply(
                  ReplyBuilder::errCantSendToChannel(ctx.nick(), entry.raw));
              break;
            }
            std::string fullMsg =
                ":" + ctx.prefix() + " PRIVMSG " + entry.raw + " :" + text;
            ctx.broadcastExcept(*entry.target.channel, fullMsg, ctx.client());
            break;
          }
          case MSGTARGET_CLIENT: {
            if (entry.target.client == NULL)
              break;
            std::string fullMsg =
                ":" + ctx.prefix() + " PRIVMSG " + entry.raw + " :" + text;
            ctx.direct(*entry.target.client, fullMsg);
            break;
          }
          case MSGTARGET_UNSUPPORTED:
            break;
        }
        break;
    }
  }
  return true;
}
