// Defines
// TS_MATRIX: Use matrix for tangent space
// TS_QUAT: Use quat for tangent space instead of matrix
// TS_NORMAL_ONLY: Use only vertex normal (when TS is not used)

#if defined(TS_NORMAL_ONLY) || defined(TS_MATRIX)
in vec3 inNormal;
#endif

#ifdef TS_QUAT
in vec4 inQTangent;
#elif defined(TS_MATRIX)
in vec4 inTangent;
mat3 tsTransform;
#endif

void GetTSNormal() {
#ifdef TS_MATRIX
    mat3 TBN;
    TBN[2] = inNormal;
    TBN[0] = inTangent.xyz;
    TBN[1] = cross(inTangent.xyz, inNormal) * inTangent.w;
    tsTransform = transpose(TBN);
#endif
}

vec3 TransformTSNormal(vec3 point) {
#ifdef TS_QUAT
    return QTransformPoint(inQTangent, point);
#elif defined(TS_MATRIX)
    return tsTransform * point;
#else
    return point;
#endif
}
