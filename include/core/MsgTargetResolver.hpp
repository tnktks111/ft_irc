#ifndef MSGTARGETRESOLVER_HPP
#define MSGTARGETRESOLVER_HPP

#include <string>
#include <vector>
#include "ServerContext.hpp"

class Channel;
class Client;

enum MsgTargetResolveResult {
  MSGTARGET_RESOLVE_OK,
  MSGTARGET_RESOLVE_AMBIGUOUS,
  MSGTARGET_RESOLVE_NO_SUCH_CHANNEL,
  MSGTARGET_RESOLVE_NO_SUCH_NICK
};

enum MsgTargetKind {
  MSGTARGET_CHANNEL,
  MSGTARGET_CLIENT,
  MSGTARGET_UNSUPPORTED
};

struct ResolvedMsgTarget {
  MsgTargetKind kind;
  std::string raw;
  Channel* channel;
  Client* client;
};

struct MsgTargetEntry {
  std::string raw;
  MsgTargetResolveResult result;
  ResolvedMsgTarget target;
};

struct MsgTargetResolution {
  std::vector<MsgTargetEntry> entries;
};

class MsgTargetResolver {
 public:
  MsgTargetResolver(ServerContext& serverCtx);
  ~MsgTargetResolver();
  MsgTargetResolution resolve(const std::string& rawTargets) const;

 private:
  MsgTargetResolver(const MsgTargetResolver& other);
  MsgTargetResolver& operator=(const MsgTargetResolver& other);

  std::vector<std::string> _splitTargets(const std::string& rawTargets) const;
  MsgTargetResolveResult _resolveOne(const std::string& token,
                                     ResolvedMsgTarget& target) const;

  MsgTargetResolveResult _resolveChannelToken(const std::string& token,
                                              ResolvedMsgTarget& target) const;

  MsgTargetResolveResult _resolveClientToken(const std::string& token,
                                             ResolvedMsgTarget& target) const;

  static bool _isUserHostFormat(const std::string& token);
  static bool _isNickMaskFormat(const std::string& token);

  MsgTargetResolveResult _resolveUserHost(const std::string& token,
                                          ResolvedMsgTarget& target) const;
  MsgTargetResolveResult _resolveNickMask(const std::string& token,
                                          ResolvedMsgTarget& target) const;
  MsgTargetResolveResult _resolveNickOnly(const std::string& token,
                                          ResolvedMsgTarget& target) const;

  ServerContext& _serverCtx;
};

#endif
