#pragma once
#include "tangent_space.hpp"

constexpr int NUM_SPOTLIGHTS = 1;

struct SpotLight {
  vec3 color;
  float cutOffBegin;
  vec3 attenuation;
  float cutOffEnd;
  vec3 position;
  vec3 direction;
} const spotLights[];

void vertexPointLight(vec3 modelSpace) {
  static const vec3 viewPos;
  static vec3 viewPosTs;
  static vec3 fragPos;
  static vec3 spotLightPos[NUM_SPOTLIGHTS];
  static vec3 spotLightDir[NUM_SPOTLIGHTS];

  fragPos = TransformTSNormal(modelSpace);
  viewPosTs = TransformTSNormal(viewPos);

  for (int l = 0; l < NUM_SPOTLIGHTS; l++) {
    spotLightPos[l] = TransformTSNormal(spotLights[l].position);
    spotLightDir[l] = TransformTSNormal(spotLights[l].direction);
  }
}

struct SpotLightData {
  vec3 color;
  vec3 specColor;
};

SpotLightData GetSpotLight(vec3 position, vec3 normal, vec3 color,
                           vec3 attenuation, float specPower, float specLevel,
                           vec3 direction, float cutOffBegin, float cutOffEnd) {
  static const vec3 fragPos;
  static const vec3 viewPosTs;

  vec3 lightToFragDistance = position - fragPos;
  float attenuationLen = length(lightToFragDistance);
  vec3 lightFragDir = lightToFragDistance / attenuationLen;
  float lightDot = max(dot(normal, lightFragDir), 0.0);
  float atten = 1.0 / (attenuation.x + attenuation.y * attenuationLen +
                       attenuation.z * (attenuationLen * attenuationLen));
  vec3 diffColor = lightDot * color * atten;

  vec3 viewDir = normalize(viewPosTs - fragPos);
  vec3 reflectDir = reflect(-lightFragDir, normal);
  float specDot = max(dot(viewDir, reflectDir), 0.0);
  float spec = pow(specDot, specPower);
  vec3 specColor = spec * specLevel * atten * color;

  float theta = dot(lightFragDir, direction);
  float epsilon = cutOffEnd - cutOffBegin;
  float intensity = clamp((theta - cutOffBegin) / epsilon, 0.0, 1.0);

  return SpotLightData(diffColor * intensity, specColor * intensity);
}

void fragmentSpotLight(vec3 normal, float specPower, float specLevel) {
  static const vec3 lightPos[NUM_SPOTLIGHTS];
  float diffuse = 0;
  float specular = 0;
  static const vec3 spotLightDir[NUM_SPOTLIGHTS];

  for (int l = 0; l < NUM_SPOTLIGHTS; l++) {
    SpotLight curSpotLight = spotLights[l];

    SpotLightData lightData = GetSpotLight(
        lightPos[l], normal, curSpotLight.color, curSpotLight.attenuation,
        specPower, specLevel, spotLightDir[l], curSpotLight.cutOffBegin,
        curSpotLight.cutOffEnd);
    diffuse += lightData.color;
    specular += lightData.specColor;
  }
}
