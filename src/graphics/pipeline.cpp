#include "graphics/pipeline.hpp"
#include "common/camera.hpp"
#include "common/resource.hpp"
#include "datas/master_printer.hpp"
#include "datas/reflector.hpp"
#include "graphics/sampler.hpp"
#include "graphics/texture.hpp"
#include "shader_classes.hpp"
#include <GL/glew.h>

static std::map<uint32, uint32> shaderObjects;

static uint32 CompileShader(uint32 type, const char *data) {
  uint32 shaderId = glCreateShader(type);
  glShaderSource(shaderId, 1, &data, nullptr);
  glCompileShader(shaderId);

  {
    int success;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);

    if (!success) {
      char infoLog[512]{};
      glGetShaderInfoLog(shaderId, 512, NULL, infoLog);
      printerror(infoLog);
      GLint sourceLen;
      glGetShaderiv(shaderId, GL_SHADER_SOURCE_LENGTH, &sourceLen);
      std::string source;
      source.resize(sourceLen);
      glGetShaderSource(shaderId, sourceLen, &sourceLen, source.data());
      printinfo(source);
    }
  }

  return shaderId;
}

struct ShaderLocations : prime::graphics::BaseProgramLocations {
  std::map<uint32, uint16> textureSlots;
};

REFLECT(CLASS(ShaderLocations), MEMBER(inPos), MEMBER(inTangent),
        MEMBER(inQTangent), MEMBER(inNormal),
        MEMBERNAME(inTexCoord20, "inTexCoord2[0]"),
        MEMBERNAME(inTexCoord21, "inTexCoord2[1]"),
        MEMBERNAME(inTexCoord22, "inTexCoord2[2]"),
        MEMBERNAME(inTexCoord23, "inTexCoord2[3]"),
        MEMBERNAME(inTexCoord40, "inTexCoord4[0]"),
        MEMBERNAME(inTexCoord41, "inTexCoord4[1]"),
        MEMBERNAME(inTexCoord42, "inTexCoord4[2]"),
        MEMBERNAME(inTexCoord43, "inTexCoord4[3]"), MEMBER(ubCamera),
        MEMBER(ubLights), MEMBER(ubFragmentProperties), MEMBER(ubLightData),
        MEMBER(ubInstanceTransforms), MEMBER(inTransform))

ShaderLocations IntrospectShader(uint32 program) {
  GLint numActiveAttribs = 0;
  GLint numActiveUniformBlocks = 0;
  GLint numActiveUniforms = 0;
  glGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES,
                          &numActiveAttribs);
  glGetProgramInterfaceiv(program, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES,
                          &numActiveUniformBlocks);
  glGetProgramInterfaceiv(program, GL_UNIFORM, GL_ACTIVE_RESOURCES,
                          &numActiveUniforms);

  char nameData[0x40]{};
  ShaderLocations retVal{};
  ReflectorWrap ref(retVal);

  for (int attrib = 0; attrib < numActiveAttribs; attrib++) {
    glGetProgramResourceName(program, GL_PROGRAM_INPUT, attrib,
                             sizeof(nameData), NULL, nameData);
    auto location = glGetAttribLocation(program, nameData);
    ref.SetReflectedValueUInt(JenHash(es::string_view(nameData)), location);
  }

  for (int ub = 0; ub < numActiveUniformBlocks; ub++) {
    glGetProgramResourceName(program, GL_UNIFORM_BLOCK, ub, sizeof(nameData),
                             NULL, nameData);
    auto location = glGetUniformBlockIndex(program, nameData);
    ref.SetReflectedValueUInt(JenHash(es::string_view(nameData)), location);
  }

  std::map<uint32, uint32> textureSlots;

  for (int u = 0; u < numActiveUniforms; u++) {
    glGetProgramResourceName(program, GL_UNIFORM, u, sizeof(nameData), NULL,
                             nameData);

    if (auto location = glGetUniformLocation(program, nameData);
        location != -1) {
      if (es::string_view(nameData).begins_with("sm")) {
        textureSlots.emplace(location, JenkinsHash_(nameData));
      } else {
        //printf("%u %s\n", location, nameData);
      }
    }
  }

  for (auto [_, destination] : textureSlots) {
    retVal.textureSlots.emplace(destination, retVal.textureSlots.size());
  }

  return retVal;
}

