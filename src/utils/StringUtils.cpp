#include "StringUtils.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace {

char toUpperChar(char c) {
  return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
}

}  // namespace

namespace StringUtils {

std::vector<std::string> split(const std::string& src, char delim) {
  std::vector<std::string> result;

  if (src.empty())
    return result;

  std::istringstream iss(src);
  std::string item;
  while (std::getline(iss, item, delim)) {
    result.push_back(item);
  }
  // std::getline は末尾に delim が来た場合の trailing empty を落とすので、
  // カンマ区切りリストで末尾要素が空文字なケースを保つために手動で追加する。
  if (src[src.length() - 1] == delim)
    result.push_back("");
  return result;
}

std::string toUpper(const std::string& value) {
  std::string result(value);
  std::transform(result.begin(), result.end(), result.begin(), toUpperChar);
  return result;
}

}  // namespace StringUtils
