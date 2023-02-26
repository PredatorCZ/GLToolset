uniform ubFragmentProperties {
    vec3 ambientColor;
    float specLevel;
    float specPower;
};

out vec4 fragColor;
in vec2 psTexCoord[1];

uniform sampler2D smAlbedo;
uniform sampler2D smNormal;

#include "common/quat.glsl"

void main() {
    vec4 albedo = texture(smAlbedo, psTexCoord[0]);
    vec4 normal = texture(smNormal, psTexCoord[0]);
    ComputeLights(GetTSNormal(normal.xyz), specLevel, specPower);
    fragColor = vec4((diffuse + specular + ambientColor), 1.f) * albedo;
}
