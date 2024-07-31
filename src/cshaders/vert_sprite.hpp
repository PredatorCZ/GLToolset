#pragma once
#include "camera.hpp"
#include "dualquat.hpp"

struct {
  bool instance = false;
} constexpr vs_feats;

struct {
  vec4 [[disable(vs_feats.instance)]] transform;
  vec4 [[disable(!vs_feats.instance)]] transforms[];
} const instanceTransforms;

void vertex() {
  const vec4 vertices[6] = {
      /**/ //
      {-1, -1, 0, 1},
      {1, -1, 1, 1},
      {1, 1, 1, 0},
      {1, 1, 1, 0},
      {-1, 1, 0, 0},
      {-1, -1, 0, 1},
  };

  static vec2 psTexCoord[1];

  vec4 curVert = vertices[gl_VertexID];
  if constexpr (vs_feats.instance) {
    vec4 tm = instanceTransforms.transforms[gl_InstanceID];
    gl_Position = ubCamera.projection *
                  (vec4(DQTransformPoint(ubCamera.view, tm.xyz), 1.0) +
                   vec4(curVert.xy, 0, 0) * tm.w);
  } else {
    gl_Position =
        ubCamera.projection *
        (vec4(DQTransformPoint(ubCamera.view, instanceTransforms.transform.xyz),
              1.0) +
         vec4(curVert.xy, 0, 0) * instanceTransforms.transform.w);
  }
  psTexCoord[0] = curVert.zw;
}
