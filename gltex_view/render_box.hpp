#pragma once
#include "graphics/detail/model_single.hpp"
#include "graphics/detail/vertex_array.hpp"
#include "utils/playground.hpp"

prime::graphics::VertexArray *GetBoxVertexArray() {
  static prime::graphics::VertexArray *VT_ARRAY = nullptr;

  if (!VT_ARRAY) {
    auto vayHash = pc::MakeHash<pg::VertexArray>("editor/box_vertex");
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
      auto mdlHash = pc::MakeHash<pg::ModelSingle>("editor/light_single_model");
      auto &mdlData = pc::LoadResource(mdlHash);
      model = static_cast<pg::ModelSingle *>(pc::GetResourceHandle(mdlData));

      for (auto &u : model->uniformValues) {
        if (u.name == "localPos") {
          localPos = const_cast<glm::vec4 *>(
              reinterpret_cast<const glm::vec4 *>(u.data));
        } else if (u.name == "lightColor") {
          lightColor = const_cast<glm::vec3 *>(
              reinterpret_cast<const glm::vec3 *>(u.data));
        }
      }
    }
  }

  void Render() { pg::Draw(*model); }
};
