#pragma once
#include "shader_types.hpp"

struct {
  int numUVs = 1;
  int numUVTMs = 1;
  int transformRemaps[1]{0};
} constexpr uv_feats;

constexpr int numUVs4 = uv_feats.numUVs / 2;

void TransformUVs() {
  const bool useUVs2 = (uv_feats.numUVs % 2) == 1;

  static vec2 psTexCoord[uv_feats.numUVs];
  static const vec4 uvTransform[uv_feats.numUVTMs]{};

  if constexpr (numUVs4 > 0) {
    static const vec4 [[location(4)]] inTexCoord4[numUVs4]{};

    for (int i = 0; i < numUVs4; i++) {
      const int tb = i * 2;
      const int remap0 = uv_feats.transformRemaps[tb];
      const int remap1 = uv_feats.transformRemaps[tb + 1];

      if (remap0 < 0) {
        psTexCoord[tb] = inTexCoord4[i].xy;
      } else {
        psTexCoord[tb] =
            uvTransform[remap0].xy + inTexCoord4[i].xy * uvTransform[remap0].zw;
      }

      if (remap1 < 0) {
        psTexCoord[tb + 1] = inTexCoord4[i].zw;
      } else {
        psTexCoord[tb + 1] =
            uvTransform[remap1].xy + inTexCoord4[i].zw * uvTransform[remap1].zw;
      }
    }
  }

  if (useUVs2) {
    const int idx2 = numUVs4 * 2;
    const int remap = uv_feats.transformRemaps[idx2];
    static const vec2 inTexCoord2 = 3;

    if (remap < 0) {
      psTexCoord[idx2] = inTexCoord2;
    } else {
      psTexCoord[idx2] =
          uvTransform[remap].xy + inTexCoord2 * uvTransform[remap].zw;
    }
  }
}
