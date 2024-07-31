#include "single_texture/main.h.glsl"

out vec4 fragColor;
in vec2 psTexCoord[1];

uniform sampler2D smTSNormal;

void main() {
    vec3 normal = GetTSNormal(texture(smTSNormal, psTexCoord[0]).xyz);
    ComputeLights(normal, specLevel, specPower);
    fragColor = vec4((diffuse + specular + ambientColor), 1.);
}
