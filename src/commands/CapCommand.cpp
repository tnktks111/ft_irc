#include "CapCommand.hpp"
#include "StringUtils.hpp"

CapCommand::CapCommand(ServerContext& serverCtx) : _serverCtx(serverCtx) {}
CapCommand::~CapCommand() {}

bool CapCommand::execute(CommandContext& ctx) {
  std::string target = ctx.nick().empty() ? "*" : ctx.nick();

  if (ctx.params().empty()) {
    return true;
  }

  const std::string sub = StringUtils::toUpper(ctx.params()[0]);

  if (sub == "LS") {
    if (!ctx.isRegistered())
      ctx.client().setCapNegotiating(true);
    ctx.reply("CAP " + target + " LS :");
  } else if (sub == "LIST") {
    ctx.reply("CAP " + target + " LIST :");
  } else if (sub == "REQ") {
    if (!ctx.isRegistered())
      ctx.client().setCapNegotiating(true);

    std::string requested;
    for (std::size_t i = 1; i < ctx.params().size(); ++i) {
      if (!requested.empty())
        requested += " ";
      requested += ctx.params()[i];
    }
    ctx.reply("CAP " + target + " NAK :" + requested);
  } else if (sub == "END") {
    if (!ctx.isRegistered()) {
      ctx.client().setCapNegotiating(false);
      _serverCtx.tryCompleteRegistration(ctx.client());
    }
  }
  return true;
}
