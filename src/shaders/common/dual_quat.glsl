#pragma once

vec3 DQTransformPoint(vec4 dq[2], vec3 point) {
    vec3 real = dq[0].yzw;
    vec3 dual = dq[1].yzw;
    vec3 crs0 = cross(real, cross(real, point) + point * dq[0].x + dual);
    vec3 crs1 = (crs0 + dual * dq[0].x - real * dq[1].x) * 2 + point;
    return crs1;
}

void SetPosition(vec3 pos) {
    vec3 viewSpace = DQTransformPoint(view, pos);
    gl_Position = projection * vec4(viewSpace, 1.0);
}
