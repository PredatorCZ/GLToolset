#include "common/common.h.glsl"
#include "common/quat.glsl"
#include "common/dual_quat.glsl"

layout(triangles) in;
layout(line_strip, max_vertices = 6) out;

in vec3 outNormal[];
in vec4 outTangent[];

uniform float magnitude = 10;

out vec3 psColor;

void GenerateLine(int index) {
    vec3 cPos = gl_in[index].gl_Position.xyz;
    const int TSType = TS_TYPE;

    if(TSType == 1) {
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
        psColor = vec3(0, 0, 1);
        SetPosition(cPos);
        EmitVertex();
        vec3 normal = outNormal[index] * magnitude;
        SetPosition(cPos + normal);
        EmitVertex();
    }

    EndPrimitive();
}

void main() {
    GenerateLine(0);
    GenerateLine(1);
    GenerateLine(2);
}
