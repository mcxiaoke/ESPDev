#ifndef _EXT_UTILS_HPP_
#define _EXT_UTILS_HPP_

#include <algorithm>
#include <memory>
#include <vector>

namespace ext {

namespace utils {

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

}  // namespace utils
}  // namespace ext

#endif