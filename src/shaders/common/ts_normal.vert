// Defines
// TS_MATRIX: Use matrix for tangent space
// TS_QUAT: Use quat for tangent space instead of matrix
// TS_NORMAL_ATTR: Use normal attribute
// TS_TANGENT_ATTR: Use tangent attribute

#ifdef TS_NORMAL_ATTR
in vec3 inNormal;
#endif

#ifdef TS_TANGENT_ATTR
in vec4 inTangent;
#endif

#ifdef TS_QUAT
#include "quat.glsl"
vec4 tsTangent_;

void GetTSNormal() {
    tsTangent_ = inTangent;
}

vec3 TransformTSNormal(vec3 point) {
    vec3 realPoint = sign(tsTangent_.x) * point;
    return QTransformPoint(tsTangent_, realPoint);
}

#elif defined(TS_NORMAL_ATTR) && defined(TS_TANGENT_ATTR)
mat3 tsTransform_;

void GetTSNormal() {
    mat3 TBN;
    TBN[2] = inNormal;
    TBN[0] = inTangent.xyz;
    TBN[1] = cross(inTangent.xyz, inNormal) * inTangent.w;
    tsTransform_ = transpose(TBN);
}

vec3 TransformTSNormal(vec3 point) {
    return tsTransform_ * point;
}

#else

void GetTSNormal() {
}

vec3 TransformTSNormal(vec3 point) {
    return point;
}

#endif
