layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inTexCoord20;

out vec2 psTexCoord;

uniform ubCamera {
    mat4 projection;
    vec4 view[2];
};
uniform vec3 localPos;

#include "../common/dual_quat.glsl"

void main() {
    gl_Position = projection * (vec4(DQTransformPoint(view, localPos), 1.0) + vec4(inPos, 0, 0) * 0.1);
    psTexCoord = inTexCoord20;
}
