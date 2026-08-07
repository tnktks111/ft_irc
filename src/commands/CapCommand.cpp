#include "CapCommand.hpp"

CapCommand::CapCommand(ServerContext &serverCtx) : _serverCtx(serverCtx) {}
CapCommand::~CapCommand() {}

bool CapCommand::execute(CommandContext &ctx) {
  // 登録前でも受け付ける (IRCv3): dispatcher の pre-registration list に
  // CAP が入っている前提。ここでは subcommand ごとに最小応答を返す。
  //
  // target は client の nick が確定していれば nick、未確定なら "*"。
  // 三項演算子の両辺を統一するため値で受ける (const-ref だと一方が
  // 一時オブジェクトになり、後々のリファクタで dangling を招きうる)。
  std::string target = ctx.nick().empty() ? "*" : ctx.nick();

  if (ctx.params().empty()) {
    // 引数無し → 何もせず戻す (無害な no-op)
    return true;
  }

  const std::string &sub = ctx.params()[0];

  if (sub == "LS") {
    // capability list を持たないので常に空を返す
    ctx.reply("CAP " + target + " LS :");
  } else if (sub == "LIST") {
    // client が有効化した capability list も空
    ctx.reply("CAP " + target + " LIST :");
  } else if (sub == "REQ") {
    // どの拡張も提供しないため NAK で返す。IRCv3 は :cap1 cap2 trailing 形式を
    // 想定するが、非 trailing の `CAP REQ cap1 cap2` にも耐えるよう
    // params[1..] を空白 join して echo する。
    std::string requested;
    for (std::size_t i = 1; i < ctx.params().size(); ++i) {
      if (!requested.empty())
        requested += " ";
      requested += ctx.params()[i];
    }
    ctx.reply("CAP " + target + " NAK :" + requested);
  }
  // CAP END / 未知 subcommand は silent。
  // 厳密には未知 subcommand は 410 ERR_INVALIDCAPCMD が推奨 (IRCv3) だが、
  // silent の方が寛容な client 実装に馴染むためこれを採用する。
  return true;
}
