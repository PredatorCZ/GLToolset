out vec3 outNormal;
out vec4 outTangent;

void main() {
  gl_Position = vec4(GetModelSpace(inPos), 0);
  const int TSType = TS_TYPE;

  if (TSType > 0) {
    outTangent = inTangent;
  }

  if (TSType > 1) {
    outNormal = inNormal;
  }
}
