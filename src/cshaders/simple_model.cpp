#include "point_light.hpp"
#include "position.hpp"
#include "uvs.hpp"

struct {
  bool normal = true;
  bool glow = true;
} constexpr fragFeats;

void fragment() {
  static const sampler2D smAlbedo;
  static const vec2 psTexCoord;
  static const float alphaCutOff;

  vec4 albedo = texture(smAlbedo, psTexCoord);
  if (albedo.w < alphaCutOff)
    discard;

  vec3 normal = vec3(0.f, 0.f, 1.f);

  if constexpr (fragFeats.normal) {
    static const sampler2D smNormal;
    normal = texture(smNormal, psTexCoord).xyz;
  }

  static const float specLevel;
  static const float specPower;
  static const vec3 ambientColor;
  static vec4 fragColor = 0;

  PointLightData lightData =
      FragmentPointLight(GetTSNormal(normal), specLevel, specPower);
  fragColor =
      vec4(lightData.color + lightData.specColor + ambientColor, 1.f) * albedo;

  if constexpr (fragFeats.glow) {
    static const sampler2D smGlow;
    static const float glowLevel;
    static vec4 glowColor = 1;

    vec4 glow = texture(smGlow, psTexCoord);
    glowColor = glow * vec4(glowLevel, glowLevel, glowLevel, 1) * glow.x;
    // fragColor = mix(fragColor, vec4(0), glow.x);
  }
}

void vertex() {
  vec3 modelSpace = GetModelSpace(inPos);
  VertexPointLight(modelSpace);
  SetPosition(modelSpace);
  TransformUVs();
}
