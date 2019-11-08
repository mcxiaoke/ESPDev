#include <algorithm>
#include <vector>

namespace extutils {

template <typename T>
void pop_front(std::vector<T>& vec) {
  assert(!vec.empty());
  vec.erase(vec.begin());
}

}  // namespace extutils