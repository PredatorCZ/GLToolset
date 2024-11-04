#pragma once
#include "graphics/detail/model_single.hpp"
#include "utils/playground.hpp"

struct NormalVisualizer {
  float *magnitude;
  prime::graphics::ModelSingle *model;

  NormalVisualizer(const prime::graphics::ModelSingle *mdl) {
    namespace pu = prime::utils;
    namespace pg = prime::graphics;
    pu::PlayGround modelPg;

    pu::PlayGround::Pointer<pg::ModelSingle> newModel =
        modelPg.AddClass<pg::ModelSingle>();

    newModel->vertexArray = mdl->vertexArray;

    modelPg.ArrayEmplace(newModel->program.stages,
                         JenkinsHash3_("basics/ts_normal.vert"), 0,
                         GL_VERTEX_SHADER);
    modelPg.ArrayEmplace(newModel->program.stages,
                         JenkinsHash3_("basics/ts_normal.frag"), 0,
                         GL_FRAGMENT_SHADER);
    modelPg.ArrayEmplace(newModel->program.stages,
                         JenkinsHash3_("basics/ts_normal.geom"), 0,
                         GL_GEOMETRY_SHADER);

    for (auto &d : mdl->program.definitions) {
      std::string_view def(d.begin(), d.end());
      if (def.starts_with("TS_") || def.starts_with("VS_NUMUVTMS")) {
        modelPg.NewString(*modelPg.ArrayEmplace(newModel->program.definitions),
                          def);
      }
    }

    uint32 magName = JenkinsHash_("magnitude");

    modelPg.ArrayEmplace(newModel->uniformValues, 0, 0, magName);

    auto mdlHash = pc::MakeHash<pg::ModelSingle>("main_normal_vis");
    pc::AddSimpleResource(pc::ResourceData{
        mdlHash,
        modelPg.Build(),
    });
    auto &data = pc::LoadResource(mdlHash);
    model = static_cast<pg::ModelSingle *>(pc::GetResourceHandle(data));

    for (auto &u : model->uniformValues) {
      if (u.name == magName) {
        magnitude =
            const_cast<float *>(reinterpret_cast<const float *>(u.data));
        break;
      }
    }
  }

  void Render() { pg::Draw(*model); }
};
