#version 330 core
uniform sampler2D smGlow;
in vec2 psTexCoord;
out vec4 outColor;

uniform float weight[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 tex_offset = 1.0 / textureSize(smGlow, 0);
    vec4 glow_ = texture(smGlow, psTexCoord);
    vec3 glow = glow_.rgb * weight[0];

    for(int i = 1; i < 7; ++i) {
        glow += texture(smGlow, psTexCoord + vec2(tex_offset.x * i, 0.0)).rgb * weight[i % 5];
        glow += texture(smGlow, psTexCoord - vec2(tex_offset.x * i, 0.0)).rgb * weight[i % 5];
    }

    outColor = vec4(glow, 1);
}
