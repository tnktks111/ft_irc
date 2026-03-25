#ifndef MODECOMMAND_HPP
#define MODECOMMAND_HPP

#include "ACommand.hpp"
#include "ServerContext.hpp"

class ModeCommand : public ACommand {
private:
	ServerContext &_serverCtx;

	ModeCommand(const ModeCommand &other);
	ModeCommand &operator=(const ModeCommand &other);

public:
	ModeCommand(ServerContext &serverCtx);
	virtual ~ModeCommand();

	virtual bool execute(CommandContext &ctx);
};

#endif
