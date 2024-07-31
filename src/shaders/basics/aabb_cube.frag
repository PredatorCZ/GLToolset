in vec2 psTexCoord[1];
out vec4 fragColor;

void main() {
    fragColor = vec4(0, 0.7, 0.33, sin(length(psTexCoord[0] * 2 - 1)));
}
