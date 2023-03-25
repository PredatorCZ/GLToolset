// Defines
// INSTANCED: Instanced draw call
// VS_NUMBONES x: 1 = no skinning, used as model space transform
//                >1 = skinning, max indexed transforms, recommended (pow 2, up to 0x40)
// VS_NUMUVS2: Num used vec2 texcoords
// VS_NUMUVS4: Num used vec4 texcoords

#include "common.h.glsl"
#include "dual_quat.glsl"

#ifndef VS_NUMBONES
#define VS_NUMBONES 1
#endif

struct Transform {
    vec4 indexedModel[2];
    vec3 indexedInflate;
};

#ifdef INSTANCED
readonly buffer InstanceTransforms {
#ifdef VS_NUMUVTMS
  vec4 uvTransform[VS_NUMUVTMS];
#endif
  Transform transforms[];
};

vec3 GetSingleModelTransform(vec4 pos, int index) {
    const int instanceStride = VS_NUMBONES;
    return DQTransformPoint(transforms[gl_InstanceID * instanceStride + index].indexedModel, pos.xyz * transforms[gl_InstanceID * instanceStride + index].indexedInflate);
}
#else
uniform ubInstanceTransforms {
#ifdef VS_NUMUVTMS
  vec4 uvTransform[VS_NUMUVTMS];
#endif
  Transform transforms[VS_NUMBONES];
};

vec3 GetSingleModelTransform(vec4 pos, int index) {
    return DQTransformPoint(transforms[index].indexedModel, pos.xyz * transforms[index].indexedInflate);
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

#ifdef VS_NUMUVS2
#include "uvs.vert"
#endif

#if defined(TS_NORMAL_ATTR) || defined(TS_TANGENT_ATTR)
#include "ts_normal.vert"
#endif

#ifdef NUM_LIGHTS
#include "light_omni.vert"
#endif
