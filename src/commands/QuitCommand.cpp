#include "QuitCommand.hpp"

QuitCommand::QuitCommand(ServerContext &serverCtx) : _serverCtx(serverCtx) {}
QuitCommand::~QuitCommand() {}

bool QuitCommand::execute(CommandContext &ctx) {
  std::string reason =
      (ctx.params().empty()) ? "Client Quit" : ctx.params()[0];

  // RFC 2812 §3.7.4: 切断する client 自身に ERROR :Closing Link を返してから
  // 切断する。send buffer に積むだけで、実際の flush は Server 側の
  // _disconnectClient が close の直前に行う。
  ctx.reply("ERROR :Closing Link: " + ctx.client().getHost() + " (Quit: " +
            reason + ")");

  _serverCtx.removeClientFromAllChannels(ctx.client(), reason);
  return false;
}
