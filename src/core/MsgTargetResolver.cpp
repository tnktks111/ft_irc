#include "MsgTargetResolver.hpp"
#include <set>
#include <sstream>
#include <string>
#include "Channel.hpp"
#include "Client.hpp"

MsgTargetResolver::MsgTargetResolver(ServerContext& serverCtx)
    : _serverCtx(serverCtx) {}

MsgTargetResolver::~MsgTargetResolver() {}

MsgTargetResolution MsgTargetResolver::resolve(
    const std::string& rawTargets) const {
  MsgTargetResolution result;

  std::vector<std::string> tokens = _splitTargets(rawTargets);

  for (std::vector<std::string>::iterator it = tokens.begin();
       it != tokens.end(); ++it) {

    const std::string& token = *it;

    if (token.empty())
      continue;

    MsgTargetEntry entry;
    entry.raw = token;
    entry.result = _resolveOne(token, entry.target);
    result.entries.push_back(entry);
  }
  return result;
}

std::vector<std::string> MsgTargetResolver::_splitTargets(
    const std::string& rawTargets) const {
  std::vector<std::string> tokens;
  std::istringstream iss(rawTargets);
  std::string token;
  std::set<std::string> seen;

  while (std::getline(iss, token, ',')) {
    if (token.empty())
      continue;
    if (seen.insert(token).second) {
      tokens.push_back(token);
    }
  }
  return tokens;
}

MsgTargetResolveResult MsgTargetResolver::_resolveOne(
    const std::string& token, ResolvedMsgTarget& target) const {
  target.raw = token;
  target.kind = MSGTARGET_UNSUPPORTED;
  target.channel = NULL;
  target.client = NULL;

  if (Channel::isChannelPrefix(token[0])) {
    return _resolveChannelToken(token, target);
  } else {
    return _resolveClientToken(token, target);
  }
}

MsgTargetResolveResult MsgTargetResolver::_resolveChannelToken(
    const std::string& token, ResolvedMsgTarget& target) const {
  target.kind = MSGTARGET_CHANNEL;
  if (Channel::isValidChannelName(token) == false)
    return MSGTARGET_RESOLVE_NO_SUCH_CHANNEL;

  Channel* channel = _serverCtx.findChannel(token);

  if (channel == NULL)
    return MSGTARGET_RESOLVE_NO_SUCH_NICK;

  target.channel = channel;
  return MSGTARGET_RESOLVE_OK;
}

MsgTargetResolveResult MsgTargetResolver::_resolveClientToken(
    const std::string& token, ResolvedMsgTarget& target) const {
  target.kind = MSGTARGET_CLIENT;
  if (_isUserHostFormat(token)) {
    return _resolveUserHost(token, target);
  }
  if (_isNickMaskFormat(token))
    return _resolveNickMask(token, target);
  return _resolveNickOnly(token, target);
}

bool MsgTargetResolver::_isUserHostFormat(const std::string& token) {
  std::string::size_type delimPos = token.find('%');

  if (delimPos == std::string::npos)
    return false;
  if (delimPos == 0 || delimPos == token.length() - 1)
    return false;
  return (Client::isValidUserName(token.substr(0, delimPos)) &&
          Client::isValidHost(token.substr(delimPos + 1)));
}

bool MsgTargetResolver::_isNickMaskFormat(const std::string& token) {
  std::string::size_type delimPos1 = token.find('!');
  std::string::size_type delimPos2 = token.find('@');
  if (delimPos1 == std::string::npos || delimPos2 == std::string::npos ||
      delimPos1 > delimPos2)
    return false;

  std::string nick = token.substr(0, delimPos1);
  std::string user = token.substr(delimPos1 + 1, delimPos2 - delimPos1 - 1);
  std::string host = token.substr(delimPos2 + 1);
  return Client::isValidNickName(nick) && Client::isValidUserName(user) &&
         Client::isValidHost(host);
}

MsgTargetResolveResult MsgTargetResolver::_resolveUserHost(
    const std::string& token, ResolvedMsgTarget& target) const {
  std::string::size_type delimPos = token.find('%');
  std::string user = token.substr(0, delimPos);
  std::string host = token.substr(delimPos + 1);

  std::vector<Client*> candidates =
      _serverCtx.findClientsByUserHost(user, host);

  if (candidates.empty())
    return MSGTARGET_RESOLVE_NO_SUCH_NICK;

  if (candidates.size() > 1)
    return MSGTARGET_RESOLVE_AMBIGUOUS;

  target.kind = MSGTARGET_CLIENT;
  target.client = candidates[0];
  return MSGTARGET_RESOLVE_OK;
}

MsgTargetResolveResult MsgTargetResolver::_resolveNickMask(
    const std::string& token, ResolvedMsgTarget& target) const {
  std::string::size_type delimPos1 = token.find('!');
  std::string::size_type delimPos2 = token.find('@');

  std::string nick = token.substr(0, delimPos1);
  std::string user = token.substr(delimPos1 + 1, delimPos2 - delimPos1 - 1);
  std::string host = token.substr(delimPos2 + 1);

  Client* client = _serverCtx.findClientByNickMask(nick, user, host);

  if (client == NULL)
    return MSGTARGET_RESOLVE_NO_SUCH_NICK;

  target.kind = MSGTARGET_CLIENT;
  target.client = client;
  return MSGTARGET_RESOLVE_OK;
}

MsgTargetResolveResult MsgTargetResolver::_resolveNickOnly(
    const std::string& token, ResolvedMsgTarget& target) const {
  Client* client = _serverCtx.findClientByNick(token);
  if (client == NULL)
    return MSGTARGET_RESOLVE_NO_SUCH_NICK;
  target.client = client;
  return MSGTARGET_RESOLVE_OK;
}
