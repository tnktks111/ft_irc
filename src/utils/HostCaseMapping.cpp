#include "HostCaseMapping.hpp"
#include <algorithm>
#include <string>

namespace {
char toLowerAscii(char c) {
  if ('A' <= c && c <= 'Z')
    return c - 'A' + 'a';
  return c;
}
}  // namespace

std::string HostCaseMapping::normalize(const std::string& host) {
  std::string normalized = host;
  std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                 toLowerAscii);
  return normalized;
}

bool HostCaseMapping::equals(const std::string& lhs, const std::string& rhs) {
  return normalize(lhs) == normalize(rhs);
}
