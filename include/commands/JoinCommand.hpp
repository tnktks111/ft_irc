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

	// member 一覧を "@op user1 user2 …" 形式で並べた文字列を
	// maxChunkLen byte 以内の chunk に分割して返す。
	// 各 chunk は space 区切り。大規模 channel の RPL_NAMREPLY (353) が
	// 512 byte overflow して truncate されないよう複数 line 分割 (#84)。
	static std::vector<std::string> _generateChannelMemberChunks(
	    const Channel &channel, std::size_t maxChunkLen);

public:
	JoinCommand(ServerContext &serverCtx);
	virtual ~JoinCommand();

	virtual bool execute(CommandContext &ctx);
};

#endif
