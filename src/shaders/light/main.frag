out vec4 fragColor;
in vec2 psTexCoord;
uniform sampler2D smTexture;
uniform vec3 v3lightColor;

void main() {
    vec4 color = texture(smTexture, psTexCoord);
    fragColor = vec4(color.a * v3lightColor, color.a);
}
