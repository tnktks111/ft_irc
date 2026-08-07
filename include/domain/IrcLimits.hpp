#ifndef IRCLIMITS_HPP
#define IRCLIMITS_HPP

#include <cstddef>

namespace IrcLimits {

// RFC 2812 §2.3: 1 message は CR-LF を含め 512 byte 上限。
// つまり CRLF を除いた実データは最大 510 byte。
// extractMessage (受信) と ResponseSink::_appendLine (送信) の両方でこの上限を
// 適用し、over-length なペイロードを truncate する。
const std::size_t MAX_MSG_LEN = 510;

}  // namespace IrcLimits

#endif
