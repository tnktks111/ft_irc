#include "ModeCommand.hpp"
#include <cstdlib>
#include <sstream>
#include "IrcCaseMapping.hpp"
#include "ReplyBuilder.hpp"

ModeCommand::ModeCommand(ServerContext &serverCtx) : _serverCtx(serverCtx) {}
ModeCommand::~ModeCommand() {}

namespace {

// User mode 1 文字 → enum マッピング。未知 flag は UMODE_NONE を返す。
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

// User が直接 set できない mode (OPER/AWAY 等はサーバー側発行のみ)
bool isSelfSettable(char c) {
  return c == 'i' || c == 'w' || c == 's' || c == 'r';
}

// User が直接 unset できない mode (r は一度 set したら解除不可 RFC 2812)
bool isSelfUnsettable(char c) {
  return c == 'i' || c == 'w' || c == 's' || c == 'o' || c == 'O' || c == 'a';
}

}  // namespace

// User mode コマンド (MODE <selfnick> [mode]) の処理。
// target が自分以外なら 502 ERR_USERSDONTMATCH を返す。
bool ModeCommand::_executeUserMode(CommandContext &ctx,
                                   const std::string &target) {
  if (!IrcCaseMapping::equals(target, ctx.nick())) {
    ctx.reply(ReplyBuilder::errUsersDontMatch(ctx.nick()));
    return true;
  }

  // Query: mode 引数無し → 現在の user mode を 221 で返す
  if (ctx.params().size() == 1) {
    ctx.reply(
        ReplyBuilder::rplUmodeIs(ctx.nick(), ctx.client().getModeString()));
    return true;
  }

  // Modification
  const std::string &mode = ctx.params()[1];
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
      // 'o'/'O'/'a' の set は OPER/AWAY コマンド専用 → silent ignore
    } else {
      if (isSelfUnsettable(flag))
        ctx.client().removeMode(um);
      // 'r' の unset は RFC 上不可 → silent ignore
    }
  }

  if (sawUnknown)
    ctx.reply(ReplyBuilder::errUmodeUnknownFlag(ctx.nick()));

  // 変更後の状態を自身にエコー
  ctx.reply(":" + ctx.prefix() + " MODE " + ctx.nick() + " :" +
            ctx.client().getModeString());
  return true;
}

bool ModeCommand::execute(CommandContext &ctx) {
  if (ctx.params().empty()) {
    ctx.reply(ReplyBuilder::errNeedMoreParams(ctx.nick(), "MODE"));
    return true;
  }

  std::string chName = ctx.params()[0];

  // Channel prefix でなければ user mode コマンドとして処理する。
  if (chName.empty() || !Channel::isChannelPrefix(chName[0]))
    return _executeUserMode(ctx, chName);

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
        if (!channel->getPassword().empty()) {
          ctx.reply(ReplyBuilder::errKeySet(chName));
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
      if (targetClient == NULL) {
        ctx.reply(ReplyBuilder::errNoSuchNick(ctx.nick(), targetNick));
        return true;
      }
      if (!channel->hasMember(*targetClient)) {
        ctx.reply(
            ReplyBuilder::errUserNotInChannel(ctx.nick(), targetNick, chName));
        return true;
      }
      if (adding)
        channel->addOperator(*targetClient);
      else
        channel->removeOperator(*targetClient);
      modeParams += " " + targetNick;
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
    } else {
      ctx.reply(ReplyBuilder::errUnknownMode(ctx.nick(), flag, chName));
      return true;
    }
  }

  ctx.broadcast(*channel,
                ":" + ctx.prefix() + " MODE " + chName + " " + mode +
                    modeParams);
  return true;
}
