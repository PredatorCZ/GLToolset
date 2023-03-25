#include "graphics/model_single.hpp"
#include "scene_program.hpp"

#include "model_single.fbs.hpp"
#include "vertex.fbs.hpp"

struct ModelObject : MainShaderProgram {
  const prime::graphics::ModelSingle *mdl;
  const prime::graphics::VertexArray *vtArray;
  std::string infoText;

  ModelObject(prime::graphics::ModelSingle *mdl_)
      : MainShaderProgram(mdl_->mutable_program()), mdl(mdl_),
        vtArray(*static_cast<prime::graphics::VertexArray *const *>(
            mdl_->vertexArray())) {
    char infoBuffer[0x100]{};
    uint32 numVerts = 0;

    for (auto v : *vtArray->buffers()) {
      if (v->target() == GL_ARRAY_BUFFER) {
        numVerts = v->size() / v->stride();
        break;
      }
    }

    snprintf(infoBuffer, sizeof(infoBuffer),
             "Num vertices: %u\nNum indices: %u\nNum UVs: %u\n", numVerts,
             vtArray->count(), vtArray->uvTransformRemaps()->size());
    infoText = infoBuffer;
  }

  void Render() {
    UseProgram();
    prime::graphics::Draw(*mdl);
  }

  void InfoText() { ImGui::Text("%s", infoText.c_str()); }
};
