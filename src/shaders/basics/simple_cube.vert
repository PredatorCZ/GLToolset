const int numVertices = 36;
const vec3 positions[numVertices] = vec3[numVertices](vec3(1, -1, 1), vec3(-1, -1, 1), vec3(-1, -1, -1), vec3(1, 1, -1), vec3(-1, 1, -1), vec3(-1, 1, 1), vec3(1, 1, -1), vec3(1, 1, 1), vec3(1, -1, 1), vec3(1, -1, 1), vec3(1, 1, 1), vec3(-1, 1, 1), vec3(-1, 1, 1), vec3(-1, 1, -1), vec3(-1, -1, -1), vec3(1, 1, -1), vec3(1, -1, -1), vec3(-1, -1, -1), vec3(1, -1, -1), vec3(1, -1, 1), vec3(-1, -1, -1), vec3(1, 1, 1), vec3(1, 1, -1), vec3(-1, 1, 1), vec3(1, -1, -1), vec3(1, 1, -1), vec3(1, -1, 1), vec3(-1, -1, 1), vec3(1, -1, 1), vec3(-1, 1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1), vec3(-1, -1, -1), vec3(-1, 1, -1), vec3(1, 1, -1), vec3(-1, -1, -1));
const vec2 uv[numVertices] = vec2[numVertices](vec2(0, 1), vec2(1, 1), vec2(1, 0), vec2(1, 0), vec2(0, 0), vec2(0, 1), vec2(1, 0), vec2(0, 0), vec2(0, 1), vec2(1, 1), vec2(1, 0), vec2(0, 0), vec2(1, 0), vec2(0, 0), vec2(0, 1), vec2(0, 0), vec2(0, 1), vec2(1, 1), vec2(0, 0), vec2(0, 1), vec2(1, 0), vec2(1, 1), vec2(1, 0), vec2(0, 1), vec2(1, 1), vec2(1, 0), vec2(0, 1), vec2(0, 1), vec2(1, 1), vec2(0, 0), vec2(1, 1), vec2(1, 0), vec2(0, 1), vec2(1, 0), vec2(0, 0), vec2(1, 1));
const vec4 tangents[numVertices] = vec4[numVertices](vec4(1E-25, -0, -0.707107, 0.707107), vec4(1E-25, -0, -0.707107, 0.707107), vec4(1E-25, -0, -0.707107, 0.707107), vec4(0.707107, 0.707107, -0, -0), vec4(0.707107, 0.707107, -0, -0), vec4(0.707107, 0.707107, -0, -0), vec4(0.707107, 0, -0.707107, -0), vec4(0.707107, 0, -0.707107, -0), vec4(0.707107, 0, -0.707107, -0), vec4(1, 0, -0, -0), vec4(1, 0, -0, -0), vec4(1, 0, -0, -0), vec4(0.707107, 0, 0.707107, -0), vec4(0.707107, 0, 0.707107, -0), vec4(0.707107, 0, 0.707107, -0), vec4(1E-25, -0, -1, -0), vec4(1E-25, -0, -1, -0), vec4(1E-25, -0, -1, -0), vec4(1E-25, -0, -0.707107, 0.707107), vec4(1E-25, -0, -0.707107, 0.707107), vec4(1E-25, -0, -0.707107, 0.707107), vec4(0.707107, 0.707107, -0, -0), vec4(0.707107, 0.707107, -0, -0), vec4(0.707107, 0.707107, -0, -0), vec4(0.707107, 0, -0.707107, -0), vec4(0.707107, 0, -0.707107, -0), vec4(0.707107, 0, -0.707107, -0), vec4(1, 0, -0, -0), vec4(1, 0, -0, -0), vec4(1, 0, -0, -0), vec4(0.707107, 0, 0.707107, -0), vec4(0.707107, 0, 0.707107, -0), vec4(0.707107, 0, 0.707107, -0), vec4(1E-25, -0, -1, -0), vec4(1E-25, -0, -1, -0), vec4(1E-25, -0, -1, -0));

out vec2 psTexCoord;

void main() {
    vec3 modelSpace = GetModelSpace(vec4(positions[gl_VertexID], 0));
#ifdef TS_TANGENT_ATTR
    tsTangent_ = tangents[gl_VertexID];
#endif
#ifdef NUM_LIGHTS
    ComputeLights(modelSpace);
#endif
    SetPosition(modelSpace);
    psTexCoord = uv[gl_VertexID];
}
