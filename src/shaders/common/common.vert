// Defines
// INSTANCED: Instanced draw call
// VS_NUMBONES x: 1 = no skinning, used as model space transform
//                >1 = skinning, max indexed transforms, recommended (pow 2, up to 0x40)

#include "common.h.glsl"
#include "dual_quat.glsl"

#ifndef VS_NUMBONES
#define VS_NUMBONES 1
#endif

struct CFeats {
  int numBones;
  int tms[3];
};

const CFeats c_feats = CFeats(VS_NUMBONES, int[](1, 2, 3));

struct Transforms {
vec4 indexedModel[c_feats.numBones][2];
vec3 indexedInflate[c_feats.numBones];
};

#ifdef INSTANCED

readonly buffer InstanceTransforms {
#ifdef VS_NUMUVTMS
vec4 uvTransform[VS_NUMUVTMS];
#endif
Transforms transforms[];
};

vec3 GetSingleModelTransform(vec4 pos, int index) {
  return DQTransformPoint(transforms[gl_InstanceID].indexedModel[index], pos.xyz * transforms[gl_InstanceID].indexedInflate[index]);
}

#else
uniform ubInstanceTransforms {
#ifdef VS_NUMUVTMS
vec4 uvTransform[VS_NUMUVTMS];
#endif
Transforms transform;
};

vec3 GetSingleModelTransform(vec4 pos, int index) {
  return DQTransformPoint(transform.transform.indexedModel[index], pos.xyz * transform.transform.indexedInflate[index]);
}

#endif


vec3 GetModelSpace(vec4 modelSpace) {
vec3 finalSpace;
#if VS_NUMBONES > 1
        // todo skin
#else
finalSpace = GetSingleModelTransform(modelSpace, 0);
#endif

return finalSpace;
}

layout(location = 0) in vec4 inPos;

#include "uvs.vert"
#include "ts_normal.vert"

#ifdef NUM_LIGHTS
#include "light_omni.vert"
#endif
