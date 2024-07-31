#pragma once
#include "quat.hpp"

struct {
  bool useNormal = false;
  bool useTangent = true;
  bool normalMapUnorm = false;
  bool normalMapDeriveZ = true;
} constexpr ts_feats;

vec3 TransformTQNormal(vec4 qtangent, vec3 point) {
  vec3 realPoint = sign(qtangent.x) * point;
  return QTransformPoint(qtangent, realPoint);
}

vec3 TransformTSNormal(vec3 point) {
  if constexpr (ts_feats.useTangent) {
    static const vec4 inTangent = 1;

    if constexpr (ts_feats.useNormal) {
      static const vec3 inNormal = 2;
      mat3 TBN;
      TBN[2] = inNormal;
      TBN[0] = inTangent.xyz;
      TBN[1] = cross(inTangent.xyz, inNormal) * inTangent.w;
      return transpose(TBN) * point;
    } else {
      return TransformTQNormal(inTangent, point);
    }
  }

  return point;
}

vec3 GetTSNormal(vec3 texel) {
  if constexpr (ts_feats.normalMapUnorm) {
    texel = texel * 2 - 1;
  }

  if constexpr (ts_feats.normalMapDeriveZ) {
    float derived = clamp(dot(texel.xy, texel.xy), 0, 1);
    return vec3(texel.xy, sqrt(1.f - derived));
  }

  return texel;
}
