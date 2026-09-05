#ifndef IRCLIMITS_HPP
#define IRCLIMITS_HPP

#include <cstddef>

namespace IrcLimits {

const std::size_t MAX_MSG_LEN = 510;
const std::size_t MAX_RECV_QUEUE = 8 * 1024;
const std::size_t MAX_SEND_QUEUE = 256 * 1024;

const std::size_t MAX_CLIENTS = 128;
const std::size_t MAX_CHANNELS_PER_CLIENT = 32;

}  // namespace IrcLimits

#endif
