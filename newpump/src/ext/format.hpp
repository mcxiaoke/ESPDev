#ifndef _EXT_UTILS_FORMAT_H_
#define _EXT_UTILS_FORMAT_H_

#include <Arduino.h>
#include <algorithm>
#include <cstdio>
#include <memory>
#include <vector>

namespace ext {

namespace format {

template <typename T>
T ArgConvert(T value) noexcept {
  return value;
}
template <typename T>
T const* ArgConvert(std::basic_string<T> const& value) noexcept {
  return value.c_str();
}

const char* ArgConvert(String const& value) noexcept;

template <typename... Args>
int strPrintf(char* const buffer,
              size_t const bufferCount,
              char const* const format,
              Args const&... args) noexcept {
  int const result = snprintf(buffer, bufferCount, format, ArgConvert(args)...);
  return result;
}

template <typename... Args>
std::string strFormat(char const* const format, Args const&... args) {
  std::string buffer;
  size_t const size = strPrintf(nullptr, 0, format, args...);
  buffer.resize(size);
  snprintf(&buffer[0], buffer.size() + 1, format, ArgConvert(args)...);
  return buffer;
}

// https://stackoverflow.com/questions/2342162
std::string strFormat2(const std::string fmt_str, ...);

// https://lonkamikaze.github.io/2016/12/16/cxx-printf-style-formatting
template <size_t BufSize>
class Formatter {
 private:
  char const* const fmt;

 public:
  constexpr Formatter(char const* const fmt) : fmt{fmt} {}

  template <typename... ArgTs>
  std::string operator()(ArgTs const&... args) const {
    char buf[BufSize];
    auto size = snprintf(buf, BufSize, this->fmt, args...);
    if (size < 0) {
      /* encoding error */
      return {};
    } else if (static_cast<size_t>(size) >= BufSize) {
      /* does not fit into buffer */
      return {buf, BufSize - 1};
    }
    return {buf, static_cast<size_t>(size)};
  }
};

}  // namespace format
}  // namespace ext

#endif