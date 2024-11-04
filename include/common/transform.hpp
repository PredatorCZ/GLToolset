#pragma once
#include "core.hpp"
#include <glm/gtx/dual_quaternion.hpp>

namespace prime::common {
struct Transform {
  glm::dualquat tm;
  glm::vec3 inflate;
};
} // namespace prime::common

HASH_CLASS(prime::common::Transform);
HASH_CLASS(glm::vec4);
