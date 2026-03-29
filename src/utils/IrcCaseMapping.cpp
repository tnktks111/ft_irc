#include "IrcCaseMapping.hpp"
#include <algorithm>
#include <string>

namespace IrcCaseMapping {
char normalizeChar(char c) {
  if (c >= 'A' && c <= 'Z') {
    return c + ('a' - 'A');
  }
  switch (c) {
    case '[':
      return '{';
    case ']':
      return '}';
    case '\\':
      return '|';
    case '~':
      return '^';
    default:
      return c;
  }
}

std::string normalize(const std::string& value) {
  std::string normalized = value;
  std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                 normalizeChar);
  return normalized;
}

bool equals(const std::string& lhs, const std::string& rhs) {
  return normalize(lhs) == normalize(rhs);
}
}  // namespace IrcCaseMapping
