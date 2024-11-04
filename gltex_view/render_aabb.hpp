#pragma once
#include "render_cube.hpp"

struct AABBObject {
  static inline prime::graphics::ModelSingle *model = nullptr;

  AABBObject(prime::graphics::VertexArray *vtArray) {
    if (!model) {
      pu::PlayGround modelPg;

      pu::PlayGround::Pointer<pg::ModelSingle> newModel =
          modelPg.AddClass<pg::ModelSingle>();

      newModel->vertexArray = vtArray;

      modelPg.ArrayEmplace(newModel->program.stages,
                           JenkinsHash3_("basics/simple_cube.vert"), 0,
                           GL_VERTEX_SHADER);
      modelPg.ArrayEmplace(newModel->program.stages,
                           JenkinsHash3_("basics/aabb_cube.frag"), 0,
                           GL_FRAGMENT_SHADER);

      auto mdlHash = pc::MakeHash<pg::ModelSingle>("aabb_single_model");
      pc::AddSimpleResource(pc::ResourceData{
          mdlHash,
          modelPg.Build(),
      });
      auto &mdlData = pc::LoadResource(mdlHash);
      model = static_cast<pg::ModelSingle *>(pc::GetResourceHandle(mdlData));
    }

    auto &vAABB = vtArray->aabb;
    prime::common::Transform newTM{
        {glm::quat{1.f, 0.f, 0.f, 0.f},
         glm::vec3{vAABB.center.x, vAABB.center.y, vAABB.center.z}},
        {vAABB.bounds.x, vAABB.bounds.y, vAABB.bounds.z},
    };

    pg::UpdateTransform(*model, newTM);
  }

  void Render() { pg::Draw(*model); }
};
