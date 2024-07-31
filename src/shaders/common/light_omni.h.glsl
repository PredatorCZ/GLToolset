#pragma once

#ifdef SHADER
#include "defs.glsl"
const int maxNumLights = NUM_LIGHTS;
#endif

struct PointLight {
  vec3 color;
  bool isActive;
  vec3 attenuation;
  vec3 position;
};

struct SpotLight {
  vec3 color;
  float cutOffBegin;
  vec3 attenuation;
  float cutOffEnd;
  vec3 position;
  vec3 direction;
};

CPPATTR(template <size_t maxNumLights>)
uniform ubLightData {
  vec3 viewPos;
  PointLight pointLight[maxNumLights];
  SpotLight spotLight[maxNumLights];
};

#ifdef FRAGMENT
#define ILIGHTS inLights
#endif

#ifdef VERTEX
#define ILIGHTS outLights
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
