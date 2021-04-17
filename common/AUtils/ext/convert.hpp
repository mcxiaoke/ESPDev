#ifndef __MCX_EXT_CONVERT_HEADER__
#define __MCX_EXT_CONVERT_HEADER__
#include <Arduino.h>

#include <vector>

namespace ext {

template <typename T>
T ArgConvert(T value) noexcept {
  return value;
}
template <typename T>
T const* ArgConvert(std::basic_string<T> const& value) noexcept {
  return value.c_str();
}

template <typename T>
const char* ArgConvert(std::vector<T> const& vec) noexcept {
  std::string value("[");
  size_t i = 0;
  size_t size = vec.size();
  for (; i < size - 1; i++) {
    value += ArgConvert(vec[i]);
    value += ",";
  }
  value += vec[i];
  value += "]";
  return value.c_str();
}

inline const char* ArgConvert(String const& value) noexcept {
  return value.c_str();
}

}  // namespace ext

#endif /* __MCX_EXT_CONVERT_HEADER__ */
