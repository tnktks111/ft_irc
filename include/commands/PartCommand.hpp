#ifndef PARTCOMMAND_HPP
#define PARTCOMMAND_HPP

#include "ACommand.hpp"
#include "ServerContext.hpp"

class PartCommand : public ACommand {
private:
	ServerContext &_serverCtx;

	PartCommand(const PartCommand &other);
	PartCommand &operator=(const PartCommand &other);

public:
	PartCommand(ServerContext &serverCtx);
	virtual ~PartCommand();

	virtual bool execute(CommandContext &ctx);
};

#endif
