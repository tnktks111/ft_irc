#include "WhoCommand.hpp"
#include <map>
#include "ReplyBuilder.hpp"

WhoCommand::WhoCommand(ServerContext& serverCtx) : _serverCtx(serverCtx) {}

WhoCommand::~WhoCommand() {}

bool WhoCommand::execute(CommandContext& ctx) {
  const std::string mask = ctx.params().empty() ? "*" : ctx.params()[0];

  Channel* ch = _serverCtx.findChannel(mask);
  if (ch) {
    const std::map<int, Client*>& members = ch->getMembers();
    for (std::map<int, Client*>::const_iterator it = members.begin();
         it != members.end(); ++it) {
      if (it->second == NULL)
        continue;

      ctx.reply(ReplyBuilder::rplWhoReply(
          ctx.nick(), ch->getName(), it->second->getUserName(),
          it->second->getHost(), "ircserv", it->second->getNickName(), "H", "0",
          it->second->getRealName()));
    }
  }

  ctx.reply(ReplyBuilder::rplEndOfWho(ctx.nick(), mask));
  return true;
}
