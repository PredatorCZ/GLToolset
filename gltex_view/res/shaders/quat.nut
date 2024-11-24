
function QTransformPoint(q = vec4, point = vec3) {
    local qvec = q.yzw;
    local uv = cross(qvec, point);
    local uuv = cross(qvec, uv);

    return point + ((uv * q.x) + uuv) * 2;
}
