uniform vec4 localPos;
out vec2 psTexCoord;

const vec4 vertices[6] = vec4[6](
    vec4(-1, -1, 0, 1), //
    vec4(1,  -1, 1, 1), //
    vec4(1,  1,  1, 0), //
    vec4(1,  1,  1, 0), //
    vec4(-1, 1,  0, 0), //
    vec4(-1, -1, 0, 1)  //
);

void main() {
    vec4 curVert = vertices[gl_VertexID];
    gl_Position = projection * (vec4(DQTransformPoint(view, localPos.xyz), 1.0) + vec4(curVert.xy, 0, 0) * localPos.w);
    psTexCoord = curVert.zw;
}
