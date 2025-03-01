
NUM_POINTLIGHTS <- 1;

class PointLight {
  color = vec3
  attenuation = vec3
  position = vec3
}

function VertexPointLight(modelSpace = vec3) {
  viewPos <- vec3;
  local pointLights = GetBuffer(PointLight, 0)
  local usedPointLights = GetUniformBLock(uint, NUM_POINTLIGHTS)

  fragPos <- TransformTSNormal(modelSpace);
  viewPosTs <- TransformTSNormal(viewPos);
  pointLightPos <- array(NUM_POINTLIGHTS, vec3)

  for (local l = 0; l < NUM_POINTLIGHTS; l++) {
    pointLightPos[usedPointLights[l]] = TransformTSNormal(pointLights[l].position);
  }
}

class PointLightData {
  color = vec3;
  specColor = vec3;
};

function GetPointLight(position = vec3, normal = vec3, color = vec3,
                             attenuation = vec3, specPower = float,
                             specLevel = float) {
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

  return PointLightData() {color = diffColor, specColor = specColor}
}

function FragmentPointLight(normal = vec3, specPower = float, specLevel = float) {
  local outVar = PointLightData();
  local pointLights = GetBuffer(PointLight)
  local usedPointLights = GetUniformBLock(uint, NUM_POINTLIGHTS)

  for (local l = 0; l < NUM_POINTLIGHTS; l++) {
    local curPointLight = pointLights[usedPointLights[l]];

    local lightData =
        GetPointLight(pointLightPos[l], normal, curPointLight.color,
                      curPointLight.attenuation, specPower, specLevel);
    outVar.color += lightData.color;
    outVar.specColor += lightData.specColor;
  }

  return outVar;
}
