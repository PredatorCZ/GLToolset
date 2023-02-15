#ifdef TS_NORMAL_ATTR
out vec3 outNormal;
#endif

#ifdef TS_TANGENT_ATTR
out vec4 outTangent;
#endif

void main() {
    gl_Position = vec4(GetModelSpace(inPos), 0);
#ifdef TS_NORMAL_ATTR
    outNormal = inNormal;
#endif

#ifdef TS_TANGENT_ATTR
    outTangent = inTangent;
#endif
}
