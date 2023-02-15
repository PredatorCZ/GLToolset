// Defines
// VS_INSTANCED: Instanced draw call
// VS_POSTYPE: Vertex position type
// VS_NUMBONES x: 1 = no skinning, used as model space transform
//                >1 = skinning, max indexed transforms, recommended (pow 2, up to 0x40)
// VS_NUMUVS2: Num used vec2 texcoords
// VS_NUMUVS4: Num used vec4 texcoords

#include "common.h.glsl"
#include "dual_quat.glsl"

#ifdef VS_INSTANCED
in InstanceTransforms inTransform;
#else
uniform ubInstanceTransforms {
    InstanceTransforms inTransform;
};
#endif


vec3 GetSingleModelTransform(vec3 pos) {
    return DQTransformPoint(inTransform.indexedModel, pos * inTransform.indexedInflate[0]);
}

vec3 GetModelSpace(vec3 modelSpace) {
    if(maxNumIndexedTransforms > 1) {
        // todo skin
    } else {
        modelSpace = GetSingleModelTransform(modelSpace);
    }

    return modelSpace;
}

#ifdef VS_POSTYPE
in VS_POSTYPE inPos;
#endif

#ifdef VS_NUMUVS2
#include "uvs.vert"
#endif

#if defined(TS_NORMAL_ATTR) || defined(TS_TANGENT_ATTR)
#include "ts_normal.vert"
#endif

#ifdef NUM_LIGHTS
#include "light_omni.vert"
#endif
