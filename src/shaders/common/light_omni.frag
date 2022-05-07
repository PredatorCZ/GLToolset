#include "light_omni.h.glsl"

vec3 diffuse = vec3(0.f);
vec3 specular = vec3(0.f);

struct LightData {
    vec3 color;
    float lightDot;
    vec3 specColor;
    float specDot;
    vec3 direction;
};

LightData GetLight(vec3 position, vec3 normal, vec3 color, vec3 attenuation, float specPower, float specLevel) {
    vec3 lightToFragDistance = position - fragPos;
    float attenuationLen = length(lightToFragDistance);
    vec3 lightFragDir = lightToFragDistance / attenuationLen;
    float lightDot = max(dot(normal, lightFragDir), 0.0);
    float atten = 1.0 / (attenuation.x + attenuation.y * attenuationLen + attenuation.z * (attenuationLen * attenuationLen));
    vec3 diffColor = lightDot * color * atten;

    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightFragDir, normal);
    float specDot = max(dot(viewDir, reflectDir), 0.0);
    float spec = pow(specDot, specPower);
    vec3 specColor = spec * specLevel * atten * color;

    return LightData(diffColor, lightDot, specColor, specDot, lightFragDir);
}

void ComputeLights(vec3 normal, float specLevel, float specPower) {
    for(int l = 0; l < maxNumLights; l++) {
        PointLight curPointLight = pointLight[l];

        if(curPointLight.active) {
            LightData lightData = GetLight(lightPos[l], normal, curPointLight.color, curPointLight.attenuation, specPower, specLevel);
            diffuse += lightData.color;
            specular += lightData.specColor;
        }
    }

    for(int l = 0; l < maxNumLights; l++) {
        SpotLight curSpotLight = spotLight[l];

        if(curSpotLight.cutOffBegin > 0.1f) {
            LightData lightData = GetLight(spotLightPos[l], normal, curSpotLight.color, curSpotLight.attenuation, specPower, specLevel);

            float theta = dot(lightData.direction, spotLightDir[l]);
            float epsilon = curSpotLight.cutOffEnd - curSpotLight.cutOffBegin;
            float intensity = clamp((theta - curSpotLight.cutOffBegin) / epsilon, 0.0, 1.0);

            diffuse += lightData.color * intensity;
            specular += lightData.specColor * intensity;
        }
    }
}
