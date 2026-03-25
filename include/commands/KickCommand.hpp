#ifndef KICKCOMMAND_HPP
#define KICKCOMMAND_HPP

#include "ACommand.hpp"
#include "ServerContext.hpp"

class KickCommand : public ACommand {
private:
	ServerContext &_serverCtx;

	KickCommand(const KickCommand &other);
	KickCommand &operator=(const KickCommand &other);

public:
	KickCommand(ServerContext &serverCtx);
	virtual ~KickCommand();

	virtual bool execute(CommandContext &ctx);
};

#endif
