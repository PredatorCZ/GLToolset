dofile("point_light")
dofile("position")

function TransformTSNormal(point = vec3) {
  if (useTangent) {
    local tangents = [
        vec4(1E-25, -0, -0.707107, 0.707107),    vec4(1E-25, -0, -0.707107, 0.707107),      vec4(1E-25, -0, -0.707107, 0.707107),   vec4(0.707107, 0.707107, -0, -0),
        vec4(0.707107, 0.707107, -0, -0),        vec4(0.707107, 0.707107, -0, -0),          vec4(0.707107, 0, -0.707107, -0),       vec4(0.707107, 0, -0.707107, -0),
        vec4(0.707107, 0, -0.707107, -0),        vec4(1, 0, -0, -0),                        vec4(1, 0, -0, -0),                     vec4(1, 0, -0, -0),
        vec4(0.707107, 0, 0.707107, -0),         vec4(0.707107, 0, 0.707107, -0),           vec4(0.707107, 0, 0.707107, -0),        vec4(1E-25, -0, -1, -0),
        vec4(1E-25, -0, -1, -0),                 vec4(1E-25, -0, -1, -0),                   vec4(1E-25, -0, -0.707107, 0.707107),   vec4(1E-25, -0, -0.707107, 0.707107),
        vec4(1E-25, -0, -0.707107, 0.707107),    vec4(0.707107, 0.707107, -0, -0),          vec4(0.707107, 0.707107, -0, -0),       vec4(0.707107, 0.707107, -0, -0),
        vec4(0.707107, 0, -0.707107, -0),        vec4(0.707107, 0, -0.707107, -0),          vec4(0.707107, 0, -0.707107, -0),       vec4(1, 0, -0, -0),
        vec4(1, 0, -0, -0),                      vec4(1, 0, -0, -0),                        vec4(0.707107, 0, 0.707107, -0),        vec4(0.707107, 0, 0.707107, -0),
        vec4(0.707107, 0, 0.707107, -0),         vec4(1E-25, -0, -1, -0),                   vec4(1E-25, -0, -1, -0),                vec4(1E-25, -0, -1, -0)
    ]
    return TransformTQNormal(tangents[gl_VertexID], point);
  }

  return point;
}

function vertex() {
    local positions = [
        vec3(1, -1, 1),   vec3(-1, -1, 1),  vec3(-1, -1, -1), vec3(1, 1, -1),  vec3(-1, 1, -1),   vec3(-1, 1, 1),    vec3(1, 1, -1),     vec3(1, 1, 1),
        vec3(1, -1, 1),   vec3(1, -1, 1),   vec3(1, 1, 1),    vec3(-1, 1, 1),  vec3(-1, 1, 1),    vec3(-1, 1, -1),   vec3(-1, -1, -1),   vec3(1, 1, -1),
        vec3(1, -1, -1),  vec3(-1, -1, -1), vec3(1, -1, -1),  vec3(1, -1, 1),  vec3(-1, -1, -1),  vec3(1, 1, 1),     vec3(1, 1, -1),     vec3(-1, 1, 1),
        vec3(1, -1, -1),  vec3(1, 1, -1),   vec3(1, -1, 1),   vec3(-1, -1, 1), vec3(1, -1, 1),    vec3(-1, 1, 1),    vec3(-1, -1, 1),    vec3(-1, 1, 1),
        vec3(-1, -1, -1), vec3(-1, 1, -1),  vec3(1, 1, -1),   vec3(-1, -1, -1)
    ]
    local uvs = [
        vec2(0, 1), vec2(1, 1), vec2(1, 0), vec2(1, 0), vec2(0, 0), vec2(0, 1), vec2(1, 0), vec2(0, 0), vec2(0, 1), vec2(1, 1), vec2(1, 0), vec2(0, 0),
        vec2(1, 0), vec2(0, 0), vec2(0, 1), vec2(0, 0), vec2(0, 1), vec2(1, 1), vec2(0, 0), vec2(0, 1), vec2(1, 0), vec2(1, 1), vec2(1, 0), vec2(0, 1),
        vec2(1, 1), vec2(1, 0), vec2(0, 1), vec2(0, 1), vec2(1, 1), vec2(0, 0), vec2(1, 1), vec2(1, 0), vec2(0, 1), vec2(1, 0), vec2(0, 0), vec2(1, 1)
    ]

  local modelSpace = GetModelSpace(vec4(positions[gl_VertexID], 0));

  psTexCoord <- array(1, vec2)

  if (NUM_POINTLIGHTS > 0) {
    VertexPointLight(modelSpace);
  }

  SetPosition(modelSpace);
  psTexCoord[0] = uvs[gl_VertexID];
}
