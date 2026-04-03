#ifndef HOSTCASEMAPPING_HPP
#define HOSTCASEMAPPING_HPP

#include <string>

class HostCaseMapping {
 private:
  HostCaseMapping();
  ~HostCaseMapping();
  HostCaseMapping(const HostCaseMapping& other);
  HostCaseMapping& operator=(const HostCaseMapping& other);

 public:
  static std::string normalize(const std::string& host);
  static bool equals(const std::string& lhs, const std::string& rhs);
};

#endif
