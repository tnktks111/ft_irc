#include "WhoCommand.hpp"
#include <map>
#include "ReplyBuilder.hpp"
#include "IrcCaseMapping.hpp"
#include "HostCaseMapping.hpp"

WhoCommand::WhoCommand(ServerContext& serverCtx) : _serverCtx(serverCtx) {}

WhoCommand::~WhoCommand() {}

bool WhoCommand::execute(CommandContext& ctx) {
  const std::string mask = ctx.params().empty() ? "*" : ctx.params()[0];

  // Check if mask is a channel name by checking prefix
  if (!mask.empty() && Channel::isChannelPrefix(mask[0])) {
    // Try to find the channel
    Channel* ch = _serverCtx.findChannel(mask);
    if (ch) {
      // Found the channel - output all members
      const std::map<int, Client*>& members = ch->getMembers();
      for (std::map<int, Client*>::const_iterator it = members.begin();
           it != members.end(); ++it) {
        if (it->second == NULL)
          continue;

        ctx.reply(ReplyBuilder::rplWhoReply(
            ctx.nick(), ch->getName(), it->second->getUserName(),
            it->second->getHost(), "ircserv", it->second->getNickName(), "H",
            "0", it->second->getRealName()));
      }
    }
    // If channel not found, output nothing (but still send end-of-who)
  } else {
    // Not a channel - search all users for matching nickname, host, or realname
    const std::map<int, Client*>& allClients = _serverCtx.getAllClients();
    for (std::map<int, Client*>::const_iterator it = allClients.begin();
         it != allClients.end(); ++it) {
      if (it->second == NULL)
        continue;

      Client* client = it->second;
      bool matches = false;

      // Check if mask matches nickname, host, or realname
      if (IrcCaseMapping::equals(client->getNickName(), mask) ||
          HostCaseMapping::equals(client->getHost(), mask) ||
          client->getRealName() == mask || mask == "*") {
        matches = true;
      }

      if (matches) {
        ctx.reply(ReplyBuilder::rplWhoReply(
            ctx.nick(), "*", client->getUserName(), client->getHost(),
            "ircserv", client->getNickName(), "H", "0",
            client->getRealName()));
      }
    }
  }

  ctx.reply(ReplyBuilder::rplEndOfWho(ctx.nick(), mask));
  return true;
}
