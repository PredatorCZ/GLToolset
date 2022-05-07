#pragma once
#include "datas/supercore.hpp"
#include <glm/gtx/dual_quaternion.hpp>

namespace prime::common {
struct Camera {
  glm::mat4 projection;
  glm::dualquat transform;
};

uint32 AddCamera(uint32 hash, Camera camera);
Camera &GetCamera(uint32 hash);
uint32 LookupCamera(uint32 hash);
uint32 GetUBCamera();
void SetCurrentCamera(uint32 index);
} // namespace prime::common
