#pragma once
#if defined(VERTEX) || defined(GEOMETRY)

layout(binding = 0) uniform ubCamera {
  mat4 projection;
  vec4 view[2];
};

#endif
