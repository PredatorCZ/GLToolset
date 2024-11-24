#pragma once
#include "graphics/detail/program.hpp"
#include "shader_classes.hpp"
#include <span>

struct ubLightData {
  glm::vec3 *viewPos;
  std::span<prime::shaders::PointLight> pointLights;
  std::span<prime::shaders::SpotLight> spotLights;

  static size_t BufferSize(size_t numLights) {
    return sizeof(prime::shaders::PointLight) * numLights +
           sizeof(prime::shaders::SpotLight) * numLights + sizeof(glm::vec4);
  }

  void FromBuffer(auto &buffer, size_t numLights) {
    using PType = decltype(pointLights);
    using SType = decltype(spotLights);
    viewPos = reinterpret_cast<glm::vec3 *>(buffer.data());
    pointLights = PType(reinterpret_cast<PType::value_type *>(
                            buffer.data() + sizeof(glm::vec4)),
                        numLights);
    spotLights = SType(
        reinterpret_cast<SType::value_type *>(pointLights.data() + numLights),
        numLights);
  }
};

struct MainShaderProgram {
  static inline uint32 LDUB = 0;
  std::string lightData;
  ubLightData lightDataSpans;
  uint32 ubLightDataBind;
  prime::graphics::Program *program;

  MainShaderProgram(prime::graphics::Program *program_) : program(program_) {
    uint32 numLights = 0;

    for (auto &d : program->proto.Get<prime::graphics::LegacyProgram>().definitions) {
      std::string_view ds(d);

      if (ds.starts_with("NUM_LIGHTS=")) {
        char buf[4]{};
        memcpy(buf, ds.data() + 11, ds.size() - 11);
        numLights = std::atol(buf);
      }
    }

    lightData.resize(ubLightData::BufferSize(numLights));
    lightDataSpans.FromBuffer(lightData, numLights);

    lightDataSpans.pointLights[0] = prime::shaders::PointLight{
        {1.f, 1.f, 1.f}, true, {1.f, 0.05f, 0.025f}, {}};

    auto &intro = prime::graphics::ProgramIntrospect(program->program);

    ubLightDataBind = intro.uniformBlockBinds.at("ubLightData");

    if (LDUB < 1) {
      glGenBuffers(1, &LDUB);
      glBindBuffer(GL_UNIFORM_BUFFER, LDUB);
      glBufferData(GL_UNIFORM_BUFFER, lightData.size(), lightData.data(),
                   GL_DYNAMIC_DRAW);
    }
  }

  void UseProgram() {
    glBindBufferBase(GL_UNIFORM_BUFFER, ubLightDataBind, LDUB);
    glBindBuffer(GL_UNIFORM_BUFFER, LDUB);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, lightData.size(), lightData.data());
  }
};
