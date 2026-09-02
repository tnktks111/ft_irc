#include "ModeCommand.hpp"
#include <sstream>
#include "IrcCaseMapping.hpp"
#include "ReplyBuilder.hpp"

namespace {
bool parsePositiveSize(const std::string& value, size_t& result) {
  if (value.empty())
    return false;

  result = 0;
  for (std::string::const_iterator it = value.begin(); it != value.end();
       ++it) {
    if (*it < '0' || *it > '9')
      return false;

    size_t digit = static_cast<size_t>(*it - '0');
    if (result > (static_cast<size_t>(-1) - digit) / 10)
      return false;

    result = result * 10 + digit;
  }
  return result > 0;
}
}  // namespace

ModeCommand::ModeCommand(ServerContext& serverCtx) : _serverCtx(serverCtx) {}
ModeCommand::~ModeCommand() {}

namespace {

UserMode userModeFromChar(char c) {
  switch (c) {
    case 'i':
      return UMODE_INVIS;
    case 'w':
      return UMODE_WALLO;
    case 's':
      return UMODE_SNICE;
    case 'r':
      return UMODE_RESTR;
    case 'o':
      return UMODE_OPER;
    case 'O':
      return UMODE_LOPER;
    case 'a':
      return UMODE_AWAY;
    default:
      return UMODE_NONE;
  }
}

bool isSelfSettable(char c) {
  return c == 'i' || c == 'w' || c == 's' || c == 'r';
}

bool isSelfUnsettable(char c) {
  return c == 'i' || c == 'w' || c == 's' || c == 'o' || c == 'O' || c == 'a';
}

}  // namespace

bool ModeCommand::_executeUserMode(CommandContext& ctx,
                                   const std::string& target) {
  if (!IrcCaseMapping::equals(target, ctx.nick())) {
    ctx.reply(ReplyBuilder::errUsersDontMatch(ctx.nick()));
    return true;
  }

  if (ctx.params().size() == 1) {
    ctx.reply(
        ReplyBuilder::rplUmodeIs(ctx.nick(), ctx.client().getModeString()));
    return true;
  }

  const std::string& mode = ctx.params()[1];
  bool adding = true;
  bool sawUnknown = false;

  for (std::size_t i = 0; i < mode.size(); ++i) {
    char flag = mode[i];
    if (flag == '+') {
      adding = true;
      continue;
    }
    if (flag == '-') {
      adding = false;
      continue;
    }

    UserMode um = userModeFromChar(flag);
    if (um == UMODE_NONE) {
      sawUnknown = true;
      continue;
    }

    if (adding) {
      if (isSelfSettable(flag))
        ctx.client().addMode(um);
    } else {
      if (isSelfUnsettable(flag))
        ctx.client().removeMode(um);
    }
  }

  if (sawUnknown)
    ctx.reply(ReplyBuilder::errUmodeUnknownFlag(ctx.nick()));

  ctx.reply(":" + ctx.prefix() + " MODE " + ctx.nick() + " :" +
            ctx.client().getModeString());
  return true;
}

bool ModeCommand::execute(CommandContext& ctx) {
  if (ctx.params().empty()) {
    ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "MODE"));
    return true;
  }

  std::string chName = ctx.params()[0];

  if (chName.empty() || !Channel::isChannelPrefix(chName[0]))
    return _executeUserMode(ctx, chName);

  Channel* channel = _serverCtx.findChannel(chName);
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
      if (!params.empty())
        params += " ";
      params += channel->getPassword();
    }
    if (channel->getUserLimit() > 0) {
      std::ostringstream oss;
      oss << channel->getUserLimit();
      modes += "l";
      if (!params.empty())
        params += " ";
      params += oss.str();
    }

    ctx.reply(
        ReplyBuilder::rplChannelModeIs(ctx.nick(), chName, modes, params));
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
          ctx.reply(ReplyBuilder::errKeySet(ctx.nick(), chName));
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
      Client* targetClient = _serverCtx.findClientByNick(targetNick);
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
        size_t limit;
        if (!parsePositiveSize(ctx.params()[paramIndex], limit)) {
          ctx.reply(ReplyBuilder::errInvalidModeParam(
              ctx.nick(), chName, 'l', ctx.params()[paramIndex]));
          break;
        }
        channel->setUserLimit(limit);
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
    ctx.broadcast(*channel, ":" + ctx.prefix() + " MODE " + chName + " " +
                                appliedMode + modeParams);
  }
  return true;
}
