const int numUVs = VS_NUMUVS;
const bool useUVs2 = (numUVs % 2) == 1;
const int numUVs4 = numUVs / 2;

layout(location = 3) in vec2 inTexCoord2;
layout(location = 4) in vec4 inTexCoord4[numUVs4 > 0 ? numUVs4 : 1];

out vec2 psTexCoord[numUVs];

const int uvTransformRemap[] = int[](VS_UVREMAPS);

#ifndef VS_NUMUVTMS
vec4 uvTransform[10];
#endif

void SetUVs() {
    for(int t = 0; t < numUVs4; t++) {
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

    if(useUVs2) {
        const int idx2 = numUVs4 * 2;
        const int remap = uvTransformRemap[idx2];

        if(remap < 0) {
            psTexCoord[idx2] = inTexCoord2;
        } else {
            psTexCoord[idx2] = uvTransform[remap].xy + inTexCoord2 * uvTransform[remap].zw;
        }
    }
}
