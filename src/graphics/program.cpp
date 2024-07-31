#include "graphics/program.hpp"
#include "spike/master_printer.hpp"
#include "utils/shader_preprocessor.hpp"
#include <GL/glew.h>

#include "program.fbs.hpp"

namespace {
using GLShaderObject = uint32;

struct ShaderObject {
  uint32 glObject;
  std::vector<uint32> programs;
};
static std::map<uint32, GLShaderObject> shaderObjects;
static std::map<uint32, std::set<prime::common::ResourceHash>> STAGE_REFERENCES;

static bool CompileShader(prime::graphics::StageObject &s,
                          std::span<std::string_view> defBuffer) {
  std::string shaderSource =
      prime::utils::PreprocessShader(s.resource(), s.type(), defBuffer);
  uint32 sourceHash = JenkinsHash_(shaderSource);

  if (auto found = shaderObjects.find(sourceHash);
      shaderObjects.end() != found) {
    s.mutate_object(found->second);
    return true;
  }

  auto data = shaderSource.c_str();

  uint32 shaderId = glCreateShader(s.type());
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

  s.mutate_object(shaderId);
  shaderObjects.emplace(sourceHash, shaderId);
  return false;
}

prime::graphics::ProgramIntrospection IntrospectShader(uint32 program) {
  prime::graphics::ProgramIntrospection retVal;
  retVal.program = program;
  GLint numActiveUniformBlocks = 0;
  GLint numActiveUniforms = 0;
  GLint numActiveBuffers = 0;

  glGetProgramInterfaceiv(program, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES,
                          &numActiveUniformBlocks);
  glGetProgramInterfaceiv(program, GL_UNIFORM, GL_ACTIVE_RESOURCES,
                          &numActiveUniforms);
  glGetProgramInterfaceiv(program, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES,
                          &numActiveBuffers);

  char nameData[0x40]{};
  char *nameDataPtr = nameData;
  uint32 curBinding = 1;

  for (int ub = 0; ub < numActiveUniformBlocks; ub++) {
    glGetProgramResourceName(program, GL_UNIFORM_BLOCK, ub, sizeof(nameData),
                             NULL, nameData);
    auto ubIndex = glGetUniformBlockIndex(program, nameData);
    PrintInfo("UBO: ", nameData);
    if (std::string_view(nameData) == "ubCamera") {
      glUniformBlockBinding(program, ubIndex, 0);
      retVal.uniformBlockBinds.emplace(nameDataPtr, 0);
    } else {
      glUniformBlockBinding(program, ubIndex, curBinding);
      retVal.uniformBlockBinds.emplace(nameDataPtr, curBinding++);
    }
  }

  for (int u = 0; u < numActiveUniforms; u++) {
    glGetProgramResourceName(program, GL_UNIFORM, u, sizeof(nameData), NULL,
                             nameData);

    if (auto location = glGetUniformLocation(program, nameData);
        PrintInfo("Uniform: ", nameData), location != -1) {
      std::string_view name(nameData);

      if (name.starts_with("sm")) {
        retVal.textureLocations.emplace(nameDataPtr, location);
      } else {
        prime::graphics::ProgramIntrospection::Uniform uniform;
        if (name.starts_with("v2")) {
          uniform.size = 1;
        } else if (name.starts_with("v3")) {
          uniform.size = 2;
        } else if (name.starts_with("v4")) {
          uniform.size = 3;
        }

        if (uniform.size > 0) {
          name.remove_prefix(2);
        }

        uniform.location = location;
        retVal.uniformLocations.emplace(name, uniform);
      }
    }
  }

  for (int ub = 0; ub < numActiveBuffers; ub++) {
    glGetProgramResourceName(program, GL_SHADER_STORAGE_BLOCK, ub,
                             sizeof(nameData), NULL, nameData);
    PrintInfo("SSBO: ", nameData);
    auto location = glGetUniformBlockIndex(program, nameData);
    retVal.storageBufferLocations.emplace(nameDataPtr, location);
  }

  return retVal;
}
} // namespace

namespace prime::graphics {

static std::map<uint32, ProgramIntrospection> programObjects;

const ProgramIntrospection &
CreateProgram(Program &pgm, common::ResourceHash referee,
              std::vector<std::string> *secondaryDefs) {
  using ProgramKey = std::array<uint32, 6>;
  static std::map<ProgramKey, uint32> stagesToProgram;
  std::vector<std::string_view> defs;

  if (auto defines = pgm.definitions()) {
    for (auto d : *defines) {
      defs.emplace_back(d->data(), d->size());
    }
  }

  if (secondaryDefs) {
    for (auto &d : *secondaryDefs) {
      defs.emplace_back(d);
    }
  }

  bool fullyCached = true;

  for (auto s : *pgm.stages()) {
    STAGE_REFERENCES[s->resource()].emplace(referee);
    bool cached = CompileShader(*const_cast<StageObject *>(s),
                                {defs.data(), defs.size()});

    if (!cached) {
      fullyCached = false;
    }
  }

  ProgramKey key{};
  size_t i = 0;

  for (auto s : *pgm.stages()) {
    key[i++] = s->object();
  }

  std::sort(key.begin(), key.end());

  if (fullyCached) {
    pgm.mutate_program(stagesToProgram.at(key));
    return programObjects.at(pgm.program());
  }
  uint32 program = glCreateProgram();

  for (auto s : *pgm.stages()) {
    glAttachShader(program, s->object());
  }

  glLinkProgram(program);

  {
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
      char infoLog[512]{};
      glGetProgramInfoLog(program, 512, NULL, infoLog);
      printerror(infoLog);
    }
  }

  GLint binSize;
  glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &binSize);

  std::vector<uint8> binProgram;
  binProgram.resize(binSize);
  GLsizei binLength;
  GLenum binFormat;
  void *pgmptr = binProgram.data();

  glGetProgramBinary(program, binSize, &binLength, &binFormat,
                     binProgram.data());

  pgm.mutate_program(program);
  stagesToProgram.emplace(key, program);

  return programObjects.emplace(program, IntrospectShader(program))
      .first->second;
}

const ProgramIntrospection &ProgramIntrospect(uint32 program) {
  return programObjects.at(program);
}
} // namespace prime::graphics

REGISTER_CLASS(prime::graphics::UniformBlockData);
HASH_CLASS(prime::graphics::Program);
HASH_CLASS(prime::graphics::StageObject);

template <> class prime::common::InvokeGuard<prime::graphics::StageObject> {
  static inline const bool data =
      prime::common::AddResourceHandle<graphics::StageObject>({
          .Process = nullptr,
          .Delete = nullptr,
          .Handle = nullptr,
          .Update =
              [](ResourceHash hash) {
                if (hash.type !=
                    common::GetClassHash<graphics::StageObject>()) {
                  return;
                }

                auto &stages = STAGE_REFERENCES.at(hash.name);

                for (auto &stage : stages) {
                  GetClassHandle(stage.type).Update(stage);
                }
              },
      });
};
