#ifndef REPLYBUILDER_HPP
#define REPLYBUILDER_HPP

#include <string>

class ReplyBuilder {

 private:
  ReplyBuilder();
  ~ReplyBuilder();
  ReplyBuilder(const ReplyBuilder& other);
  ReplyBuilder& operator=(const ReplyBuilder& other);

 public:
  static std::string rplWelcome(const std::string& clientName,
                                const std::string& clientPrefix);
  static std::string rplChannelModeIs(const std::string& clientName,
                                      const std::string& channelName,
                                      const std::string& mode,
                                      const std::string& modeParams);
  static std::string rplNoTopic(const std::string& clientName,
                                const std::string& channelName);
  static std::string rplTopic(const std::string& clientName,
                              const std::string& channelName,
                              const std::string& topic);
  static std::string rplInviting(const std::string& clientName,
                                 const std::string& channelName,
                                 const std::string& targetNick);
  static std::string rplNamReply(const std::string& clientName,
                                 const std::string& channelName,
                                 const std::string& namesList);
  static std::string rplEndOfNames(const std::string& clientName,
                                   const std::string& channelName);
  static std::string rplWhoReply(
      const std::string& clientName, const std::string& channelName,
      const std::string& userName, const std::string& hostName,
      const std::string& serverName, const std::string& nickName,
      const std::string& status, const std::string& hopcount,
      const std::string& realName);
  static std::string rplEndOfWho(const std::string& clientName,
                                 const std::string& name);
  static std::string errNoSuchNick(const std::string& clientName,
                                   const std::string& target);
  static std::string errNoSuchChannel(const std::string& clientName,
                                      const std::string& channelName);
  static std::string errTooManyTargets(const std::string& clientName,
                                       const std::string& target,
                                       const std::string& errCode,
                                       const std::string& abortMsg);
  static std::string errUnknownCommand(const std::string& command);
  static std::string errNoRecipient(const std::string& command);
  static std::string errNoOrigin(const std::string& clientName);
  static std::string errNoTextToSend(const std::string& clientName);
  static std::string errNoNicknameGiven();
  static std::string erroneusNickName(const std::string& nickName);
  static std::string errNickNameInUse(const std::string& nickName);
  static std::string errCantSendToChannel(const std::string& clientName,
                                          const std::string& channelName);
  static std::string errNeedMoreParams(const std::string& clientName,
                                       const std::string& command);
  static std::string errUserNotInChannel(const std::string& clientName,
                                         const std::string& targetNick,
                                         const std::string& channelName);
  static std::string errNotOnChannel(const std::string& clientName,
                                     const std::string& channelName);
  static std::string errUserOnChannel(const std::string& clientName,
                                      const std::string& targetNick,
                                      const std::string& channelName);
  static std::string errNotRegistered();
  static std::string errAlreadyRegistered(const std::string& clientName);
  static std::string errChanOPrivsNeeded(const std::string& clientName,
                                         const std::string& channelName);
  static std::string errChannelIsFull(const std::string& clientName,
                                      const std::string& channelName);
  static std::string errUnknownMode(const std::string& clientName, char c,
                                    const std::string& chName);
  static std::string errBadChannelKey(const std::string& clientName,
                                      const std::string& channelName);
  static std::string errInviteOnlyChan(const std::string& clientName,
                                       const std::string& channelName);
  static std::string errKeySet(const std::string& channelName);
  static std::string errIncorrectPassword();
};

#endif
