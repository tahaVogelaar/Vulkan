#pragma once
#include <glm/gtx/hash.hpp> // provides std::hash<glm::vec2/vec3>
#include <functional>

namespace lve {

  template <typename T, typename... Rest>
  void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
    seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    if constexpr (sizeof...(rest) > 0)
      hashCombine(seed, rest...);
  }

} // namespace lve