struct ReflectorFriend : Reflector {
  using Reflector::GetReflectedType;
};

namespace prime::graphics {
void AddPipeline(Pipeline &pipeline) {
  pipeline.program = glCreateProgram();

  for (auto &s : pipeline.stageObjects) {
    uint32 objectHash = s.object;

    if (auto found = shaderObjects.find(objectHash);
        !es::IsEnd(shaderObjects, found)) {
      s.object = found->second;
    } else {
      auto &res = common::LoadResource(objectHash);
      s.object = CompileShader(s.stageType, res.buffer.c_str());
      shaderObjects.emplace(objectHash, s.object);
    }

    glAttachShader(pipeline.program, s.object);
  }

  glLinkProgram(pipeline.program);

  {
    int success;
    glGetProgramiv(pipeline.program, GL_LINK_STATUS, &success);

    if (!success) {
      char infoLog[512]{};
      glGetProgramInfoLog(pipeline.program, 512, NULL, infoLog);
      printerror(infoLog);
    }
  }

  auto introData = IntrospectShader(pipeline.program);
  pipeline.locations = introData;

  ReflectorWrap ref(introData);
  ReflectorFriend &reff =
      static_cast<ReflectorFriend &>(static_cast<Reflector &>(ref));

  for (auto &t : pipeline.textureUnits) {
    t.sampler = LookupSampler(t.sampler);
    auto unit = LookupTexture(t.texture);
    t.texture = unit.id;
    t.slot = introData.textureSlots.at(t.slotHash);
    t.target = unit.target;
  }

  for (auto &u : pipeline.uniformBlocks) {
    auto &res = common::LoadResource(u.dataObject);
    u.dataObject = reinterpret_cast<uint64>(res.buffer.data());
    u.dataSize = res.buffer.size();
    JenHash bufferLocationHash(u.bufferObject);
    glGenBuffers(1, &u.bufferObject);
    glBindBuffer(GL_UNIFORM_BUFFER, u.bufferObject);
    glBufferData(GL_UNIFORM_BUFFER, u.dataSize,
                 reinterpret_cast<void *>(u.dataObject), GL_DYNAMIC_DRAW);

    auto type = reff.GetReflectedType(bufferLocationHash);

    if (type) {
      auto rData = reinterpret_cast<char *>(&ref.data);
      uint32 location = *reinterpret_cast<uint32 *>(rData + type->offset);
      glUniformBlockBinding(pipeline.program, location, location);
      glBindBufferBase(GL_UNIFORM_BUFFER, location, u.bufferObject);
    } else {
      printerror("Cannot find unform block location "
                 << std::hex << bufferLocationHash.raw());
    }
  }

  glUniformBlockBinding(pipeline.program, pipeline.locations.ubCamera,
                        pipeline.locations.ubCamera);
}

void Pipeline::BeginRender() const {
  glUseProgram(program);

  for (auto &t : textureUnits) {
    glActiveTexture(GL_TEXTURE0 + t.slot);
    glBindSampler(t.slot, t.sampler);
    glBindTexture(t.target, t.texture);
  }

  glBindBufferBase(GL_UNIFORM_BUFFER, locations.ubCamera,
                   common::GetUBCamera());

  for (auto &u : uniformBlocks) {
    glBindBuffer(GL_UNIFORM_BUFFER, u.bufferObject);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, u.dataSize,
                    reinterpret_cast<void *>(u.dataObject));
  }
}

} // namespace prime::graphics
