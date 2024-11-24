#pragma once
#include "graphics/detail/vertex_array.hpp"
#include "render_cube.hpp"

struct AABBObject {
  static inline prime::graphics::ModelSingle *model = nullptr;

  AABBObject(prime::graphics::VertexArray *vtArray) {
    if (!model) {
      auto mdlHash = pc::MakeHash<pg::ModelSingle>("editor/aabb_single_model");
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
