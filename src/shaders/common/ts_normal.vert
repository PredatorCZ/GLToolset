// Defines
// TS_TYPE: 0 none, 1 quat, 2 tbn

layout(location = 1) in vec4 inTangent;
layout(location = 2) in vec3 inNormal;

#include "quat.glsl"

vec3 TransformTSNormal(vec3 point) {
    const int TSType = TS_TYPE;
    if(TSType == 1) {
        vec3 realPoint = sign(inTangent.x) * point;
        return QTransformPoint(inTangent, realPoint);
    } else if(TSType == 2) {
        mat3 TBN;
        TBN[2] = inNormal;
        TBN[0] = inTangent.xyz;
        TBN[1] = cross(inTangent.xyz, inNormal) * inTangent.w;
        return transpose(TBN) * point;
    }

    return point;
}

