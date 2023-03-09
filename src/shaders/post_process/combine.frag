#version 330 core
uniform sampler2D smColor;
uniform sampler2D smGlow;
uniform vec4 v4BGColor;
in vec2 psTexCoord;
out vec4 outColor;

void main() {
    vec4 color = texture(smColor, psTexCoord);
    vec4 glow = texture(smGlow, psTexCoord);

    outColor = mix(v4BGColor, color, color.a) + glow;
}
