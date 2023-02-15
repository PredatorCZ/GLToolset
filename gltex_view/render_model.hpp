#include "graphics/vertex.hpp"
#include "scene_program.hpp"

struct ModelObject : MainShaderProgram {
  prime::graphics::VertexArray *vtArray;
  std::string infoText;

  ModelObject(prime::graphics::VertexArray *vtArray_)
      : MainShaderProgram(vtArray_->pipeline), vtArray(vtArray_) {}

  void Render() {
    vtArray->BeginRender();
    UseProgram();
    vtArray->EndRender();
  }
};
