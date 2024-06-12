uniform ubFragmentProperties {
    vec3 ambientColor;
    float specLevel;
    float specPower;
    float glowLevel;
    float alphaCutOff;
    float hueRotation;
};

layout(location = 0) out vec4 fragColor;
in vec2 psTexCoord[1];

uniform sampler2D smAlbedo;

#ifdef PS_IN_NORMAL
uniform sampler2D smNormal;
#endif

#ifdef PS_IN_GLOW
uniform sampler2D smGlow;
layout(location = 1) out vec4 glowColor;
#endif

#include "common/quat.glsl"

vec4 HOSToRGB(vec4 albedo) {
    const vec3 rotator = vec3(1 / 2., 1 / 3., 2 / 3.);
    int roundedHue = int(albedo.z * 10);
    vec3 albedoHue = vec3(roundedHue) / 10;
    vec3 absex = abs(mod(albedoHue + hueRotation, vec3(1)) - rotator) * 3;
    vec3 clamped = clamp(1 - absex, 0, 0.5) * 2;
    clamped.r = 1 - clamped.r;
    return vec4(albedo.xxx + clamped.xyz * albedo.yyy, albedo.a);
}

void main() {
    vec4 albedo = texture(smAlbedo, psTexCoord[0]);
    if(albedo.a < alphaCutOff)
        discard;

    //int roundedHue = int(albedo.z * 255);
    //vec3 albedoHue = vec3(roundedHue) / 255;
    //albedo = vec4(albedoHue, albedo.w);

    //albedo = HOSToRGB(albedo.yxzw);

#ifdef PS_IN_NORMAL
    vec4 normal = texture(smNormal, psTexCoord[0]);
#else
    vec3 normal = vec3(0, 0, 1);
#endif
    ComputeLights(GetTSNormal(normal.xyz), specLevel, specPower);
    fragColor = vec4(diffuse + specular + ambientColor, 1.) * albedo;

#ifdef PS_IN_GLOW
    vec4 glow = texture(smGlow, psTexCoord[0]);
    //glow = HOSToRGB(glow);
    glowColor = glow * vec4(glowLevel, glowLevel, glowLevel, 1) * glow.a;
    //fragColor = mix(fragColor, vec4(0), glow.a);
#endif
}
