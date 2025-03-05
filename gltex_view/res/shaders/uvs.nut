
numUVs <- 1;
numUVTMs <- 1;
transformRemaps <- [0]
numUVs4 <- numUVs / 2;
useUVs2 <- (numUVs % 2) == 1;

function TransformUVs() {
  psTexCoord <- array(numUVs, vec2)
  uvTransform <- array(numUVTMs, vec4)

  if (numUVs4 > 0) {
    local inTexCoord4 = GetAttribute(array(numUVs4 ,vec4), 4)

    for (local i = 0; i < numUVs4; i++) {
      local tb = i * 2;
      local remap0 = transformRemaps[tb];
      local remap1 = transformRemaps[tb + 1];

      if (remap0 < 0) {
        psTexCoord[tb] = inTexCoord4[i].xy;
      } else {
        psTexCoord[tb] =
            uvTransform[remap0].xy + inTexCoord4[i].xy * uvTransform[remap0].zw;
      }

      if (remap1 < 0) {
        psTexCoord[tb + 1] = inTexCoord4[i].zw;
      } else {
        psTexCoord[tb + 1] =
            uvTransform[remap1].xy + inTexCoord4[i].zw * uvTransform[remap1].zw;
      }
    }
  }

  if (useUVs2) {
    local idx2 = numUVs4 * 2;
    local remap = transformRemaps[idx2];
    local inTexCoord2 = GetAttribute(vec2, 3)

    if (remap < 0) {
      psTexCoord[idx2] = inTexCoord2;
    } else {
      psTexCoord[idx2] =
          uvTransform[remap].xy + inTexCoord2 * uvTransform[remap].zw;
    }
  }
}
