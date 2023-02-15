#pragma once
#include <glm/ext.hpp>
#include <glm/gtx/dual_quaternion.hpp>

namespace prime::shaders {

#define vec3 alignas(16) glm::highp_vec3
#define vec4 alignas(16) glm::vec4
#define mat4 alignas(16) glm::mat4
#define dquat_arr(name, numItems) alignas(16) glm::dualquat name[numItems]
#define dquat(name) alignas(16) glm::dualquat name
#define uniform struct
#define FRAGMENT
#define VERTEX
#define CPPATTR(...) __VA_ARGS__
#define CPPATTR2(...) __VA_ARGS__
#define CPPATTR3(...) __VA_ARGS__

#include "common/light_omni.h.glsl"
#include "common/common.h.glsl"

namespace single_texture {
#include "single_texture/main.h.glsl"
}

#undef uniform
#undef FRAGMENT
#undef VERTEX
#undef vec3
#undef vec4
#undef mat4
#undef dquat_arr
#undef dquat
#undef CPPATTR
#undef CPPATTR2
#undef CPPATTR3

} // namespace prime::shaders
