#pragma once
#include "graphics/pipeline.hpp"

struct NormalVisualizer {
  prime::graphics::VertexArray *vtArray;
  float magnitude = 10;
  uint32 magnitudePos = 0;

  NormalVisualizer(prime::graphics::VertexArray *vtArray_) {
    pu::Builder<pg::Pipeline> pipelineRes;
    pipelineRes.SetArray(pipelineRes.Data().stageObjects, 3);
    pg::Pipeline *vtPip = vtArray_->pipeline;

    std::string_view definitionBuffer(vtPip->definitions.begin(),
                                      vtPip->definitions.numItems);
    pu::DefineBuilder defBuilder;

    while (definitionBuffer.size()) {
      int8 len = definitionBuffer.front();
      definitionBuffer.remove_prefix(1);
      auto item = definitionBuffer.substr(0, len);

      if (!item.starts_with("NUM_LIGHTS")) {
        defBuilder.AddDefine(item);
      }

      definitionBuffer.remove_prefix(len);
    }

    pipelineRes.SetArray(pipelineRes.Data().definitions,
                         defBuilder.buffer.size());
    memcpy(pipelineRes.Data().definitions.begin(), defBuilder.buffer.data(),
           defBuilder.buffer.size());

    auto &stageArray = pipelineRes.Data().stageObjects;

    auto &vertObj = stageArray[0];
    vertObj.stageType = GL_VERTEX_SHADER;
    vertObj.object = JenkinsHash3_("basics/ts_normal.vert");

    auto &geomObj = stageArray[1];
    geomObj.stageType = GL_GEOMETRY_SHADER;
    geomObj.object = JenkinsHash3_("basics/ts_normal.geom");

    auto &fragObj = stageArray[2];
    fragObj.stageType = GL_FRAGMENT_SHADER;
    fragObj.object = JenkinsHash3_("basics/ts_normal.frag");

    auto pipelineHash = pc::MakeHash<pg::Pipeline>("main_normal_vis");
    pc::AddSimpleResource({pipelineHash, std::move(pipelineRes.buffer)});

    prime::common::ResourceData vtArrayRes = pc::FindResource(vtArray_);
    vtArrayRes.hash.name = JenkinsHash3_("main_normal_vis_vertex_array");

    vtArray = vtArrayRes.As<prime::graphics::VertexArray>();
    vtArray->pipeline.resourceHash = pipelineHash;
    vtArray->index = 0;

    pc::AddSimpleResource({vtArrayRes.hash, std::move(vtArrayRes.buffer)});
    auto &vtRes = pc::LoadResource(vtArrayRes.hash);
    vtArray = vtRes.As<prime::graphics::VertexArray>();
    vtArray->SetupTransform(true);
    magnitudePos =
        glGetUniformLocation(vtArray->pipeline->program, "magnitude");
  }

  void Render() {
    vtArray->BeginRender();
    glUniform1f(magnitudePos, magnitude);
    vtArray->EndRender();
  }
};
