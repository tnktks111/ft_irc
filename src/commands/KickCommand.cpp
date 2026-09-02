#include "KickCommand.hpp"
#include <iostream>
#include <string>
#include <vector>
#include "ReplyBuilder.hpp"
#include "StringUtils.hpp"

KickCommand::KickCommand(ServerContext& serverCtx) : _serverCtx(serverCtx) {}
KickCommand::~KickCommand() {}

bool KickCommand::execute(CommandContext& ctx) {
  if (ctx.params().size() < 2) {
    ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "KICK"));
    return true;
  }

  std::vector<std::string> channels = StringUtils::split(ctx.params()[0], ',');
  std::vector<std::string> users = StringUtils::split(ctx.params()[1], ',');
  const bool hasReason = ctx.params().size() > 2;
  const std::string reason = hasReason ? ctx.params()[2] : std::string();

  if (channels.empty() || users.empty()) {
    ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "KICK"));
    return true;
  }

  // RFC 2812 §3.2.8: channels は 1 個、または users と同数でなければならない
  // (paired form)。それ以外は不整合なので 461 を返す。
  if (channels.size() != 1 && channels.size() != users.size()) {
    ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "KICK"));
    return true;
  }

  for (std::size_t i = 0; i < users.size(); ++i) {
    const std::string& chName =
        (channels.size() == 1) ? channels[0] : channels[i];
    const std::string& targetNick = users[i];

    if (chName.empty() || targetNick.empty())
      continue;

    // reason 未指定なら各 user 名をそのまま comment に使う (元実装の挙動)
    const std::string& comment = hasReason ? reason : targetNick;

    Channel* channel = _serverCtx.findChannel(chName);
    if (channel == NULL) {
      ctx.reply(ReplyBuilder::errNoSuchChannel(ctx.nick(), chName));
      continue;
    }
    if (!channel->hasMember(ctx.client())) {
      ctx.reply(ReplyBuilder::errNotOnChannel(ctx.nick(), chName));
      continue;
    }
    if (!ctx.isOperatorOf(*channel)) {
      ctx.reply(ReplyBuilder::errChanOPrivsNeeded(ctx.nick(), chName));
      continue;
    }

    Client* targetClient = _serverCtx.findClientByNick(targetNick);
    if (targetClient == NULL || !channel->hasMember(*targetClient)) {
      ctx.reply(
          ReplyBuilder::errUserNotInChannel(ctx.nick(), targetNick, chName));
      continue;
    }

    ctx.broadcast(*channel, ":" + ctx.prefix() + " KICK " + chName + " " +
                                targetNick + " :" + comment);

    channel->removeMember(*targetClient);
    if (channel->getMemberCount() == 0) {
      _serverCtx.removeChannel(chName);
      std::cout << "[-] Channel deleted (no member): " << chName << std::endl;
    }
  }

  return true;
}
