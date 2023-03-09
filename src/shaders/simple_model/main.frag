uniform ubFragmentProperties {
    vec3 ambientColor;
    float specLevel;
    float specPower;
    float glowLevel;
    float alphaCutOff;
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

void main() {
    vec4 albedo = texture(smAlbedo, psTexCoord[0]);
    if(albedo.a < alphaCutOff)
        discard;

#ifdef PS_IN_NORMAL
    vec4 normal = texture(smNormal, psTexCoord[0]);
#else
    vec3 normal = vec3(0, 0, 1);
#endif
    ComputeLights(GetTSNormal(normal.xyz), specLevel, specPower);
    fragColor = vec4((diffuse + specular + ambientColor), 1.f) * albedo;

#ifdef PS_IN_GLOW
    vec4 glow = texture(smGlow, psTexCoord[0]);
    glowColor = glow * vec4(glowLevel, glowLevel, glowLevel, 1) * glow.a;
    fragColor = mix(fragColor, vec4(0), glow.a);
#endif
}
