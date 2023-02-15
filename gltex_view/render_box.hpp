#pragma once
#include "graphics/pipeline.hpp"

struct BoxObject {
  static inline uint32 VAOID = 0;
  static inline uint32 localPosLoc = 0;
  static inline uint32 lightColorLoc = 0;
  glm::vec4 localPos;
  glm::vec3 lightColor;

  BoxObject(prime::graphics::Pipeline &pipeline) {
    if (VAOID < 1) {
      glGenVertexArrays(1, &VAOID);
      glBindVertexArray(VAOID);
    }
    localPosLoc = glGetUniformLocation(pipeline.program, "localPos");
    lightColorLoc = glGetUniformLocation(pipeline.program, "lightColor");
  }

  void Render() {
    glUniform4fv(localPosLoc, 1, reinterpret_cast<float *>(&localPos));
    glUniform3fv(lightColorLoc, 1, reinterpret_cast<float *>(&lightColor));
    glBindVertexArray(VAOID);
    glDrawArrays(GL_TRIANGLES, 0, 6);
  }
};
