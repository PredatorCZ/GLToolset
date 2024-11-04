#include "graphics/detail/model_single.hpp"
#include "graphics/detail/vertex_array.hpp"
#include "scene_program.hpp"

struct ModelObject : MainShaderProgram {
  const prime::graphics::ModelSingle *mdl;
  prime::graphics::VertexArray *vtArray;
  std::string infoText;

  ModelObject(prime::graphics::ModelSingle *mdl_)
      : MainShaderProgram(&mdl_->program), mdl(mdl_),
        vtArray(mdl_->vertexArray) {
    char infoBuffer[0x100]{};
    uint32 numVerts = 0;

    for (auto &v : vtArray->buffers) {
      if (v.target == GL_ARRAY_BUFFER) {
        numVerts = v.size / v.stride;
        break;
      }
    }

    snprintf(infoBuffer, sizeof(infoBuffer),
             "Num vertices: %u\nNum indices: %u\nNum UVs: %u\n", numVerts,
             vtArray->count, vtArray->uvTransforRemaps.numItems);
    infoText = infoBuffer;
  }

  void Render() {
    UseProgram();
    prime::graphics::Draw(*mdl);
  }

  void InfoText() { ImGui::Text("%s", infoText.c_str()); }
};
