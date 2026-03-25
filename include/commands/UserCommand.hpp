#ifndef USERCOMMAND_HPP
#define USERCOMMAND_HPP

#include "ACommand.hpp"
#include "ServerContext.hpp"

class UserCommand : public ACommand {
private:
	ServerContext &_serverCtx;

	UserCommand(const UserCommand &other);
	UserCommand &operator=(const UserCommand &other);

public:
	UserCommand(ServerContext &serverCtx);
	virtual ~UserCommand();

	virtual bool execute(CommandContext &ctx);
};

#endif
