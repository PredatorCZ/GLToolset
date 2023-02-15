#include "scene_program.hpp"

struct CubeObject : MainShaderProgram {
  static inline uint32 VAOID = 0;
  static inline uint32 ITID = 0;
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

  CubeObject(prime::graphics::Pipeline *pipeline)
      : MainShaderProgram(pipeline) {
    if (VAOID < 1) {
      glGenVertexArrays(1, &VAOID);
      glBindVertexArray(VAOID);
    }

    auto &locations = pipeline->locations;

    if (ITID < 1) {
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
    UseProgram();
    glBindVertexArray(VAOID);
    glDrawArrays(GL_TRIANGLES, 0, 36);
  }
};
