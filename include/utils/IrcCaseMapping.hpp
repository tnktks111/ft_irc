#ifndef IRCCASEMAPPING_HPP
#define IRCCASEMAPPING_HPP
#include <string>

namespace IrcCaseMapping {
char normalizeChar(char c);
std::string normalize(const std::string& value);
bool equals(const std::string& lhs, const std::string& rhs);
}  // namespace IrcCaseMapping

#endif
