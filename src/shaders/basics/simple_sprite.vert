uniform vec4 v4localPos;

const vec4 vertices[6] = vec4[6](vec4(-1, -1, 0, 1), //
vec4(1, -1, 1, 1), //
vec4(1, 1, 1, 0), //
vec4(1, 1, 1, 0), //
vec4(-1, 1, 0, 0), //
vec4(-1, -1, 0, 1)  //
);

void main() {
    vec4 curVert = vertices[gl_VertexID];
    gl_Position = projection * (vec4(DQTransformPoint(view, v4localPos.xyz), 1.0) + vec4(curVert.xy, 0, 0) * v4localPos.w);
    psTexCoord[0] = curVert.zw;
}
