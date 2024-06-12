#ifdef VS_NUMUVS2
#ifndef VS_NUMUVS4
#define VS_NUMUVS4 0
#endif
const int maxNumUVs2 = VS_NUMUVS2;
const int maxNumUVs4 = VS_NUMUVS4;
const int maxNumUVs = maxNumUVs2 + maxNumUVs4 * 2;
#else
const int maxNumUVs = 1;
#endif

#if VS_NUMUVS2 > 0
layout(location = 3) in vec2 inTexCoord2;
#endif

#if VS_NUMUVS4 > 0
layout(location = 4) in vec4 inTexCoord4[maxNumUVs4];
#endif

out vec2 psTexCoord[maxNumUVs];

const int uvTransformRemap[] = int[](VS_UVREMAPS);

void SetUVs() {
#if VS_NUMUVS4 > 0
    for(int t = 0; t < maxNumUVs4; t++) {
        const int tb = t * 2;
        const int remap0 = uvTransformRemap[tb];
        const int remap1 = uvTransformRemap[tb + 1];

        if(remap0 < 0) {
            psTexCoord[tb] = inTexCoord4[t].xy;
        } else {
            psTexCoord[tb] = uvTransform[remap0].xy + inTexCoord4[t].xy * uvTransform[remap0].zw;
        }

        if(remap1 < 0) {
            psTexCoord[tb + 1] = inTexCoord4[t].zw;
        } else {
            psTexCoord[tb + 1] = uvTransform[remap1].xy + inTexCoord4[t].zw * uvTransform[remap1].zw;
        }
    }
#endif

#if VS_NUMUVS2 > 0
    const int idx2 = maxNumUVs4 * 2;
    const int remap = uvTransformRemap[idx2];

    if(remap < 0) {
        psTexCoord[idx2] = inTexCoord2;
    } else {
        psTexCoord[idx2] = uvTransform[remap].xy + inTexCoord2 * uvTransform[remap].zw;
    }
#endif
}
