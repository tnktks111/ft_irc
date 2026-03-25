#include "JoinCommand.hpp"
#include "ReplyBuilder.hpp"
#include <iostream>
#include <map>

JoinCommand::JoinCommand(ServerContext &serverCtx) : _serverCtx(serverCtx) {}
JoinCommand::~JoinCommand() {}

std::string JoinCommand::_generateChannelMemberStr(const Channel &channel) {
  std::string namesList;
  const std::map<int, Client *> &members = channel.getMembers();

  for (std::map<int, Client *>::const_iterator it = members.begin();
       it != members.end(); ++it) {
    if (!namesList.empty())
      namesList += " ";
    if (channel.isOperator(*(it->second)))
      namesList += "@";
    namesList += it->second->getNickName();
  }
  return namesList;
}

bool JoinCommand::execute(CommandContext &ctx) {
  if (ctx.params().empty()) {
    ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "JOIN"));
    return true;
  }

  std::string chName = ctx.params()[0];
  ServerContext::ChannelSlot slot = _serverCtx.getOrCreateChannel(chName);
  Channel *channel = slot.first;
  bool isNewChannel = slot.second;

  if (isNewChannel) {
    std::cout << "[+] Channel created: " << chName << std::endl;
  }

  if (!isNewChannel && channel->isInviteOnly() && !channel->isInvited(ctx.nick())) {
    ctx.reply(ReplyBuilder::errInviteOnlyChan(ctx.nick(), chName));
    return true;
  }

  if (!isNewChannel && !channel->getPassword().empty()) {
    std::string providedKey = (ctx.params().size() > 1) ? ctx.params()[1] : "";
    if (providedKey != channel->getPassword()) {
      ctx.reply(ReplyBuilder::errBadChannelKey(ctx.nick(), chName));
      return true;
    }
  }

  if (!isNewChannel && channel->getUserLimit() > 0 &&
      channel->getMemberCount() >= channel->getUserLimit()) {
    ctx.reply(ReplyBuilder::errChannelIsFull(ctx.nick(), chName));
    return true;
  }

  if (!channel->hasMember(ctx.client())) {
    channel->addMember(ctx.client());

    if (isNewChannel) {
      channel->addOperator(ctx.client());
      std::cout << "[*] " << ctx.nick() << " is now the operator of " << chName
                << std::endl;
    }

    channel->removeInvite(ctx.nick());

    ctx.broadcast(*channel, ":" + ctx.prefix() + " JOIN :" + chName);

    if (channel->getTopic().empty())
      ctx.reply(ReplyBuilder::rplNoTopic(ctx.nick(), chName));
    else
      ctx.reply(ReplyBuilder::rplTopic(ctx.nick(), chName, channel->getTopic()));

    ctx.reply(ReplyBuilder::rplNamReply(
        ctx.nick(), chName, _generateChannelMemberStr(*channel)));
    ctx.reply(ReplyBuilder::rplEndOfNames(ctx.nick(), chName));
  }

  return true;
}
