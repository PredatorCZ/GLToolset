dofile("tangent_space")

NUM_SPOTLIGHTS <- 1;

class SpotLight {
  color = vec3;
  cutOffBegin = float;
  attenuation = vec3;
  cutOffEnd = float;
  position = vec3;
  direction = vec3;
}

function vertexPointLight(modelSpace = vec3) {
  viewPos <- vec3;
  spotLightPos <- array(NUM_SPOTLIGHTS, vec3);
  spotLightDir <- array(NUM_SPOTLIGHTS, vec3);
  local spotLights = GetBuffer(SpotLight, 0)
  usedSpotLights <- array(NUM_SPOTLIGHTS, uint)

  fragPos <- TransformTSNormal(modelSpace);
  viewPosTs <- TransformTSNormal(viewPos);

  for (int l = 0; l < NUM_SPOTLIGHTS; l++) {
    spotLightPos[l] = TransformTSNormal(spotLights[usedSpotLights[l]].position);
    spotLightDir[l] = TransformTSNormal(spotLights[usedSpotLights[l]].direction);
  }
}

class SpotLightData {
  color = vec3;
  specColor = vec3;
};

function GetSpotLight(position = vec3, normal = vec3, color = vec3, attenuation = vec3, specPower = float, specLevel = float,
                      direction = vec3, cutOffBegin = float, cutOffEnd = float) {
  fragPos <- vec3;
  viewPosTs <- vec3;

  local lightToFragDistance = position - fragPos;
  local attenuationLen = length(lightToFragDistance);
  local lightFragDir = lightToFragDistance / attenuationLen;
  local lightDot = max(dot(normal, lightFragDir), 0.0);
  local atten = 1.0 / (attenuation.x + attenuation.y * attenuationLen +
                       attenuation.z * (attenuationLen * attenuationLen));
  local diffColor = lightDot * color * atten;

  local viewDir = normalize(viewPosTs - fragPos);
  local reflectDir = reflect(-lightFragDir, normal);
  local specDot = max(dot(viewDir, reflectDir), 0.0);
  local spec = pow(specDot, specPower);
  local specColor = spec * specLevel * atten * color;

  local theta = dot(lightFragDir, direction);
  local epsilon = cutOffEnd - cutOffBegin;
  local intensity = clamp((theta - cutOffBegin) / epsilon, 0.0, 1.0);

  return SpotLightData(diffColor * intensity, specColor * intensity);
}

function fragmentSpotLight(normal = vec3, specPower = float, specLevel = float) {
  spotLightPos <- array(NUM_SPOTLIGHTS, vec3);
  spotLightDir <- array(NUM_SPOTLIGHTS, vec3);
  local diffuse = 0;
  local specular = 0;
  local spotLights = GetBuffer(SpotLight, 0)
  usedSpotLights <- array(NUM_SPOTLIGHTS, uint)

  for (int l = 0; l < NUM_SPOTLIGHTS; l++) {
    local curSpotLight = spotLights[usedSpotLights[l]];

    local lightData = GetSpotLight(
        spotLightPos[l], normal, curSpotLight.color, curSpotLight.attenuation,
        specPower, specLevel, spotLightDir[l], curSpotLight.cutOffBegin,
        curSpotLight.cutOffEnd);
    diffuse += lightData.color;
    specular += lightData.specColor;
  }
}
