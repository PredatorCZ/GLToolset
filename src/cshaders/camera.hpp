#pragma once
#include "shader_types.hpp"

struct {
  mat4 projection;
  vec4 view[2];
} const [[binding(0)]] ubCamera;
