// Defines
// VS_INSTANCED: Instanced draw call
// VS_POSTYPE: Vertex position type
// VS_NUMBONES x: 1 = no skinning, used as model space transform
//                >1 = skinning, max indexed transforms, recommended (pow 2, up to 0x40)
// VS_NUMUVS2: Num used vec2 texcoords
// VS_NUMUVS4: Num used vec4 texcoords

in VS_POSTYPE inPos;
#if VS_NUMUVS2 > 0
in vec2 inTexCoord2[maxNumUVs2];
#endif
#if VS_NUMUVS4 > 0
in vec4 inTexCoord4[maxNumUVs4];
#endif
out vec2 psTexCoord[maxNumUVs];

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

vec3 GetModelSpace() {
    vec3 modelSpace = inPos.xyz;

    if(maxNumIndexedTransforms > 1) {
        // todo skin
    } else {
        modelSpace = GetSingleModelTransform(modelSpace);
    }

    return modelSpace;
}

void SetPosition(vec3 pos) {
    vec3 viewSpace = DQTransformPoint(view, pos);
    gl_Position = projection * vec4(viewSpace, 1.0);
}

void SetUVs() {
#if VS_NUMUVS4 > 0
    for(int t = 0; t < maxNumUVs4; t++) {
        const int tb = t * 2;
        psTexCoord[tb] = inTransform.uvTransform[tb].xy + inTexCoord4[t].xy * inTransform.uvTransform[tb].zw;
        psTexCoord[tb + 1] = inTransform.uvTransform[tb + 1].xy + inTexCoord4[t].zw * inTransform.uvTransform[tb + 1].zw;
    }
#endif

#if VS_NUMUVS2 > 0
    for(int t = maxNumUVs4 * 2, p = 0; p < maxNumUVs2; t++, p++) {
        psTexCoord[t] = inTransform.uvTransform[t].xy + inTexCoord2[t] * inTransform.uvTransform[t].zw;
    }
#endif
}
