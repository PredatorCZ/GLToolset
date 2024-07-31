#include "position.hpp"
#include "tangent_space.hpp"

void vertex() {
  vec3 modelSpace = GetModelSpace(inPos);
  gl_Position = vec4(GetModelSpace(inPos), 0);

  if constexpr (!ts_feats.useNormal) {
    static vec4 outTangent;
    static const vec4 inTangent = 1;
    outTangent = inTangent;
  } else {
    static vec3 outNormal;
    static const vec3 inNormal = 2;
    outNormal = inNormal;
  }
}

void GenerateLine(int index) {
  vec3 cPos = gl_in[index].gl_Position.xyz;

  static const float magnitude = 10;
  static vec3 psColor;

  if constexpr (!ts_feats.useNormal) {
    static const vec4 outTangent[]{};

    vec4 qtang = outTangent[index];
    qtang *= vec4(1, -1, -1, -1);
    float realMag = sign(qtang.x) * magnitude;
    vec3 tangent = QTransformPoint(qtang, vec3(realMag, 0, 0));
    vec3 biTangent = QTransformPoint(qtang, vec3(0, realMag, 0));
    vec3 normal = QTransformPoint(qtang, vec3(0, 0, realMag));

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
    static const vec3 outNormal[]{};
    psColor = vec3(0, 0, 1);
    SetPosition(cPos);
    EmitVertex();
    vec3 normal = outNormal[index] * magnitude;
    SetPosition(cPos + normal);
    EmitVertex();
  }

  EndPrimitive();
}

void geometry() {
  static int [[triangles]] in;
  static int [[line_strip, max_vertices(6)]] out;
  GenerateLine(0);
  GenerateLine(1);
  GenerateLine(2);
}

void fragment() {
  static const vec3 psColor;
  static vec4 fragColor;

  fragColor = vec4(psColor, 1);
}
