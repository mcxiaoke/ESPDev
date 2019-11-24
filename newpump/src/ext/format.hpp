#ifndef _EXT_UTILS_FORMAT_H_
#define _EXT_UTILS_FORMAT_H_

#include <algorithm>
#include <memory>
#include <vector>

namespace ext {

namespace format {

template <typename T>
T Argument(T value) noexcept {
  return value;
}
template <typename T>
T const* Argument(std::basic_string<T> const& value) noexcept {
  return value.c_str();
}

const char* Argument(const String& value) noexcept {
  return value.c_str();
}

template <typename... Args>
int strPrintf(char* const buffer,
              size_t const bufferCount,
              char const* const format,
              Args const&... args) noexcept {
  int const result = snprintf(buffer, bufferCount, format, Argument(args)...);
  return result;
}

template <typename... Args>
std::string strFormat(char const* const format, Args const&... args) {
  std::string buffer;
  size_t const size = strPrintf(nullptr, 0, format, args...);
  buffer.resize(size);
  snprintf(&buffer[0], buffer.size() + 1, format, Argument(args)...);
  return buffer;
}

// https://stackoverflow.com/questions/2342162
std::string strFormat2(const std::string fmt_str, ...) {
  int final_n;
  /* Reserve two times as much as the length of the fmt_str */
  int n = ((int)fmt_str.size()) * 2;
  std::unique_ptr<char[]> formatted;
  va_list ap;
  while (1) {
    /* Wrap the plain char array into the unique_ptr */
    formatted.reset(new char[n]);
    strcpy(&formatted[0], fmt_str.c_str());
    va_start(ap, fmt_str);
    final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
    va_end(ap);
    if (final_n < 0 || final_n >= n)
      n += abs(final_n - n + 1);
    else
      break;
  }
  return std::string(formatted.get());
}

}  // namespace format
}  // namespace ext

#endif