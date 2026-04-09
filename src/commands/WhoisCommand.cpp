#include "WhoisCommand.hpp"
#include <string>
#include <vector>
#include "ReplyBuilder.hpp"

namespace {
const std::string kWhoisServerName = "ircserv";
const std::string kWhoisServerInfo = "ft_irc server";

std::vector<std::string> splitTargets(const std::string& rawTargets) {
  std::vector<std::string> result;
  std::string::size_type start = 0;

  while (start <= rawTargets.length()) {
    std::string::size_type comma = rawTargets.find(',', start);
    if (comma == std::string::npos) {
      result.push_back(rawTargets.substr(start));
      break;
    }
    result.push_back(rawTargets.substr(start, comma - start));
    start = comma + 1;
    if (start == rawTargets.length()) {
      result.push_back("");
      break;
    }
  }

  return result;
}

std::string buildChannelList(const Client& client,
                              const std::vector<Channel*>& channels) {
  std::string list;

  for (std::vector<Channel*>::const_iterator it = channels.begin();
       it != channels.end(); ++it) {
    if (*it == NULL)
      continue;
    if (!list.empty())
      list += " ";
    if ((*it)->isOperator(client))
      list += "@";
    list += (*it)->getName();
  }
  return list;
}
}  // namespace

WhoisCommand::WhoisCommand(ServerContext& serverCtx) : _serverCtx(serverCtx) {}
WhoisCommand::~WhoisCommand() {}

bool WhoisCommand::execute(CommandContext& ctx) {
  if (ctx.params().empty()) {
    ctx.reply(ReplyBuilder::errNoNicknameGiven());
    return true;
  }

  std::string rawTargets = ctx.params().back();
  std::vector<std::string> nicknames = splitTargets(rawTargets);

  for (std::vector<std::string>::const_iterator it = nicknames.begin();
       it != nicknames.end(); ++it) {
    const std::string& targetNick = *it;
    if (targetNick.empty()) {
      ctx.reply(ReplyBuilder::errNoNicknameGiven());
      continue;
    }

    Client* targetClient = _serverCtx.findClientByNick(targetNick);
    if (targetClient == NULL) {
      ctx.reply(ReplyBuilder::errNoSuchNick(ctx.nick(), targetNick));
      ctx.reply(ReplyBuilder::rplEndOfWhois(ctx.nick(), targetNick));
      continue;
    }

    ctx.reply(ReplyBuilder::rplWhoisUser(
        ctx.nick(), targetClient->getNickName(), targetClient->getUserName(),
        targetClient->getHost(), targetClient->getRealName()));
    ctx.reply(ReplyBuilder::rplWhoisServer(ctx.nick(),
                                           targetClient->getNickName(),
                                           kWhoisServerName,
                                           kWhoisServerInfo));

    std::vector<Channel*> channels = _serverCtx.findChannelsOf(*targetClient);
    std::string channelsList = buildChannelList(*targetClient, channels);
    if (!channelsList.empty()) {
      ctx.reply(ReplyBuilder::rplWhoisChannels(ctx.nick(),
                                               targetClient->getNickName(),
                                               channelsList));
    }

    ctx.reply(ReplyBuilder::rplEndOfWhois(ctx.nick(), targetClient->getNickName()));
  }

  return true;
}