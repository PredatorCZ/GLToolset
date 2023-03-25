#pragma once
#include "render_cube.hpp"

struct AABBObject {
  static inline prime::graphics::ModelSingle *model = nullptr;

  AABBObject(const prime::graphics::VertexArray *vtArray) {
    if (!model) {
      flatbuffers::FlatBufferBuilder builder;

      auto stagesPtr = [&] {
        std::vector<pg::StageObject> objects;
        objects.emplace_back(JenkinsHash3_("basics/simple_cube.vert"), 0,
                             GL_VERTEX_SHADER);
        objects.emplace_back(JenkinsHash3_("basics/aabb_cube.frag"), 0,
                             GL_FRAGMENT_SHADER);
        return builder.CreateVectorOfStructs(objects);
      }();

      pg::ProgramBuilder pgmBuild(builder);
      pgmBuild.add_stages(stagesPtr);
      pgmBuild.add_program(-1);
      auto pgmPtr = pgmBuild.Finish();
      auto vtxPtr = builder.CreateStruct(GetCubeVertexArray());

      pg::ModelRuntime runtimeDummy{};
      pg::ModelSingleBuilder mdlBuild(builder);
      mdlBuild.add_program(pgmPtr);
      mdlBuild.add_runtime(&runtimeDummy);
      mdlBuild.add_vertexArray_type(pc::ResourceVar_ptr);
      mdlBuild.add_vertexArray(vtxPtr.Union());

      prime::utils::FinishFlatBuffer(mdlBuild);

      auto mdlHash = pc::MakeHash<pg::ModelSingle>("aabb_single_model");
      pc::AddSimpleResource(pc::ResourceData{
          mdlHash,
          {
              reinterpret_cast<const char *>(builder.GetBufferPointer()),
              builder.GetSize(),
          },
      });
      auto &mdlData = pc::LoadResource(mdlHash);
      model = static_cast<pg::ModelSingle *>(pc::GetResourceHandle(mdlData));
    }

    auto vAABB = *vtArray->aabb();
    prime::common::Transform newTM{
        {glm::quat{1.f, 0.f, 0.f, 0.f},
         glm::vec3{vAABB.center.x, vAABB.center.y, vAABB.center.z}},
        {vAABB.bounds.x, vAABB.bounds.y, vAABB.bounds.z},
    };

    pg::UpdateTransform(*model, newTM);
  }

  void Render() { pg::Draw(*model); }
};
