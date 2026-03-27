#include "ReplyBuilder.hpp"

std::string ReplyBuilder::rplWelcome(const std::string &clientName,
                                     const std::string &clientPrefix) {
  return "001 " + clientName + " :Welcome to the Internet Relay Network " +
         clientPrefix;
}

std::string ReplyBuilder::rplChannelModeIs(const std::string &clientName,
                                           const std::string &channelName,
                                           const std::string &mode,
                                           const std::string &modeParams) {
  if (modeParams.empty()) {
    return "324 " + clientName + " " + channelName + " " + mode;
  } else {
    return "324 " + clientName + " " + channelName + " " + mode + " " +
           modeParams;
  }
}

std::string ReplyBuilder::rplNoTopic(const std::string &clientName,
                                     const std::string &channelName) {
  return "331 " + clientName + " " + channelName + " :No topic is set";
}

std::string ReplyBuilder::rplTopic(const std::string &clientName,
                                   const std::string &channelName,
                                   const std::string &topic) {
  return "332 " + clientName + " " + channelName + " :" + topic;
}

std::string ReplyBuilder::rplInviting(const std::string &clientName,
                                      const std::string &channelName,
                                      const std::string &targetNick) {
  return "341 " + clientName + " " + targetNick + " " + channelName;
}

std::string ReplyBuilder::rplNamReply(const std::string &clientName,
                                      const std::string &channelName,
                                      const std::string &namesList) {
  return "353 " + clientName + " = " + channelName + " :" + namesList;
}

std::string ReplyBuilder::rplEndOfNames(const std::string &clientName,
                                        const std::string &channelName) {
  return "366 " + clientName + " " + channelName + " :End of /NAMES list";
}

std::string ReplyBuilder::errNoSuchNick(const std::string &clientName,
                                        const std::string &target) {
  return "401 " + clientName + " " + target + " :No such nick/channel";
}

std::string ReplyBuilder::errNoSuchChannel(const std::string &clientName,
                                           const std::string &channelName) {
  return "403 " + clientName + " " + channelName + " :No such channel";
}

std::string ReplyBuilder::errUnknownCommand(const std::string &command) {
  return "421 " + command + " :Unknown command";
}

std::string ReplyBuilder::errCantSendToChannel(const std::string &clientName,
                                               const std::string &channelName) {
  return "404 " + clientName + " " + channelName + " :Cannot send to channel";
}

std::string ReplyBuilder::errNoRecipient(const std::string &command) {
  return "411 :No recipient given (" + command + ")";
}

std::string ReplyBuilder::errNoOrigin(const std::string &clientName) {
  return "409 " + clientName + " :No origin specified";
}

std::string ReplyBuilder::errNoTextToSend(const std::string &clientName) {
  return "412 " + clientName + " :No text to send";
}

std::string ReplyBuilder::errNoNicknameGiven() {
  return "431 * :No nickname given";
}

std::string ReplyBuilder::errNickNameInUse(const std::string &nickName) {
  return "433 * " + nickName + " :Nickname is already in use";
}

std::string ReplyBuilder::errUserNotInChannel(const std::string &clientName,
                                              const std::string &targetNick,
                                              const std::string &channelName) {
  return "441 " + clientName + " " + targetNick + " " + channelName +
         " :They aren't on that channel";
}

std::string ReplyBuilder::errNotOnChannel(const std::string &clientName,
                                          const std::string &channelName) {
  return "442 " + clientName + " " + channelName +
         " :You're not on that channel";
}

std::string ReplyBuilder::errUserOnChannel(const std::string &clientName,
                                           const std::string &targetNick,
                                           const std::string &channelName) {
  return "443 " + clientName + " " + targetNick + " " + channelName +
         " :is already on channel";
}

std::string ReplyBuilder::errNotRegistered() {
  return "451 * :You have not registered";
}

std::string ReplyBuilder::errNeedMoreParams(const std::string &clientName,
                                            const std::string &command) {
  return "461 " + clientName + " " + command + " :Not enough parameters";
}

std::string ReplyBuilder::errAlreadyRegistered(const std::string &clientName) {
  return "462 " + clientName + " :You may not reregister";
}

std::string ReplyBuilder::errIncorrectPassword() {
  return "464 * :Password incorrect";
}

std::string ReplyBuilder::errChannelIsFull(const std::string &clientName,
                                           const std::string &channelName) {
  return "471 " + clientName + " " + channelName + " :Cannot join channel (+l)";
}

std::string ReplyBuilder::errUnknownMode(const std::string &clientName, char c,
                                         const std::string &chName) {
  return "472 " + clientName + " " + std::string(1, c) +
         " :is unknown mode char to me for " + chName;
}

std::string ReplyBuilder::errInviteOnlyChan(const std::string &clientName,
                                            const std::string &channelName) {
  return "473 " + clientName + " " + channelName + " :Cannot join channel (+i)";
}

std::string ReplyBuilder::errBadChannelKey(const std::string &clientName,
                                           const std::string &channelName) {
  return "475 " + clientName + " " + channelName + " :Cannot join channel (+k)";
}

std::string ReplyBuilder::errChanOPrivsNeeded(const std::string &clientName,
                                              const std::string &channelName) {
  return "482 " + clientName + " " + channelName +
         " :You're not channel operator";
}
