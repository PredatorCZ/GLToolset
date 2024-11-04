#pragma once
#include "spike/type/vectors_simd.hpp"
#include "core.hpp"

namespace prime::common {
struct AABB {
  Vector4A16 center;
  Vector4A16 bounds;
};
}; // namespace prime::common

HASH_CLASS(prime::common::AABB);
