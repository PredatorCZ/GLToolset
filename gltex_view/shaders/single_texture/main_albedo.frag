out vec4 fragColor;
in vec2 psTexCoord;

uniform sampler2D smAlbedo;

void main() {
    ComputeLights(vec3(0, 0, 1), specLevel, specPower);
    vec4 albedo = texture(smAlbedo, psTexCoord);
    fragColor = vec4((diffuse + specular + ambientColor), 1.f) * albedo;
}
