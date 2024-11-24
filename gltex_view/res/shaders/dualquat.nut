function DQTransformPoint(dq = array(2, vec4), point = vec3) {
    local real = dq[0].yzw;
    local dual = dq[1].yzw;
    local crs0 = cross(real, cross(real, point) + point * dq[0].x + dual);
    local crs1 = (crs0 + dual * dq[0].x - real * dq[1].x) * 2 + point;
    return crs1;
}
