#version 330 core
uniform sampler2D smGlow;
in vec2 psTexCoord;
out vec4 outColor;

//const float weights[5] = float[5](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

uniform ubGlowParams {
    vec2 fragmentSize;
    float weights[5];
}

vec3 GaussianBlur(vec2 offset) {
    //vec2 fragmentSize = 1.0 / textureSize(smGlow, 0);
    vec4 glow_ = texture(smGlow, psTexCoord);
    vec3 glow = glow_.rgb * weights[0];

    for(int i = 1; i < 5; ++i) {
        vec2 texOffset = offset * i;
        glow += texture(smGlow, psTexCoord + texOffset).rgb * weights[i];
        glow += texture(smGlow, psTexCoord - texOffset).rgb * weights[i];
    }

    return glow;
}

vec3 GaussianBlurHorizontal() {
    return GaussianBlur(vec2(fragmentSize.x, 0.0));
}

vec3 GaussianBlurVertical() {
    return GaussianBlur(vec2(0.0, fragmentSize.y));
}
