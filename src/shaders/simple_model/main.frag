uniform ubFragmentProperties {
    vec3 ambientColor;
    float specLevel;
    float specPower;
};

out vec4 fragColor;
in vec2 psTexCoord[1];

uniform sampler2D smAlbedo;

#include "common/quat.glsl"

void main() {
    vec4 albedo = texture(smAlbedo, psTexCoord[0]);
    ComputeLights(vec3(0, 0, 1), specLevel, specPower);
    fragColor = vec4((diffuse + specular + ambientColor), 1.f) * albedo;
}
