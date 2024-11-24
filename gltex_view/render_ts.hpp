#pragma once
#include "graphics/detail/model_single.hpp"
#include "utils/playground.hpp"

struct NormalVisualizer {
  float *magnitude;
  pu::PlayGround modelPg;
  pu::PlayGround::Pointer<pg::ModelSingle> newModel{0, nullptr};

  NormalVisualizer(const prime::graphics::ModelSingle *mdl) {
    auto mdlHash = pc::MakeHash<pg::ModelSingle>("editor/tangent_space_vis");
    auto &data = pc::LoadResource(mdlHash);

    namespace pu = prime::utils;
    namespace pg = prime::graphics;
    std::construct_at(&newModel, modelPg.NewBytes<pg::ModelSingle>(
                                     data.buffer.data(), data.buffer.size()));
    newModel->vertexArray = mdl->vertexArray;
    pg::LegacyProgram &pgm = newModel->program.proto.Get<pg::LegacyProgram>();

    for (auto &d : mdl->program.proto.Get<pg::LegacyProgram>().definitions) {
      std::string_view def(d);
      if (def.starts_with("TS_") || def.starts_with("VS_NUMUVTMS")) {
        modelPg.NewString(*modelPg.ArrayEmplace(pgm.definitions), def);
      }
    }

    for (auto &u : newModel->uniformValues) {
      if (u.name == "magnitude") {
        magnitude =
            const_cast<float *>(reinterpret_cast<const float *>(u.data));
        break;
      }
    }
  }

  void Render() { pg::Draw(*newModel); }
};
