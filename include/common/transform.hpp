#pragma once
#include <glm/gtx/dual_quaternion.hpp>

namespace prime::common {
    struct Transform {
        glm::dualquat tm;
        glm::vec3 inflate;
    };
}
