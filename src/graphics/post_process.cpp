#include "graphics/post_process.hpp"
#include "common/resource.hpp"
#include "spike/master_printer.hpp"
#include <GL/glew.h>

static uint32 CompileStage(uint32 type, const char *data) {
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
      throw;
    }
  }

  return shaderId;
}

namespace prime::graphics {
static uint32 VAOID = 0;

PostProcessStage AddPostProcessStage(uint32 object, const FrameBuffer &canvas) {
  if (VAOID < 1) {
    glGenVertexArrays(1, &VAOID);
    glBindVertexArray(VAOID);
  }

  PostProcessStage retVal;
  retVal.program = glCreateProgram();

  {
    auto &res =
        common::LoadResource(common::MakeHash<char>("basics/viewport.vert"));
    uint32 vtStage = CompileStage(GL_VERTEX_SHADER, res.buffer.c_str());
    glAttachShader(retVal.program, vtStage);
  }
  {
    auto &res = common::LoadResource(common::MakeHash<char>(object));
    uint32 vtStage = CompileStage(GL_FRAGMENT_SHADER, res.buffer.c_str());
    glAttachShader(retVal.program, vtStage);
  }

  glLinkProgram(retVal.program);

  {
    int success;
    glGetProgramiv(retVal.program, GL_LINK_STATUS, &success);

    if (!success) {
      char infoLog[512]{};
      glGetProgramInfoLog(retVal.program, 512, NULL, infoLog);
      printerror(infoLog);
    }
  }

  static std::map<std::string_view, FrameBufferTextureType> slotToTexture{
      {"smColor", FrameBufferTextureType::Color},
      {"smGlow", FrameBufferTextureType::Glow},
  };

  GLint numActiveUniforms = 0;
  glGetProgramInterfaceiv(retVal.program, GL_UNIFORM, GL_ACTIVE_RESOURCES,
                          &numActiveUniforms);

  char nameData[0x40]{};

  for (int u = 0; u < numActiveUniforms; u++) {
    glGetProgramResourceName(retVal.program, GL_UNIFORM, u, sizeof(nameData),
                             NULL, nameData);

    if (auto location = glGetUniformLocation(retVal.program, nameData);
        location != -1) {
      std::string_view name(nameData);
      if (name.starts_with("sm")) {
        auto txType = slotToTexture.at(nameData);
        uint32 txId = canvas.FindTexture(txType);
        retVal.textures.emplace_back(
            PostProcessStage::Texture{txId, uint32(location)});
      } else {
        PostProcessStage::Uniform uniform;
        if (name.starts_with("v2")) {
          uniform.numItems = 1;
        } else if (name.starts_with("v3")) {
          uniform.numItems = 2;
        } else if (name.starts_with("v4")) {
          uniform.numItems = 3;
        }

        if (uniform.numItems > 0) {
          name.remove_prefix(2);
        }

        uniform.location = location;

        retVal.uniforms.emplace(std::string(name), uniform);
      }
    }
  }

  return retVal;
}

void PostProcessStage::RenderToResult() const {
  glUseProgram(program);

  uint32 curTexture = 0;
  for (auto &t : textures) {
    glActiveTexture(GL_TEXTURE0 + curTexture);
    glBindTexture(GL_TEXTURE_2D, t.id);
    glBindSampler(curTexture, 0);
    glUniform1i(t.location, curTexture++);
  }

  using UniformSetter = void (*)(const Uniform &);

  static const UniformSetter uniformSetters[]{
      [](const Uniform &u) { glUniform1f(u.location, u.data[0]); },
      [](const Uniform &u) { glUniform2f(u.location, u.data[0], u.data[1]); },
      [](const Uniform &u) {
        glUniform3f(u.location, u.data[0], u.data[1], u.data[2]);
      },
      [](const Uniform &u) {
        glUniform4f(u.location, u.data[0], u.data[1], u.data[2], u.data[3]);
      },
  };

  for (auto &[_, u] : uniforms) {
    uniformSetters[u.numItems](u);
  }

  glBindVertexArray(VAOID);
  glDrawArrays(GL_TRIANGLES, 0, 6);
}

void PostProcess::Resize(uint32 width_, uint32 height_) {
  width = width_;
  height = height_;

  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, NULL);
}

PostProcess CreatePostProcess(uint32 width, uint32 height) {
  PostProcess retVal{};
  glGenFramebuffers(1, &retVal.bufferId);
  glBindFramebuffer(GL_FRAMEBUFFER, retVal.bufferId);

  glGenTextures(1, &retVal.texture);
  glBindTexture(GL_TEXTURE_2D, retVal.texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  retVal.Resize(width, height);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         retVal.texture, 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return retVal;
}

} // namespace prime::graphics
