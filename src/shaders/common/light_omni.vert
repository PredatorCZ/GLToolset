#include "light_omni.h.glsl"

void ComputeLights(vec3 modelSpace) {
    outLights.fragPos = TransformTSNormal(modelSpace);
    outLights.viewPos = TransformTSNormal(viewPos);

    for(int l = 0; l < maxNumLights; l++) {
        outLights.lightPos[l] = TransformTSNormal(pointLight[l].position);
        outLights.spotLightPos[l] = TransformTSNormal(spotLight[l].position);
        outLights.spotLightDir[l] = TransformTSNormal(spotLight[l].direction);
    }
}
