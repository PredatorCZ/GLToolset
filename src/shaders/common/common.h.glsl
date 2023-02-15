#pragma once
#ifdef SHADER
#include "defs.glsl"
#ifdef VS_NUMBONES
const int maxNumIndexedTransforms = VS_NUMBONES;
#else
const int maxNumIndexedTransforms = 1;
#endif
#ifdef VS_NUMUVS2
#ifndef VS_NUMUVS4
#define VS_NUMUVS4 0
#endif
const int maxNumUVs2 = VS_NUMUVS2;
const int maxNumUVs4 = VS_NUMUVS4;
const int maxNumUVs = maxNumUVs2 + maxNumUVs4 * 2;
#else
const int maxNumUVs = 1;
#endif
#endif

#ifdef VERTEX

CPPATTR(template <size_t maxNumUVs, size_t maxNumIndexedTransforms>)
struct InstanceTransforms {
  vec4 uvTransform[maxNumUVs];
  dquat_arr(indexedModel, maxNumIndexedTransforms);
  vec3 indexedInflate[maxNumIndexedTransforms];
};

#endif

#if defined(VERTEX) || defined(GEOMETRY)

uniform ubCamera {
  mat4 projection;
  dquat(view);
};

#endif
