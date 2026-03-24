#include "../include/PartCommand.hpp"
#include "../include/ReplyBuilder.hpp"
#include <iostream>

PartCommand::PartCommand(ServerContext &serverCtx) : _serverCtx(serverCtx) {}
PartCommand::~PartCommand() {}

bool PartCommand::execute(CommandContext &ctx) {
  if (ctx.params().empty()) {
    ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "PART"));
    return true;
  }

  std::string chName = ctx.params()[0];
  std::string partMsg = (ctx.params().size() > 1) ? ctx.params()[1] : "Leaving";

  Channel *channel = _serverCtx.findChannel(chName);
  if (channel == NULL) {
    ctx.reply(ReplyBuilder::errNoSuchChannel(ctx.nick(), chName));
    return true;
  }
  if (!channel->hasMember(ctx.client())) {
    ctx.reply(ReplyBuilder::errNotOnChannel(ctx.nick(), chName));
    return true;
  }

  ctx.broadcast(*channel,
                ":" + ctx.prefix() + " PART " + chName + " :" + partMsg);

  channel->removeMember(ctx.client());
  if (channel->getMemberCount() == 0) {
    _serverCtx.removeChannel(chName);
    std::cout << "[-] Channel deleted (no member): " << chName << std::endl;
  }

  return true;
}
