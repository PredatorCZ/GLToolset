in vec2 psTexCoord;
out vec4 fragColor;

void main() {
    fragColor = vec4(0, 0.7, 0.33, sin(length(psTexCoord * 2 - 1)));
}
