#ifndef __MCX_EXT_UTILS_HEADER__
#define __MCX_EXT_UTILS_HEADER__

#include <Arduino.h>

#include <algorithm>
#include <cstdio>
#include <memory>
#include <vector>

#include "convert.hpp"

namespace ext {

template <typename T>
void pop_front(std::vector<T>& vec) {
  assert(!vec.empty());
  vec.erase(vec.begin());
}

// port from c++ 14
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename... Args>
int strPrintf(char* const buffer, size_t const bufferCount,
              char const* const format, Args const&... args) noexcept {
  int const result =
      snprintf(buffer, bufferCount, format, ext::ArgConvert(args)...);
  return result;
}

template <typename... Args>
std::string strFormat(char const* const format, Args const&... args) {
  std::string buffer;
  size_t const size = strPrintf(nullptr, 0, format, args...);
  buffer.resize(size);
  snprintf(&buffer[0], buffer.size() + 1, format, ext::ArgConvert(args)...);
  return buffer;
}

// https://stackoverflow.com/questions/2342162
/**
 * https://stackoverflow.com/questions/4445654
 * Intuitively, when you fully specialize something, it doesn't depend on a
 * template parameter any more -- so unless you make the specialization
 * inline, you need to put it in a .cpp file instead of a .h or you end up
 * violating the one definition rule as David says. Note that when you
 * partially specialize templates, the partial specializations do still
 * depend on one or more template parameters, so they still go in a .h file.
 * */

inline std::string strFormat2(const std::string fmt_str, ...) {
  /* Reserve two times as much as the length of the fmt_str */
  int n = static_cast<int>(fmt_str.size()) * 2;
  std::unique_ptr<char[]> formatted;
  va_list ap;
  while (1) {
    /* Wrap the plain char array into the unique_ptr */
    formatted.reset(new char[n]);
    strcpy(formatted.get(), fmt_str.c_str());
    va_start(ap, fmt_str);
    auto final_n = vsnprintf(formatted.get(), n, fmt_str.c_str(), ap);
    va_end(ap);
    if (final_n < 0 || final_n >= n)
      n += abs(final_n - n + 1);
    else
      break;
  }
  return std::string(formatted.get());
}

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

}  // namespace ext

#endif /* __MCX_EXT_UTILS_HEADER__ */
