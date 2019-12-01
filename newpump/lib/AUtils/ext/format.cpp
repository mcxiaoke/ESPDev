#include "format.hpp"

namespace ext {

namespace format {

/**
 * https://stackoverflow.com/questions/4445654
 * Intuitively, when you fully specialize something, it doesn't depend on a
 * template parameter any more -- so unless you make the specialization
 * inline, you need to put it in a .cpp file instead of a .h or you end up
 * violating the one definition rule as David says. Note that when you
 * partially specialize templates, the partial specializations do still
 * depend on one or more template parameters, so they still go in a .h file.
 * */
const char* ArgConvert(String const& value) noexcept {
  return value.c_str();
}

std::string strFormat2(const std::string fmt_str, ...) {
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
}  // namespace format
}  // namespace ext