vec3 GetTSNormal(vec3 texel) {
    const bool useUnorm = TS_NORMAL_UNORM;
    const bool deriveZ = TS_NORMAL_DERIVE_Z;

    if(useUnorm) {
        texel = texel * 2 - 1;
    }

    if(deriveZ) {
        float derived = clamp(dot(texel.xy, texel.xy), 0, 1);
        return vec3(texel.xy, sqrt(1. - derived));
    }

    return texel;
}
