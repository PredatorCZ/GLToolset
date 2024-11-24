dofile("camera")
dofile("dualquat")

function vertex() {
  local vertices = [
      vec4(-1, -1, 0, 1),
      vec4(1, -1, 1, 1),
      vec4(1, 1, 1, 0),
      vec4(1, 1, 1, 0),
      vec4(-1, 1, 0, 0),
      vec4(-1, -1, 0, 1),
  ];

  psTexCoord <- array(1, vec2)

  local curVert = vertices[gl_VertexID];
  local transforms = GetBuffer(vec4, 0)
  local transform = transforms[gl_InstanceID]
  local camera = GetUniformBlock(Camera)

  gl_Position = camera.projection *
                  (vec4(DQTransformPoint(camera.view, transform.xyz), 1.0) +
                   vec4(curVert.xy, 0, 0) * transform.w);

  psTexCoord[0] = curVert.zw;
}
