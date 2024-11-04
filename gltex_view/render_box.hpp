#pragma once
#include "graphics/detail/model_single.hpp"
#include "graphics/detail/vertex_array.hpp"
#include "utils/playground.hpp"

prime::graphics::VertexArray *GetBoxVertexArray() {
  static prime::graphics::VertexArray *VT_ARRAY = nullptr;

  if (!VT_ARRAY) {
    namespace pu = prime::utils;
    pu::PlayGround vayPg;
    pu::PlayGround::Pointer<prime::graphics::VertexArray> newVay =
        vayPg.AddClass<prime::graphics::VertexArray>();

    newVay->count = 6;
    newVay->mode = GL_TRIANGLES;

    auto vayHash = pc::MakeHash<pg::VertexArray>("box_vertex");

    pc::AddSimpleResource(pc::ResourceData{
        vayHash,
        vayPg.Build(),
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
      pu::PlayGround modelPg;

      pu::PlayGround::Pointer<pg::ModelSingle> newModel =
          modelPg.AddClass<pg::ModelSingle>();

      newModel->vertexArray = GetCubeVertexArray();

      modelPg.ArrayEmplace(newModel->program.stages,
                           JenkinsHash3_("basics/simple_sprite.vert"), 0,
                           GL_VERTEX_SHADER);
      modelPg.ArrayEmplace(newModel->program.stages,
                           JenkinsHash3_("light/main.frag"), 0,
                           GL_FRAGMENT_SHADER);

      uint32 localPosName = JenkinsHash_("localPos");
      uint32 lightColorName = JenkinsHash_("lightColor");

      modelPg.ArrayEmplace(newModel->uniformValues, 0, 0, localPosName);
      modelPg.ArrayEmplace(newModel->uniformValues, 0, 0, lightColorName);

      modelPg.ArrayEmplace(newModel->textures, JenkinsHash3_("res/light"),
                           JenkinsHash_("smTexture"),
                           JenkinsHash3_("res/default"), 0, 0);

      auto mdlHash = pc::MakeHash<pg::ModelSingle>("light_single_model");
      pc::AddSimpleResource(pc::ResourceData{
          mdlHash,
          modelPg.Build(),
      });
      auto &mdlData = pc::LoadResource(mdlHash);
      model = static_cast<pg::ModelSingle *>(pc::GetResourceHandle(mdlData));

      for (auto &u : model->uniformValues) {
        if (u.name == localPosName) {
          localPos = const_cast<glm::vec4 *>(
              reinterpret_cast<const glm::vec4 *>(u.data));
        } else if (u.name == lightColorName) {
          lightColor = const_cast<glm::vec3 *>(
              reinterpret_cast<const glm::vec3 *>(u.data));
        }
      }
    }
  }

  void Render() { pg::Draw(*model); }
};
