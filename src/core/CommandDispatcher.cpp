#include "CommandDispatcher.hpp"
#include <iostream>
#include "InviteCommand.hpp"
#include "JoinCommand.hpp"
#include "KickCommand.hpp"
#include "ModeCommand.hpp"
#include "NickCommand.hpp"
#include "PartCommand.hpp"
#include "PassCommand.hpp"
#include "PingCommand.hpp"
#include "PrivMsgCommand.hpp"
#include "QuitCommand.hpp"
#include "ReplyBuilder.hpp"
#include "TopicCommand.hpp"
#include "UserCommand.hpp"
#include "WhoCommand.hpp"

CommandDispatcher::CommandDispatcher(ServerContext& serverCtx)
    : _serverCtx(serverCtx) {
  _commands["PASS"] = new PassCommand(_serverCtx);
  _commands["NICK"] = new NickCommand(_serverCtx);
  _commands["USER"] = new UserCommand(_serverCtx);
  _commands["JOIN"] = new JoinCommand(_serverCtx);
  _commands["PRIVMSG"] = new PrivMsgCommand(_serverCtx);
  _commands["PART"] = new PartCommand(_serverCtx);
  _commands["QUIT"] = new QuitCommand(_serverCtx);
  _commands["TOPIC"] = new TopicCommand(_serverCtx);
  _commands["KICK"] = new KickCommand(_serverCtx);
  _commands["INVITE"] = new InviteCommand(_serverCtx);
  _commands["MODE"] = new ModeCommand(_serverCtx);
  _commands["PING"] = new PingCommand(_serverCtx);
  _commands["WHO"] = new WhoCommand(_serverCtx);
}

CommandDispatcher::~CommandDispatcher() {
  for (std::map<std::string, ACommand*>::iterator it = _commands.begin();
       it != _commands.end(); ++it) {
    delete it->second;
  }
}

bool CommandDispatcher::_isPreRegistrationCommand(
    const std::string& command) const {
  return command == "PASS" || command == "NICK" || command == "USER" ||
         command == "QUIT";
}

bool CommandDispatcher::dispatch(CommandContext& ctx) {
  if (!ctx.isRegistered() && !_isPreRegistrationCommand(ctx.command())) {
    ctx.reply(ReplyBuilder::errNotRegistered());
    return true;
  }

  std::map<std::string, ACommand*>::iterator it = _commands.find(ctx.command());
  if (it == _commands.end()) {
    if (ctx.isRegistered()) {
      ctx.reply(ReplyBuilder::errUnknownCommand(ctx.command()));
      std::cout << "Unknown command from authenticated User: " << ctx.command()
                << std::endl;  // [TODO] この出力いらないなら消す
    }
    return true;
  }

  return it->second->execute(ctx);
}
