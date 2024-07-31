#include "shader_types.hpp"

constexpr int MAX_BALLS = 100;

struct BallData {
  vec2 center;
  vec3 color;
  float radius;
};

struct {
  BallData data[MAX_BALLS];
} const balls;

struct {
  bool bumpSurface = false;
  int colorMode = 0; // 0 no color, 1 color, 2 smooth color
  int easeType = 0;  // 0 none, 1 ease in, 2 ease out
  bool polygonSmoothColor = false;
} constexpr feats;

vec4 getColor(vec3 pColor, float pSmoothCoeff) {
  if constexpr (feats.colorMode == 0) {
    return vec4(vec3(1.0f) * pSmoothCoeff, 1.0f);
  } else if constexpr (feats.colorMode == 1) {
    return vec4(pColor * pSmoothCoeff, 1.0f);
  } else {
    return vec4(normalize(pColor) * pSmoothCoeff, 1.0f);
  }
}

float getSmoothCoeff(float pSum) {
  float lSmoothCoeff = 1.0f;
  if constexpr (feats.easeType == 1) {
    lSmoothCoeff *= 1.0f - smoothstep(1.0f, 1.5f, pSum);
  } else if constexpr (feats.easeType == 2) {
    lSmoothCoeff *= smoothstep(1.0f, 1.5f, pSum);
  }

  return lSmoothCoeff;
}

vec3 computeNormal(float pRadius, float pPowRadius, float pPowDistAdd,
                   vec2 pCenter) {
  float lDist =
      1.0f - clamp(distance(pCenter, gl_FragCoord.xy) / pRadius, 0.0f, 1.0f);
  return (pPowRadius * 1.0f) / (pPowDistAdd * pPowDistAdd) *
         vec3(2.0f * (pCenter.x - gl_FragCoord.x),
              2.0f * (pCenter.y - gl_FragCoord.y), 2.0f * sin(lDist) * pRadius);
}

void fragment() {
  static vec4 outColor;
  float lSum = 0;
  vec3 lNormal = vec3(0);
  vec3 lColor = vec3(0);
  float lPowMinDist = 1000000.0f;

  for (int i = 0; i < MAX_BALLS; i++) {
    float lRadius = balls.data[i].radius;
    float lPowRadius = lRadius * lRadius;
    vec2 lPowDist = gl_FragCoord.xy - balls.data[i].center.xy;
    lPowDist *= lPowDist;
    float lPowDistAdd = lPowDist.x + lPowDist.y;
    lSum += lPowRadius * 1.0f / lPowDistAdd;

    if constexpr (feats.bumpSurface) {
      lNormal +=
          computeNormal(lRadius, lPowRadius, lPowDistAdd, balls.data[i].center);
    }

    if constexpr (feats.colorMode == 1) {
      if (lPowDistAdd < lPowMinDist) {
        lPowMinDist = lPowDistAdd;
        lColor = balls.data[i].color;
      }
    }

    if constexpr (feats.colorMode == 2) {
      float lDistCoeff =
          1.0f - smoothstep(0.0f, lRadius + (160 * (lRadius / 40.0f)),
                            distance(balls.data[i].center, gl_FragCoord.xy));
      lColor += lDistCoeff * balls.data[i].color;
    }
  }

  if (lSum > 1.0f)
    outColor = getColor(lColor, getSmoothCoeff(lSum));
  else
    outColor = vec4(vec3(0.0f), 1.0f);

  if constexpr (feats.bumpSurface) {
    outColor = vec4(normalize(lNormal), 1.0f);
  }
}

void vertex() {
  static const vec3 in_Vertex = 0;
  static const mat4 uni_Matrix;

  gl_Position = uni_Matrix * vec4(in_Vertex, 1.0f);
}
