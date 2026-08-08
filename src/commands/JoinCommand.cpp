#include "JoinCommand.hpp"
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "ReplyBuilder.hpp"
#include "StringUtils.hpp"

JoinCommand::JoinCommand(ServerContext& serverCtx) : _serverCtx(serverCtx) {}
JoinCommand::~JoinCommand() {}

std::string JoinCommand::_generateChannelMemberStr(const Channel& channel) {
  std::string namesList;
  const std::map<int, Client*>& members = channel.getMembers();

  for (std::map<int, Client*>::const_iterator it = members.begin();
       it != members.end(); ++it) {
    if (!namesList.empty())
      namesList += " ";
    if (channel.isOperator(*(it->second)))
      namesList += "@";
    namesList += it->second->getNickName();
  }
  return namesList;
}

bool JoinCommand::execute(CommandContext& ctx) {
  if (ctx.params().empty()) {
    ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "JOIN"));
    return true;
  }

  if (ctx.params()[0] == "0") {
    ctx.serverCtx().leaveAllChannels(ctx.client());
    return true;
  }

  std::vector<std::string> chNames = StringUtils::split(ctx.params()[0], ',');
  std::vector<std::string> keys =
      (ctx.params().size() > 1) ? StringUtils::split(ctx.params()[1], ',')
                                : std::vector<std::string>();

  for (std::size_t idx = 0; idx != chNames.size(); ++idx) {
    std::string chName = chNames[idx];
    std::string key = (idx < keys.size()) ? keys[idx] : "";

    if (Channel::isValidChannelName(chName) == false) {
      ctx.reply(ReplyBuilder::errNoSuchChannel(ctx.nick(), chName));
      continue;
    }

    ServerContext::ChannelSlot slot = _serverCtx.getOrCreateChannel(chName);
    Channel* channel = slot.first;
    bool isNewChannel = slot.second;

    if (isNewChannel) {
      std::cout << "[+] Channel created: " << chName << std::endl;
    } else {
      if (channel->isInviteOnly() && !channel->isInvited(ctx.nick())) {
        ctx.reply(ReplyBuilder::errInviteOnlyChan(ctx.nick(), chName));
        continue;
      }
      if (channel->getUserLimit() > 0 &&
          channel->getMemberCount() >= channel->getUserLimit()) {
        ctx.reply(ReplyBuilder::errChannelIsFull(ctx.nick(), chName));
        continue;
      }
      if (!channel->getPassword().empty() && key != channel->getPassword()) {
        ctx.reply(ReplyBuilder::errBadChannelKey(ctx.nick(), chName));
        continue;
      }
    }
    if (!channel->hasMember(ctx.client())) {
      channel->addMember(ctx.client());

      if (isNewChannel) {
        channel->addOperator(ctx.client());
        std::cout << "[*] " << ctx.nick() << " is now the operator of "
                  << chName << std::endl;
      }

      channel->removeInvite(ctx.nick());

      ctx.broadcast(*channel, ":" + ctx.prefix() + " JOIN :" + chName);

      if (channel->getTopic().empty())
        ctx.reply(ReplyBuilder::rplNoTopic(ctx.nick(), chName));
      else
        ctx.reply(
            ReplyBuilder::rplTopic(ctx.nick(), chName, channel->getTopic()));

      ctx.reply(ReplyBuilder::rplNamReply(ctx.nick(), chName,
                                          _generateChannelMemberStr(*channel)));
      ctx.reply(ReplyBuilder::rplEndOfNames(ctx.nick(), chName));
    }
  }
  return true;
}
