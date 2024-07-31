#include "single_texture/main.h.glsl"

out vec4 fragColor;
in vec2 psTexCoord[1];

uniform sampler2D smAlbedo;

void main() {
    ComputeLights(vec3(0, 0, 1), specLevel, specPower);
    vec4 albedo = texture(smAlbedo, psTexCoord[0]);
    fragColor = vec4((diffuse + specular + ambientColor), 1.) * albedo;
}
