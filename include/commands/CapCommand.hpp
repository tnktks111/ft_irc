#ifndef CAPCOMMAND_HPP
#define CAPCOMMAND_HPP

#include "ACommand.hpp"
#include "ServerContext.hpp"

// IRCv3 CAP handshake の最小実装。
// 本サーバは IRCv3 拡張を一切サポートしないため、常に空 capability list を
// 返して client の登録シーケンスを妨げないことだけを目的とする。
class CapCommand : public ACommand {
 private:
  ServerContext &_serverCtx;

  CapCommand(const CapCommand &other);
  CapCommand &operator=(const CapCommand &other);

 public:
  CapCommand(ServerContext &serverCtx);
  virtual ~CapCommand();

  virtual bool execute(CommandContext &ctx);
};

#endif
