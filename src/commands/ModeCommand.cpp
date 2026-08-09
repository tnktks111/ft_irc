#include "ModeCommand.hpp"
#include "ReplyBuilder.hpp"
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

  std::string appliedMode;
  char lastAppliedSign = 0;

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

    bool applied = false;

    if (flag == 'i') {
      channel->setInviteOnly(adding);
      applied = true;
    } else if (flag == 't') {
      channel->setTopicProtected(adding);
      applied = true;
    } else if (flag == 'k') {
      if (adding) {
        if (ctx.params().size() <= paramIndex) {
          ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "MODE"));
          break;
        }
        if (!channel->getPassword().empty()) {
          ctx.reply(ReplyBuilder::errKeySet(chName));
          break;
        }
        channel->setPassword(ctx.params()[paramIndex]);
        modeParams += " " + ctx.params()[paramIndex++];
      } else {
        channel->setPassword("");
      }
      applied = true;
    } else if (flag == 'o') {
      if (ctx.params().size() <= paramIndex) {
        ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "MODE"));
        break;
      }
      std::string targetNick = ctx.params()[paramIndex++];
      Client *targetClient = _serverCtx.findClientByNick(targetNick);
      if (targetClient == NULL) {
        ctx.reply(ReplyBuilder::errNoSuchNick(ctx.nick(), targetNick));
        break;
      }
      if (!channel->hasMember(*targetClient)) {
        ctx.reply(
            ReplyBuilder::errUserNotInChannel(ctx.nick(), targetNick, chName));
        break;
      }
      if (adding)
        channel->addOperator(*targetClient);
      else
        channel->removeOperator(*targetClient);
      modeParams += " " + targetNick;
      applied = true;
    } else if (flag == 'l') {
      if (adding) {
        if (ctx.params().size() <= paramIndex) {
          ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "MODE"));
          break;
        }
        channel->setUserLimit(std::atoi(ctx.params()[paramIndex].c_str()));
        modeParams += " " + ctx.params()[paramIndex++];
      } else {
        channel->setUserLimit(0);
      }
      applied = true;
    } else {
      ctx.reply(ReplyBuilder::errUnknownMode(ctx.nick(), flag, chName));
      continue;
    }

    if (applied) {
      char sign = adding ? '+' : '-';
      if (sign != lastAppliedSign) {
        appliedMode += sign;
        lastAppliedSign = sign;
      }
      appliedMode += flag;
    }
  }

  if (!appliedMode.empty()) {
    ctx.broadcast(*channel,
                  ":" + ctx.prefix() + " MODE " + chName + " " + appliedMode +
                      modeParams);
  }
  return true;
}
