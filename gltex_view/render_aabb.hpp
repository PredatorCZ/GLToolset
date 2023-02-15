#pragma once
#include "graphics/pipeline.hpp"

struct AABBObject {
  static inline uint32 VAOID = 0;
  uint32 ITID = 0;
  prime::graphics::Pipeline *pipeline;
  prime::shaders::InstanceTransforms<1, 1> transforms{
      .uvTransform =
          {
              {0.f, 0.f, 1.f, 1.f},
          },
      .indexedModel =
          {
              {glm::quat{1.f, 0.f, 0.f, 0.f}, glm::vec3{}},
          },
      .indexedInflate =
          {
              {1.f, 1.f, 1.f},
          },
  };

  AABBObject(prime::graphics::VertexArray *vtArray) {
    if (VAOID < 1) {
      glGenVertexArrays(1, &VAOID);
      glBindVertexArray(VAOID);
      pu::Builder<pg::Pipeline> pipelineRes;
      pipelineRes.SetArray(pipelineRes.Data().stageObjects, 2);
      auto &stageArray = pipelineRes.Data().stageObjects;

      auto &vertObj = stageArray[0];
      vertObj.stageType = GL_VERTEX_SHADER;
      vertObj.object = JenkinsHash3_("basics/simple_cube.vert");

      auto &fragObj = stageArray[1];
      fragObj.stageType = GL_FRAGMENT_SHADER;
      fragObj.object = JenkinsHash3_("basics/aabb_cube.frag");

      auto pipelineHash = pc::MakeHash<pg::Pipeline>("main_aabb_cube_vis");
      pc::AddSimpleResource({pipelineHash, std::move(pipelineRes.buffer)});
      auto &res = pc::LoadResource(pipelineHash);
      pipeline = res.As<pg::Pipeline>();
    }

    auto &locations = pipeline->locations;

    if (ITID < 1) {
      transforms.indexedInflate[0] = {
          vtArray->aabb.bounds.x,
          vtArray->aabb.bounds.y,
          vtArray->aabb.bounds.z,
      };
      transforms.indexedModel[0] = {glm::quat{1.f, 0.f, 0.f, 0.f},
                                    glm::vec3{
                                        vtArray->aabb.center.x,
                                        vtArray->aabb.center.y,
                                        vtArray->aabb.center.z,
                                    }};

      glGenBuffers(1, &ITID);
      glBindBuffer(GL_UNIFORM_BUFFER, ITID);
      glBufferData(GL_UNIFORM_BUFFER, sizeof(transforms), &transforms,
                   GL_STREAM_DRAW);
    }
    glBindBufferBase(GL_UNIFORM_BUFFER, locations.ubInstanceTransforms, ITID);
    glUniformBlockBinding(pipeline->program, locations.ubInstanceTransforms,
                          locations.ubInstanceTransforms);
  }

  void Render() {
    pipeline->BeginRender();
    glBindVertexArray(VAOID);
    glBindBufferBase(GL_UNIFORM_BUFFER, pipeline->locations.ubInstanceTransforms, ITID);
    glDrawArrays(GL_TRIANGLES, 0, 36);
  }
};
