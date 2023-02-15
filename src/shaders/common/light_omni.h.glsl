#pragma once

#ifdef SHADER
#include "defs.glsl"
const int maxNumLights = NUM_LIGHTS;
#endif

#ifdef FRAGMENT
#define ILIGHTS inLights

struct PointLight {
  vec3 color;
  bool isActive;
  vec3 attenuation;
};

struct SpotLight {
  vec3 color;
  float cutOffBegin;
  vec3 attenuation;
  float cutOffEnd;
};

CPPATTR(template <size_t maxNumLights>)
uniform ubLightData {
  PointLight pointLight[maxNumLights];
  SpotLight spotLight[maxNumLights];
};

#endif

#ifdef VERTEX
#define ILIGHTS outLights

CPPATTR(template <size_t maxNumLights>)
uniform ubLights {
  vec3 viewPos;
#ifndef VS_INSTANCED
  vec3 lightPos[maxNumLights];
  vec3 spotLightPos[maxNumLights];
  vec3 spotLightDir[maxNumLights];
#endif
};

#endif

#ifdef SHADER

INTERFACE LightsPS {
  vec3 viewPos;
  vec3 fragPos;
  vec3 lightPos[maxNumLights];
  vec3 spotLightPos[maxNumLights];
  vec3 spotLightDir[maxNumLights];
}
ILIGHTS;

#endif

#undef ILIGHTS
