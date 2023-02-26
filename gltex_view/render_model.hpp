#include "graphics/vertex.hpp"
#include "scene_program.hpp"

struct ModelObject : MainShaderProgram {
  prime::graphics::VertexArray *vtArray;
  std::string infoText;

  ModelObject(prime::graphics::VertexArray *vtArray_)
      : MainShaderProgram(vtArray_->pipeline), vtArray(vtArray_) {
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
             vtArray->count, vtArray->maxNumUVs);
    infoText = infoBuffer;
  }

  void Render() {
    vtArray->BeginRender();
    UseProgram();
    vtArray->EndRender();
  }

  void InfoText() {
    ImGui::Text("%s", infoText.c_str());
  }
};
