#include "datas/vectors_simd.hpp"

namespace prime::common {
struct AABB {
  Vector4A16 center;
  Vector4A16 bounds;
};
}; // namespace prime::common
