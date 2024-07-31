#pragma once
#include "tangent_space.hpp"

constexpr int NUM_POINTLIGHTS = 1;

struct PointLight {
  vec3 color;
  vec3 attenuation;
  vec3 position;
} const pointLights[NUM_POINTLIGHTS];

void VertexPointLight(vec3 modelSpace) {
  static const vec3 viewPos;
  static vec3 viewPosTs;
  static vec3 fragPos;
  static vec3 pointLightPos[NUM_POINTLIGHTS];

  fragPos = TransformTSNormal(modelSpace);
  viewPosTs = TransformTSNormal(viewPos);

  for (int l = 0; l < NUM_POINTLIGHTS; l++) {
    pointLightPos[l] = TransformTSNormal(pointLights[l].position);
  }
}

struct PointLightData {
  vec3 color;
  vec3 specColor;
};

PointLightData GetPointLight(vec3 position, vec3 normal, vec3 color,
                             vec3 attenuation, float specPower,
                             float specLevel) {
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

  return PointLightData(diffColor, specColor);
}

PointLightData FragmentPointLight(vec3 normal, float specPower, float specLevel) {
  static const vec3 pointLightPos[NUM_POINTLIGHTS];
  PointLightData outVar = PointLightData(vec3(0), vec3(0));

  for (int l = 0; l < NUM_POINTLIGHTS; l++) {
    PointLight curPointLight = pointLights[l];

    PointLightData lightData =
        GetPointLight(pointLightPos[l], normal, curPointLight.color,
                      curPointLight.attenuation, specPower, specLevel);
    outVar.color += lightData.color;
    outVar.specColor += lightData.specColor;
  }

  return outVar;
}
