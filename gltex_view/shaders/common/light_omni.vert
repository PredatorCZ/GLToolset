// Defines
// NUM_LIGHTS: Maximum used lights

#if NUM_LIGHTS > 0


#ifdef VS_INSTANCED
in vec3 lightPos[maxNumLights];
in vec3 spotLightPos[maxNumLights];
in vec3 spotLightDir[maxNumLights];
#endif

void ComputeLights(vec3 modelSpace) {
    outLights.fragPos = TransformTSNormal(modelSpace);
    outLights.viewPos = TransformTSNormal(viewPos);

    for(int l = 0; l < maxNumLights; l++) {
        outLights.lightPos[l] = TransformTSNormal(lightPos[l]);
        outLights.spotLightPos[l] = TransformTSNormal(spotLightPos[l]);
        outLights.spotLightDir[l] = TransformTSNormal(spotLightDir[l]);
    }
}
#endif
