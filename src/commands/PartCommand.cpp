#include "PartCommand.hpp"
#include <iostream>
#include <string>
#include <vector>
#include "ReplyBuilder.hpp"
#include "StringUtils.hpp"

PartCommand::PartCommand(ServerContext &serverCtx) : _serverCtx(serverCtx) {}
PartCommand::~PartCommand() {}

bool PartCommand::execute(CommandContext &ctx) {
  if (ctx.params().empty()) {
    ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "PART"));
    return true;
  }

  std::vector<std::string> channels = StringUtils::split(ctx.params()[0], ',');
  std::string partMsg =
      (ctx.params().size() > 1) ? ctx.params()[1] : std::string("Leaving");

  if (channels.empty()) {
    ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "PART"));
    return true;
  }

  // RFC 2812 §3.2.2: 各 channel を独立に処理し、1 つ失敗しても他は継続。
  for (std::size_t i = 0; i < channels.size(); ++i) {
    const std::string &chName = channels[i];
    if (chName.empty())
      continue;

    Channel *channel = _serverCtx.findChannel(chName);
    if (channel == NULL) {
      ctx.reply(ReplyBuilder::errNoSuchChannel(ctx.nick(), chName));
      continue;
    }
    if (!channel->hasMember(ctx.client())) {
      ctx.reply(ReplyBuilder::errNotOnChannel(ctx.nick(), chName));
      continue;
    }

    ctx.broadcast(*channel,
                  ":" + ctx.prefix() + " PART " + chName + " :" + partMsg);

    channel->removeMember(ctx.client());
    if (channel->getMemberCount() == 0) {
      _serverCtx.removeChannel(chName);
      std::cout << "[-] Channel deleted (no member): " << chName << std::endl;
    }
  }

  return true;
}
