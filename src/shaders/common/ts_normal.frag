vec3 GetTSNormal(vec3 texel) {
#ifdef TS_NORMAL_UNORM
    vec3 result = -1. + texel * 2.;
#else
    vec3 result = texel;
#endif

#ifdef TS_NORMAL_DERIVE_Z
    float derived = clamp(dot(result.xy, result.xy), 0, 1);
    return vec3(result.xy, sqrt(1. - derived));
#else
    return result;
#endif
}
