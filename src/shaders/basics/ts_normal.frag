out vec4 fragColor;
in vec3 psColor;

void main() {
    fragColor = vec4(psColor, 0);
}
