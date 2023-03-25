#pragma once
#include "model_single.fbs.hpp"

struct NormalVisualizer {
  float *magnitude;
  prime::graphics::ModelSingle *model;

  NormalVisualizer(const prime::graphics::ModelSingle *mdl) {
    flatbuffers::FlatBufferBuilder builder;
    auto stagesPtr = [&] {
      std::vector<pg::StageObject> objects;
      objects.emplace_back(JenkinsHash3_("basics/ts_normal.vert"), 0,
                           GL_VERTEX_SHADER);
      objects.emplace_back(JenkinsHash3_("basics/ts_normal.frag"), 0,
                           GL_FRAGMENT_SHADER);
      objects.emplace_back(JenkinsHash3_("basics/ts_normal.geom"), 0,
                           GL_GEOMETRY_SHADER);
      return builder.CreateVectorOfStructs(objects);
    }();

    auto definesPtr = [&] {
      std::vector<flatbuffers::Offset<flatbuffers::String>> offsets;
      for (auto d : *mdl->program()->definitions()) {
        std::string_view def(d->data(), d->size());
        if (def.starts_with("TS_") || def.starts_with("VS_NUMUVTMS")) {
          offsets.emplace_back(builder.CreateString(def));
        }
      }
      return builder.CreateVector(offsets);
    }();

    pg::ProgramBuilder pgmBuild(builder);
    pgmBuild.add_stages(stagesPtr);
    pgmBuild.add_program(-1);
    pgmBuild.add_definitions(definesPtr);
    auto pgmPtr = pgmBuild.Finish();
    auto vtxPtr = builder.CreateStruct(
        *static_cast<pg::VertexArray *const *>(mdl->vertexArray()));
    uint32 magName = JenkinsHash_("magnitude");

    auto uniformsPtr = [&] {
      std::vector<pg::UniformValue> uniforms;
      uniforms.emplace_back(magName, 0, 0);
      return builder.CreateVectorOfStructs(uniforms);
    }();

    pg::ModelRuntime runtimeDummy{};
    pg::ModelSingleBuilder mdlBuild(builder);
    mdlBuild.add_program(pgmPtr);
    mdlBuild.add_runtime(&runtimeDummy);
    mdlBuild.add_vertexArray_type(pc::ResourceVar_ptr);
    mdlBuild.add_vertexArray(vtxPtr.Union());
    mdlBuild.add_uniformValues(uniformsPtr);

    prime::utils::FinishFlatBuffer(mdlBuild);

    auto mdlHash = pc::MakeHash<pg::ModelSingle>("main_normal_vis");
    pc::AddSimpleResource(pc::ResourceData{
        mdlHash,
        {
            reinterpret_cast<const char *>(builder.GetBufferPointer()),
            builder.GetSize(),
        },
    });
    auto &data = pc::LoadResource(mdlHash);
    model = static_cast<pg::ModelSingle *>(pc::GetResourceHandle(data));

    for (auto u : *model->uniformValues()) {
      if (u->name() == magName) {
        magnitude = const_cast<float *>(reinterpret_cast<const float *>(u->data()));
        break;
      }
    }
  }

  void Render() { pg::Draw(*model); }
};
