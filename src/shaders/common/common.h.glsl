#ifdef SHADER
#include "defs.glsl"
const int maxNumIndexedTransforms = VS_NUMBONES;
const int maxNumUVs2 = VS_NUMUVS2;
const int maxNumUVs4 = VS_NUMUVS4;
const int maxNumUVs = maxNumUVs2 + maxNumUVs4 * 2;
#endif

#ifdef VERTEX

CPPATTR(template <size_t maxNumUVs, size_t maxNumIndexedTransforms>)
struct InstanceTransforms {
  vec4 uvTransform[maxNumUVs];
  dquat_arr(indexedModel, maxNumIndexedTransforms);
  vec3 indexedInflate[maxNumIndexedTransforms];
};

uniform ubCamera {
  mat4 projection;
  dquat(view);
};
#endif
