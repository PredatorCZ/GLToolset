#pragma once
#include "model_single.fbs.hpp"
#include "vertex.fbs.hpp"

const prime::graphics::VertexArray *GetBoxVertexArray() {
  static const prime::graphics::VertexArray *VT_ARRAY = nullptr;

  if (!VT_ARRAY) {
    flatbuffers::FlatBufferBuilder builder;
    prime::graphics::VertexArrayBuilder vtBuilder(builder);
    vtBuilder.add_count(6);
    vtBuilder.add_mode(GL_TRIANGLES);
    prime::utils::FinishFlatBuffer(vtBuilder);
    auto vayHash = pc::MakeHash<pg::VertexArray>("box_vertex");

    pc::AddSimpleResource(pc::ResourceData{
        vayHash,
        {
            reinterpret_cast<const char *>(builder.GetBufferPointer()),
            builder.GetSize(),
        },
    });
    auto &data = pc::LoadResource(vayHash);
    VT_ARRAY = static_cast<prime::graphics::VertexArray *>(
        pc::GetResourceHandle(data));
  }

  return VT_ARRAY;
}

struct BoxObject {
  static inline prime::graphics::ModelSingle *model = nullptr;
  glm::vec4 *localPos;
  glm::vec3 *lightColor;

  BoxObject() {
    if (!model) {
      flatbuffers::FlatBufferBuilder builder;

      auto stagesPtr = [&] {
        std::vector<pg::StageObject> objects;
        objects.emplace_back(JenkinsHash3_("basics/simple_sprite.vert"), 0,
                             GL_VERTEX_SHADER);
        objects.emplace_back(JenkinsHash3_("light/main.frag"), 0,
                             GL_FRAGMENT_SHADER);
        return builder.CreateVectorOfStructs(objects);
      }();

      pg::ProgramBuilder pgmBuild(builder);
      pgmBuild.add_stages(stagesPtr);
      pgmBuild.add_program(-1);
      auto pgmPtr = pgmBuild.Finish();
      auto vtxPtr = builder.CreateStruct(GetBoxVertexArray());

      uint32 localPosName = JenkinsHash_("localPos");
      uint32 lightColorName = JenkinsHash_("lightColor");

      auto uniformsPtr = [&] {
        std::vector<pg::UniformValue> uniforms;
        uniforms.emplace_back(localPosName, 0, 0);
        uniforms.emplace_back(lightColorName, 0, 0);
        return builder.CreateVectorOfStructs(uniforms);
      }();

      auto texturesPtr = [&] {
        std::vector<pg::SampledTexture> textures;

        textures.emplace_back(JenkinsHash3_("res/light"),
                              JenkinsHash_("smTexture"),
                              JenkinsHash3_("res/default"), 0, 0);

        return builder.CreateVectorOfStructs(textures);
      }();

      pg::ModelRuntime runtimeDummy{};
      pg::ModelSingleBuilder mdlBuild(builder);
      mdlBuild.add_program(pgmPtr);
      mdlBuild.add_runtime(&runtimeDummy);
      mdlBuild.add_vertexArray_type(pc::ResourceVar_ptr);
      mdlBuild.add_vertexArray(vtxPtr.Union());
      mdlBuild.add_uniformValues(uniformsPtr);
      mdlBuild.add_textures(texturesPtr);

      prime::utils::FinishFlatBuffer(mdlBuild);

      auto mdlHash = pc::MakeHash<pg::ModelSingle>("light_single_model");
      pc::AddSimpleResource(pc::ResourceData{
          mdlHash,
          {
              reinterpret_cast<const char *>(builder.GetBufferPointer()),
              builder.GetSize(),
          },
      });
      auto &mdlData = pc::LoadResource(mdlHash);
      model = static_cast<pg::ModelSingle *>(pc::GetResourceHandle(mdlData));

      for (auto u : *model->uniformValues()) {
        if (u->name() == localPosName) {
          localPos = const_cast<glm::vec4 *>(
              reinterpret_cast<const glm::vec4 *>(u->data()));
        } else if (u->name() == lightColorName) {
          lightColor = const_cast<glm::vec3 *>(
              reinterpret_cast<const glm::vec3 *>(u->data()));
        }
      }
    }
  }

  void Render() { pg::Draw(*model); }
};
