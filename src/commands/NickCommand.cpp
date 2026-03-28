#include "NickCommand.hpp"
#include "ReplyBuilder.hpp"

NickCommand::NickCommand(ServerContext &serverCtx) : _serverCtx(serverCtx) {}
NickCommand::~NickCommand() {}

bool NickCommand::_isLetter(char c) {
  return ((0x41 <= c && c <= 0x5a) || (0x61 <= c && c <= 0x7a));
}

bool NickCommand::_isDigit(char c) { return (0x30 <= c && c <= 0x39); }

bool NickCommand::_isSpecial(char c) {
  return ((0x5b <= c && c <= 0x60) || (0x7b <= c && c <= 0x7d));
}

bool NickCommand::_isValidNick(const std::string &name) {
  if (name.length() > 9 || name.empty())
    return false;
  if (_isLetter(*name.begin()) == false && _isSpecial(*name.begin()) == false)
    return false;
  for (std::string::const_iterator it = name.begin() + 1; it != name.end();
       ++it) {
    if (!_isLetter(*it) && !_isDigit(*it) && !_isSpecial(*it) && *it != '-')
      return false;
  }
  return true;
}

bool NickCommand::execute(CommandContext &ctx) {
  if (ctx.params().empty()) {
    ctx.reply(ReplyBuilder::errNoNicknameGiven());
    return true;
  }

  std::string newNick = ctx.params()[0];

  if (_isValidNick(newNick) == false) {
    ctx.reply(ReplyBuilder::erroneusNickName(newNick));
    return true;
  }

  if (_serverCtx.hasNick(newNick, ctx.client())) {
    ctx.reply(ReplyBuilder::errNickNameInUse(newNick));
    return true;
  }

  ctx.client().setNickName(newNick);
  _serverCtx.tryCompleteRegistration(ctx.client());
  return true;
}
