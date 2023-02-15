in vec2 psTexCoord;

void main() {
    gl_FragColor = vec4(0, 0.7, 0.33, sin(length(psTexCoord * 2 - 1)));
}
