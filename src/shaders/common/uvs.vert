#if VS_NUMUVS2 > 0
in vec2 inTexCoord2;
#endif

#if VS_NUMUVS4 > 0
in vec4 inTexCoord4[maxNumUVs4];
#endif

out vec2 psTexCoord[maxNumUVs];

void SetUVs() {
#if VS_NUMUVS4 > 0
    for(int t = 0; t < maxNumUVs4; t++) {
        const int tb = t * 2;
        psTexCoord[tb] = inTransform.uvTransform[tb].xy + inTexCoord4[t].xy * inTransform.uvTransform[tb].zw;
        psTexCoord[tb + 1] = inTransform.uvTransform[tb + 1].xy + inTexCoord4[t].zw * inTransform.uvTransform[tb + 1].zw;
    }
#endif

#if VS_NUMUVS2 > 0
    const int uv2Pos = maxNumUVs4 * 2;
    psTexCoord[uv2Pos] = inTransform.uvTransform[uv2Pos].xy + inTexCoord2 * inTransform.uvTransform[uv2Pos].zw;
#endif
}
