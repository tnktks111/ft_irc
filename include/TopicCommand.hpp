#ifndef TOPICCOMMAND_HPP
#define TOPICCOMMAND_HPP

#include "ACommand.hpp"
#include "ServerContext.hpp"

class TopicCommand : public ACommand {
private:
	ServerContext &_serverCtx;

	TopicCommand(const TopicCommand &other);
	TopicCommand &operator=(const TopicCommand &other);

public:
	TopicCommand(ServerContext &serverCtx);
	virtual ~TopicCommand();

	virtual bool execute(CommandContext &ctx);
};

#endif
