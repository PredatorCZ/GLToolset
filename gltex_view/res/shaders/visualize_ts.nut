dofile("position")
dofile("tangent_space")

function vertex() {
  local inPos = GetAttribute(vec4, 0)
  local modelSpace = GetModelSpace(inPos);
  gl_Position = vec4(GetModelSpace(inPos), 0);

  if (!useNormal) {
    outTangent <- vec4;
    local inTangent = GetAttribute(vec4, 1);
    outTangent = inTangent;
  } else {
    outNormal <- vec3;
    local inNormal = GetAttribute(vec3, 2);
    outNormal = inNormal;
  }
}

function GenerateLine(index = int) {
  local cPos = gl_in[index].gl_Position.xyz;

  magnitude <- float;
  psColor <- vec3;

  if (!useNormal) {
    outTangent <- array(0, vec4);

    local qtang = outTangent[index];
    qtang *= vec4(1, -1, -1, -1);
    local realMag = sign(qtang.x) * magnitude;
    local tangent = QTransformPoint(qtang, vec3(realMag, 0, 0));
    local biTangent = QTransformPoint(qtang, vec3(0, realMag, 0));
    local normal = QTransformPoint(qtang, vec3(0, 0, realMag));

    SetPosition(cPos + tangent);
    psColor = vec3(0, 1, 0);
    EmitVertex();

    psColor = vec3(0.5);
    SetPosition(cPos);
    EmitVertex();

    psColor = vec3(0, 0, 1);
    SetPosition(cPos + normal);
    EmitVertex();

    psColor = vec3(0.5);
    SetPosition(cPos);
    EmitVertex();

    psColor = vec3(1, 0, 0);
    SetPosition(cPos + biTangent);
    EmitVertex();
  } else {
    outNormal <- array(0, vec3);

    psColor = vec3(0, 0, 1);
    SetPosition(cPos);
    EmitVertex();
    local normal = outNormal[index] * magnitude;
    SetPosition(cPos + normal);
    EmitVertex();
  }

  EndPrimitive();
}

function geometry() {
  GeometryInput(triangles)
  GeometryOutput(line_strip, max_vertices(6))
  GenerateLine(0);
  GenerateLine(1);
  GenerateLine(2);
}

function fragment() {
  psColor <- vec3;
  FrameBuffer(0, vec4(psColor, 1))
}
