dofile("camera")
dofile("dualquat")

numBones <- 1;

class Transforms {
  indexedModel = array(numBones, array(2, vec4))
  indexedInflate = array(numBones, vec3);
};

function GetSingleModelTransform(pos = vec4, index = int) {
  local transforms = GetBuffer(Transforms, 0)
  local transform = transforms[gl_InstanceID]

  return DQTransformPoint(transform.indexedModel[index],
                          pos.xyz * transform.indexedInflate[index]);
}

class WeightData {
  wt4BeginIndex = uint;
  wt8BeginIndex = uint;
  wt8BaseIndex = uint;
  data = array(0, uint);
}

function unpackUint4x8(input = uint) {
  return (input.xxxx >> uvec4(0, 8, 16, 24)) & 0xffu;
}

function Get4BoneTm(baseIndex = uint, modelSpace = vec4) {
  local weightData = GetBuffer(WeightData)
  local weights = unpackUnorm4x8(weightData.data[baseIndex + 1]);
  local bones = unpackUint4x8(weightData.data[baseIndex]);

  local tm0 = GetSingleModelTransform(modelSpace, bones.x) * weights.x;
  local tm1 = GetSingleModelTransform(modelSpace, bones.y) * weights.y;
  local tm2 = GetSingleModelTransform(modelSpace, bones.z) * weights.z;
  local tm3 = GetSingleModelTransform(modelSpace, bones.w) * weights.w;

  return tm0 + tm1 + tm2 + tm3;
}

function GetModelSpace(modelSpace = vec4) {
  if (numBones > 1) {
    local index = modelSpace.w;
    local weightData = GetBuffer(WeightData)

    if (index < numBones) {
      return GetSingleModelTransform(modelSpace, index);
    } else if (index < weightData.wt4BeginIndex) {
      local wt = unpackUint4x8(weightData.data[index - numBones]);
      local bones = wt.xy;
      local weights = vec2(wt.zw) * (1 / 255.f);
      local tm0 = GetSingleModelTransform(modelSpace, bones.x) * weights.x;
      local tm1 = GetSingleModelTransform(modelSpace, bones.y) * weights.y;

      return tm0 + tm1;
    } else if (index < weightData.wt8BeginIndex) {
      local baseIndex = (index - weightData.wt4BeginIndex) * 2;
      return Get4BoneTm(baseIndex, modelSpace);
    } else {
      local baseIndex =
          weightData.wt8BaseIndex + (index - weightData.wt8BeginIndex) * 4;
      return Get4BoneTm(baseIndex, modelSpace) +
             Get4BoneTm(baseIndex + 2, modelSpace);
    }
  } else {
    return GetSingleModelTransform(modelSpace, 0);
  }
}

function SetPosition(pos = vec3) {
  local viewSpace = DQTransformPoint(ubCamera.view, pos);
  local camera = GetUniformBlock(Camera)
  gl_Position = camera.projection * vec4(viewSpace, 1.0);
}
