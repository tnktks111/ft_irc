#include "UserCommand.hpp"
#include <iostream>
#include <sstream>
#include "ReplyBuilder.hpp"

UserCommand::UserCommand(ServerContext& serverCtx) : _serverCtx(serverCtx) {}
UserCommand::~UserCommand() {}

namespace {
bool stringToUint(const std::string& str, unsigned int& result) {
  std::istringstream iss(str);
  char c;

  if (!(iss >> result) || (iss >> c)) {
    return false;
  }
  return true;
}
}  // namespace

// USER <user> <mode> <unused> <realname>
bool UserCommand::execute(CommandContext& ctx) {
  if (ctx.isRegistered()) {
    ctx.reply(ReplyBuilder::errAlreadyRegistered(ctx.nick()));
    return true;
  }
  if (ctx.params().size() < 4) {
    ctx.reply(ReplyBuilder::errNeedMoreParams("*", "USER"));
    return true;
  }

  const std::string& userName = ctx.params()[0];
  const std::string& modeStr = ctx.params()[1];
  const std::string& realName = ctx.params()[3];

  if (Client::isValidUserName(userName) == false) {
    std::cout << "Warning: USER command skipped. (invalid username: "
              << userName << ")" << std::endl;
    return true;
  }

  unsigned int modeVal;
  if (stringToUint(modeStr, modeVal) == false)
    modeVal = 0;
  if (modeVal & MODE_W)
    ctx.client().addMode(UMODE_WALLO);
  if (modeVal & MODE_I)
    ctx.client().addMode(UMODE_INVIS);

  ctx.client().setUserName(userName);
  ctx.client().setRealName(realName);
  _serverCtx.tryCompleteRegistration(ctx.client());
  return true;
}
