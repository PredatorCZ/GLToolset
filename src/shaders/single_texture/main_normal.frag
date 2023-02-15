#include "single_texture/main.h.glsl"

out vec4 fragColor;
in vec2 psTexCoord;

uniform sampler2D smTSNormal;

void main() {
    vec3 normal = GetTSNormal(texture(smTSNormal, psTexCoord).xyz);
    ComputeLights(normal, specLevel, specPower);
    fragColor = vec4((diffuse + specular + ambientColor), 1.f);
}
