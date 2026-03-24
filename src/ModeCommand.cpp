#include "../include/ModeCommand.hpp"
#include "../include/ReplyBuilder.hpp"
#include <cstdlib>
#include <sstream>

ModeCommand::ModeCommand(ServerContext &serverCtx) : _serverCtx(serverCtx) {}
ModeCommand::~ModeCommand() {}

bool ModeCommand::execute(CommandContext &ctx) {
  if (ctx.params().empty()) {
    ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "MODE"));
    return true;
  }

  std::string chName = ctx.params()[0];
  Channel *channel = _serverCtx.findChannel(chName);
  if (channel == NULL) {
    ctx.reply(ReplyBuilder::errNoSuchChannel(ctx.nick(), chName));
    return true;
  }

  if (ctx.params().size() == 1) {
    std::string modes = "+";
    std::string params;

    if (channel->isInviteOnly())
      modes += "i";
    if (channel->isTopicProtected())
      modes += "t";
    if (!channel->getPassword().empty()) {
      modes += "k";
      params += " " + channel->getPassword();
    }
    if (channel->getUserLimit() > 0) {
      std::ostringstream oss;
      oss << channel->getUserLimit();
      modes += "l";
      params += " " + oss.str();
    }

    ctx.reply(ReplyBuilder::rplChannelModeIs(ctx.nick(), chName, modes, params));
    return true;
  }

  if (!ctx.isMemberOf(*channel)) {
    ctx.reply(ReplyBuilder::errNotOnChannel(ctx.nick(), chName));
    return true;
  }
  if (!ctx.isOperatorOf(*channel)) {
    ctx.reply(ReplyBuilder::errChanOPrivsNeeded(ctx.nick(), chName));
    return true;
  }

  std::string mode = ctx.params()[1];
  bool adding = true;
  size_t paramIndex = 2;
  std::string modeParams;

  for (size_t i = 0; i < mode.size(); ++i) {
    char flag = mode[i];

    if (flag == '+') {
      adding = true;
      continue;
    }
    if (flag == '-') {
      adding = false;
      continue;
    }

    if (flag == 'i') {
      channel->setInviteOnly(adding);
    } else if (flag == 't') {
      channel->setTopicProtected(adding);
    } else if (flag == 'k') {
      if (adding) {
        if (ctx.params().size() <= paramIndex) {
          ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "MODE"));
          return true;
        }
        channel->setPassword(ctx.params()[paramIndex]);
        modeParams += " " + ctx.params()[paramIndex++];
      } else {
        channel->setPassword("");
      }
    } else if (flag == 'o') {
      if (ctx.params().size() <= paramIndex) {
        ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "MODE"));
        return true;
      }
      std::string targetNick = ctx.params()[paramIndex++];
      Client *targetClient = _serverCtx.findClientByNick(targetNick);
      if (targetClient != NULL && channel->hasMember(*targetClient)) {
        if (adding)
          channel->addOperator(*targetClient);
        else
          channel->removeOperator(*targetClient);
        modeParams += " " + targetNick;
      }
    } else if (flag == 'l') {
      if (adding) {
        if (ctx.params().size() <= paramIndex) {
          ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "MODE"));
          return true;
        }
        channel->setUserLimit(std::atoi(ctx.params()[paramIndex].c_str()));
        modeParams += " " + ctx.params()[paramIndex++];
      } else {
        channel->setUserLimit(0);
      }
    }
  }

  ctx.broadcast(*channel,
                ":" + ctx.prefix() + " MODE " + chName + " " + mode +
                    modeParams);
  return true;
}
