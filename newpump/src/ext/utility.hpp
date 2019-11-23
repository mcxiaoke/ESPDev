#ifndef _EXT_UTILS_H_
#define _EXT_UTILS_H_

#include <algorithm>
#include <vector>

namespace extutils {

template <typename T>
void pop_front(std::vector<T>& vec) {
  assert(!vec.empty());
  vec.erase(vec.begin());
}

// port from c++ 14
template <typename T, typename... Ts>
std::unique_ptr<T> make_unique(Ts&&... params) {
  return std::unique_ptr<T>(new T(std::forward<Ts>(params)...));
}

}  // namespace extutils

#endif