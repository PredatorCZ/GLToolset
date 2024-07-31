out vec4 fragColor;
in vec2 psTexCoord[1];
uniform sampler2D smTexture;
uniform vec3 v3lightColor;

void main() {
    vec4 color = texture(smTexture, psTexCoord[0]);
    fragColor = vec4(color.a * v3lightColor, color.a);
}
