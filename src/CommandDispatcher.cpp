#include "../include/CommandDispatcher.hpp"
#include "../include/InviteCommand.hpp"
#include "../include/JoinCommand.hpp"
#include "../include/KickCommand.hpp"
#include "../include/ModeCommand.hpp"
#include "../include/NickCommand.hpp"
#include "../include/PartCommand.hpp"
#include "../include/PassCommand.hpp"
#include "../include/PingCommand.hpp"
#include "../include/PrivMsgCommand.hpp"
#include "../include/QuitCommand.hpp"
#include "../include/ReplyBuilder.hpp"
#include "../include/TopicCommand.hpp"
#include "../include/UserCommand.hpp"
#include <iostream>

CommandDispatcher::CommandDispatcher(ServerContext &serverCtx)
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
}

CommandDispatcher::~CommandDispatcher() {
  for (std::map<std::string, ACommand *>::iterator it = _commands.begin();
       it != _commands.end(); ++it) {
    delete it->second;
  }
}

bool CommandDispatcher::_isPreRegistrationCommand(
    const std::string &command) const {
  return command == "PASS" || command == "NICK" || command == "USER" ||
         command == "QUIT";
}

bool CommandDispatcher::dispatch(CommandContext &ctx) {
  if (!ctx.isRegistered() && !_isPreRegistrationCommand(ctx.command())) {
    ctx.reply(ReplyBuilder::errNotRegistered());
    return true;
  }

  std::map<std::string, ACommand *>::iterator it =
      _commands.find(ctx.command());
  if (it == _commands.end()) {
    if (ctx.isRegistered()) {
      std::cout << "Command from an authenticated User: " << ctx.command()
                << std::endl;
    }
    return true;
  }

  return it->second->execute(ctx);
}
