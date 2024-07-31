#pragma once
#include "shader_types.hpp"

vec3 QTransformPoint(vec4 q, vec3 point) {
  vec3 qvec = q.yzw;
  vec3 uv = cross(qvec, point);
  vec3 uuv = cross(qvec, uv);

  return point + ((uv * q.x) + uuv) * 2;
}

