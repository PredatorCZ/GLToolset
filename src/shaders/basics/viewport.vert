#version 330 core
out vec2 psTexCoord;

const vec4 vertices[6] = vec4[6](
    vec4(-1, 1, 0, 1), //
    vec4(1, -1, 1, 0), //
    vec4(1, 1, 1, 1), //
    vec4(1, -1, 1, 0), //
    vec4(-1, 1, 0, 1),  //
    vec4(-1, -1, 0, 0) //
);

void main() {
    vec4 curVert = vertices[gl_VertexID];
    gl_Position = vec4(curVert.xy , 0, 1);
    psTexCoord = curVert.zw;
}
