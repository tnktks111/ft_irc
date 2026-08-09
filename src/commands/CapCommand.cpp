#include "CapCommand.hpp"

CapCommand::CapCommand(ServerContext &serverCtx) : _serverCtx(serverCtx) {}
CapCommand::~CapCommand() {}

bool CapCommand::execute(CommandContext &ctx) {
  std::string target = ctx.nick().empty() ? "*" : ctx.nick();

  if (ctx.params().empty()) {
    return true;
  }

  const std::string &sub = ctx.params()[0];

  if (sub == "LS") {
    ctx.reply("CAP " + target + " LS :");
  } else if (sub == "LIST") {
    ctx.reply("CAP " + target + " LIST :");
  } else if (sub == "REQ") {
    std::string requested;
    for (std::size_t i = 1; i < ctx.params().size(); ++i) {
      if (!requested.empty())
        requested += " ";
      requested += ctx.params()[i];
    }
    ctx.reply("CAP " + target + " NAK :" + requested);
  }
  return true;
}
