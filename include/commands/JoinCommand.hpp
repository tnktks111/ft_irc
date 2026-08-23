#ifndef JOINCOMMAND_HPP
#define JOINCOMMAND_HPP

#include "ACommand.hpp"
#include "ServerContext.hpp"
#include <string>
#include <vector>

class JoinCommand : public ACommand {
private:
	ServerContext &_serverCtx;

	JoinCommand(const JoinCommand &other);
	JoinCommand &operator=(const JoinCommand &other);

	static std::vector<std::string> _generateChannelMemberChunks(
	    const Channel &channel, std::size_t maxChunkLen);

public:
	JoinCommand(ServerContext &serverCtx);
	virtual ~JoinCommand();

	virtual bool execute(CommandContext &ctx);
};

#endif
