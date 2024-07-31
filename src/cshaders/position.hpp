#pragma once
#include "camera.hpp"
#include "dualquat.hpp"

constexpr uint numBones = 1;

struct Transforms {
  vec4 indexedModel[numBones][2];
  vec3 indexedInflate[numBones];
};

vec3 GetSingleModelTransform(vec4 pos, int index) {
  static const Transforms [[instanced]] transform;

  return DQTransformPoint(transform.indexedModel[index],
                          pos.xyz * transform.indexedInflate[index]);
}

static const vec4 inPos = 0;

struct {
  uint wt4BeginIndex;
  uint wt8BeginIndex;
  uint wt8BaseIndex;
  uint data[];
} const weightData;

uvec4 unpackUint4x8(uint input) {
  return (input.xxxx >> uvec4(0, 8, 16, 24)) & 0xffu;
}

vec3 Get4BoneTm(uint baseIndex, vec4 modelSpace) {
  vec4 weights = unpackUnorm4x8(weightData.data[baseIndex + 1]);
  uvec4 bones = unpackUint4x8(weightData.data[baseIndex]);

  vec3 tm0 = GetSingleModelTransform(modelSpace, bones.x) * weights.x;
  vec3 tm1 = GetSingleModelTransform(modelSpace, bones.y) * weights.y;
  vec3 tm2 = GetSingleModelTransform(modelSpace, bones.z) * weights.z;
  vec3 tm3 = GetSingleModelTransform(modelSpace, bones.w) * weights.w;

  return tm0 + tm1 + tm2 + tm3;
}

vec3 GetModelSpace(vec4 modelSpace) {
  if constexpr (numBones > 1) {
    uint index = modelSpace.w;
    if (index < numBones) {
      return GetSingleModelTransform(modelSpace, index);
    } else if (index < weightData.wt4BeginIndex) {
      uvec4 wt = unpackUint4x8(weightData.data[index - numBones]);
      uvec2 bones = wt.xy;
      vec2 weights = vec2(wt.zw) * (1 / 255.f);
      vec3 tm0 = GetSingleModelTransform(modelSpace, bones.x) * weights.x;
      vec3 tm1 = GetSingleModelTransform(modelSpace, bones.y) * weights.y;

      return tm0 + tm1;
    } else if (index < weightData.wt8BeginIndex) {
      uint baseIndex = (index - weightData.wt4BeginIndex) * 2;
      return Get4BoneTm(baseIndex, modelSpace);
    } else {
      uint baseIndex =
          weightData.wt8BaseIndex + (index - weightData.wt8BeginIndex) * 4;
      return Get4BoneTm(baseIndex, modelSpace) +
             Get4BoneTm(baseIndex + 2, modelSpace);
    }
  } else {
    return GetSingleModelTransform(modelSpace, 0);
  }
}

void SetPosition(vec3 pos) {
  vec3 viewSpace = DQTransformPoint(ubCamera.view, pos);
  gl_Position = ubCamera.projection * vec4(viewSpace, 1.0);
}
