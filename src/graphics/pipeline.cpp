#include "graphics/pipeline.hpp"
#include "common/camera.hpp"
#include "common/resource.hpp"
#include "datas/master_printer.hpp"
#include "datas/reflector.hpp"
#include "graphics/sampler.hpp"
#include "graphics/texture.hpp"
#include "shader_classes.hpp"
#include "utils/shader_preprocessor.hpp"
#include <GL/glew.h>

using GLShaderObject = uint32;
static std::map<uint32, GLShaderObject> shaderObjects;

static void CompileShader(prime::graphics::StageObject &s,
                          std::string_view defBuffer) {
  std::string shaderSource =
      prime::utils::PreprocessShader(s.object, s.stageType, defBuffer);
  uint32 sourceHash = JenkinsHash_(shaderSource);

  if (auto found = shaderObjects.find(sourceHash);
      !es::IsEnd(shaderObjects, found)) {
    s.object = found->second;
    return;
  }

  auto data = shaderSource.c_str();

  uint32 shaderId = glCreateShader(s.stageType);
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
      throw;
    }
  }

  shaderObjects.emplace(sourceHash, s.object = shaderId);
}

struct ShaderLocations : prime::graphics::BaseProgramLocations {
  std::map<uint32, uint16> textureLocations;
};

REFLECT(CLASS(ShaderLocations), MEMBER(inPos), MEMBER(inTangent),
        MEMBER(inNormal), MEMBERNAME(inTexCoord2, "inTexCoord2"),
        MEMBERNAME(inTexCoord40, "inTexCoord4[0]"),
        MEMBERNAME(inTexCoord41, "inTexCoord4[1]"),
        MEMBERNAME(inTexCoord42, "inTexCoord4[2]"), MEMBER(ubCamera),
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
    ref.SetReflectedValueUInt(JenHash(std::string_view(nameData)), location);
  }

  for (int ub = 0; ub < numActiveUniformBlocks; ub++) {
    glGetProgramResourceName(program, GL_UNIFORM_BLOCK, ub, sizeof(nameData),
                             NULL, nameData);
    auto location = glGetUniformBlockIndex(program, nameData);
    ref.SetReflectedValueUInt(JenHash(std::string_view(nameData)), location);
  }

  for (int u = 0; u < numActiveUniforms; u++) {
    glGetProgramResourceName(program, GL_UNIFORM, u, sizeof(nameData), NULL,
                             nameData);

    if (auto location = glGetUniformLocation(program, nameData);
        location != -1) {
      if (std::string_view(nameData).starts_with("sm")) {
        retVal.textureLocations.emplace(JenkinsHash_(nameData), location);
      } else {
        // printf("%u %s\n", location, nameData);
      }
    }
  }

  return retVal;
}

struct ReflectorFriend : Reflector {
  using Reflector::GetReflectedType;
};

namespace prime::graphics {
void AddPipeline(Pipeline &pipeline) {
  pipeline.program = glCreateProgram();
  std::string_view defBuffer(pipeline.definitions.begin(),
                             pipeline.definitions.end());

  for (auto &s : pipeline.stageObjects) {
    CompileShader(s, defBuffer);
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
    if (!introData.textureLocations.contains(t.slotHash)) {
      continue;
    }
    t.sampler = LookupSampler(t.sampler);
    auto unit = LookupTexture(t.texture);
    t.texture = unit.id;
    t.location = introData.textureLocations.at(t.slotHash);
    t.target = unit.target;
  }

  for (auto &u : pipeline.uniformBlocks) {
    common::LinkResource(u.data);
    JenHash bufferLocationHash(u.bufferObject);

    if (u.dataSize == 0) {
      u.dataSize = common::FindResource(u.data.resourcePtr).buffer.size();
    }

    glGenBuffers(1, &u.bufferObject);
    glBindBuffer(GL_UNIFORM_BUFFER, u.bufferObject);
    glBufferData(GL_UNIFORM_BUFFER, u.dataSize, u.data.resourcePtr,
                 GL_DYNAMIC_DRAW);

    auto type = reff.GetReflectedType(bufferLocationHash);

    if (type) {
      auto rData = reinterpret_cast<char *>(&ref.data);
      uint32 location = *reinterpret_cast<uint32 *>(rData + type->offset);
      glUniformBlockBinding(pipeline.program, location, location);
      u.location = location;
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

  uint32 curTexture = 0;
  for (auto &t : textureUnits) {
    glActiveTexture(GL_TEXTURE0 + curTexture);
    glBindTexture(t.target, t.texture);
    glBindSampler(curTexture, t.sampler);
    glUniform1i(t.location, curTexture++);
  }

  glBindBufferBase(GL_UNIFORM_BUFFER, locations.ubCamera,
                   common::GetUBCamera());

  for (auto &u : uniformBlocks) {
    glBindBufferBase(GL_UNIFORM_BUFFER, u.location, u.bufferObject);
    glBindBuffer(GL_UNIFORM_BUFFER, u.bufferObject);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, u.dataSize, u.data.resourcePtr);
  }
}
} // namespace prime::graphics

template <> class prime::common::InvokeGuard<prime::graphics::Pipeline> {
  static inline const bool data =
      prime::common::AddResourceHandle<prime::graphics::Pipeline>({
          .Process =
              [](ResourceData &data) {
                auto hdr = data.As<prime::graphics::Pipeline>();
                prime::graphics::AddPipeline(*hdr);
              },
          .Delete = nullptr,
      });
};
