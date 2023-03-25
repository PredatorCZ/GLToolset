#pragma once
#include "program.fbs.hpp"
#include "shader_classes.hpp"
#include <span>

struct ubLightData {
  std::span<prime::shaders::PointLight> pointLights;
  std::span<prime::shaders::SpotLight> spotLights;

  static size_t BufferSize(size_t numLights) {
    return sizeof(prime::shaders::PointLight) * numLights +
           sizeof(prime::shaders::SpotLight) * numLights;
  }

  void FromBuffer(auto &buffer, size_t numLights) {
    using PType = decltype(pointLights);
    using SType = decltype(spotLights);
    pointLights =
        PType(reinterpret_cast<PType::value_type *>(buffer.data()), numLights);
    spotLights = SType(
        reinterpret_cast<SType::value_type *>(pointLights.data() + numLights),
        numLights);
  }
};

struct ubLights {
  glm::vec3 *viewPos;
  std::span<glm::vec4> pointLightTMs;
  std::span<glm::vec4> spotLightTMs;
  std::span<glm::vec4> spotLightDirs;

  static size_t BufferSize(size_t numLights) {
    return sizeof(glm::vec4) * ((numLights * 3) + 1);
  }

  void FromBuffer(auto &buffer, size_t numLights) {
    viewPos = reinterpret_cast<glm::vec3 *>(buffer.data());
    using VType = decltype(pointLightTMs);
    pointLightTMs = VType(
        reinterpret_cast<VType::value_type *>(buffer.data()) + 1, numLights);
    spotLightTMs = VType(
        reinterpret_cast<VType::value_type *>(pointLightTMs.data() + numLights),
        numLights);
    spotLightDirs = VType(
        reinterpret_cast<VType::value_type *>(spotLightTMs.data() + numLights),
        numLights);
  }
};

struct MainShaderProgram {
  static inline uint32 LUB = 0;
  static inline uint32 LDUB = 0;
  std::string lightData;
  ubLightData lightDataSpans;
  std::string lights;
  ubLights lightSpans;
  uint32 ubLightsBind;
  uint32 ubLightDataBind;
  prime::graphics::Program *program;

  MainShaderProgram(prime::graphics::Program *program_) : program(program_) {
    uint32 numLights = 0;

    for (auto d : *program->definitions()) {
      std::string_view ds(d->data(), d->size());

      if (ds.starts_with("NUM_LIGHTS=")) {
        char buf[4]{};
        memcpy(buf, ds.data() + 11, ds.size() - 11);
        numLights = std::atol(buf);
      }
    }

    lightData.resize(ubLightData::BufferSize(numLights));
    lightDataSpans.FromBuffer(lightData, numLights);

    lightDataSpans.pointLights[0] =
        prime::shaders::PointLight{{1.f, 1.f, 1.f}, true, {1.f, 0.05f, 0.025f}};

    lights.resize(ubLights::BufferSize(numLights));
    lightSpans.FromBuffer(lights, numLights);

    auto &intro = prime::graphics::ProgramIntrospect(program->program());

    ubLightsBind = intro.uniformBlockBinds.at("ubLights");
    ubLightDataBind = intro.uniformBlockBinds.at("ubLightData");

    if (LUB < 1) {
      glGenBuffers(1, &LUB);
      glBindBuffer(GL_UNIFORM_BUFFER, LUB);
      glBufferData(GL_UNIFORM_BUFFER, lights.size(), lights.data(),
                   GL_DYNAMIC_DRAW);
    }

    if (LDUB < 1) {
      glGenBuffers(1, &LDUB);
      glBindBuffer(GL_UNIFORM_BUFFER, LDUB);
      glBufferData(GL_UNIFORM_BUFFER, lightData.size(), lightData.data(),
                   GL_DYNAMIC_DRAW);
    }
  }

  void UseProgram() {
    glBindBufferBase(GL_UNIFORM_BUFFER, ubLightsBind, LUB);
    glBindBuffer(GL_UNIFORM_BUFFER, LUB);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, lights.size(), lights.data());
    glBindBufferBase(GL_UNIFORM_BUFFER, ubLightDataBind, LDUB);
    glBindBuffer(GL_UNIFORM_BUFFER, LDUB);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, lightData.size(), lightData.data());
  }
};
