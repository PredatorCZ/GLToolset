dofile("point_light")
dofile("position")
dofile("uvs")

normal <- true;
glow <- true;

function fragment() {
  smAlbedo <- sampler2D
  psTexCoord <- vec2;
  alphaCutOff <- float;

  local albedo = texture(smAlbedo, psTexCoord);
  if (albedo.w < alphaCutOff)
    return;

  local normal = vec3(0, 0, 1);

  if (normal) {
    smNormal <- sampler2D;
    normal = texture(smNormal, psTexCoord).xyz;
  }

  specLevel <- float;
  specPower <- float;
  ambientColor <- vec3;

  local lightData =
      FragmentPointLight(GetTSNormal(normal), specLevel, specPower);
  local fragColor =
      vec4(lightData.color + lightData.specColor + ambientColor, 1) * albedo;

  if (glow) {
    smGlow <- sampler2D;
    glowLevel <- float;

    local glow = texture(smGlow, psTexCoord);
    FrameBuffer(1, glow * vec4(glowLevel, glowLevel, glowLevel, 1) * glow.x);
    // fragColor = mix(fragColor, vec4(0), glow.x);
  }

  FrameBuffer(0, fragColor)
}

function vertex() {
  local inPos = GetAttribute(vec4, 0)
  local modelSpace = GetModelSpace(inPos);
  VertexPointLight(modelSpace);
  SetPosition(modelSpace);
  TransformUVs();
}
