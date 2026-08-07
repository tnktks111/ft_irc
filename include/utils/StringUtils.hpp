#ifndef STRINGUTILS_HPP
#define STRINGUTILS_HPP
#include <string>
#include <vector>

namespace StringUtils {

// Split `src` by `delim` into tokens.
// Preserves empty tokens (e.g. "a,,b" → ["a", "", "b"]).
// If `src` ends with `delim`, a trailing empty token is appended too.
// (Callers that want to skip empties should filter the result.)
std::vector<std::string> split(const std::string& src, char delim);

}  // namespace StringUtils

#endif
