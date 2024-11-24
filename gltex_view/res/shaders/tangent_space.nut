dofile("quat")

useNormal <- false;
useTangent <- true;
normalMapUnorm <- false;
normalMapDeriveZ <- true;

function TransformTQNormal(qtangent = vec4, point = vec3) {
  local realPoint = sign(qtangent.x) * point;
  return QTransformPoint(qtangent, realPoint);
}

function TransformTSNormal(point = vec3) {
  if (useTangent) {
    local inTangent = GetAttribute(vec4, 1);

    if (useNormal) {
      local inNormal = GetAttribute(vec3, 2);
      local TBN = mat3();
      TBN[2] = inNormal;
      TBN[0] = inTangent.xyz;
      TBN[1] = cross(inTangent.xyz, inNormal) * inTangent.w;
      return transpose(TBN) * point;
    } else {
      return TransformTQNormal(inTangent, point);
    }
  }

  return point;
}

function GetTSNormal(texel = vec3) {
  if (normalMapUnorm) {
    texel = texel * 2 - 1;
  }

  if (normalMapDeriveZ) {
    local derived = clamp(dot(texel.xy, texel.xy), 0, 1);
    return vec3(texel.xy, sqrt(1.f - derived));
  }

  return texel;
}
