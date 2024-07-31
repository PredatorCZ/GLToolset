#include "common/constants.hpp"
#include <GL/glew.h>

prime::common::Constants prime::common::GetConstants() {
  auto get64 = [](GLenum what) {
    GLint64 value;
    glGetInteger64v(what, &value);
    return uint64_t(value);
  };

  auto get = [](GLenum what) {
    GLint value;
    glGetIntegerv(what, &value);
    return uint32_t(value);
  };

  return {
      .uniform{
          get64(GL_MAX_COMPUTE_UNIFORM_COMPONENTS),
          get64(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS),
          get64(GL_MAX_VERTEX_UNIFORM_COMPONENTS),
          get64(GL_MAX_GEOMETRY_UNIFORM_COMPONENTS),
          get64(GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS),
          get64(GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS),
      },

      .uniformBuffer{
          get64(GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS),
          get64(GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS),
          get64(GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS),
          get64(GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS),
          get64(GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS),
          get64(GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS),
      },

      .ssbo{
          get(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS),
          get(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS),
          get(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS),
          get(GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS),
          get(GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS),
          get(GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS),
          get(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS),
          get(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS),
          get(GL_MAX_SHADER_STORAGE_BLOCK_SIZE),
      },

      .ubo{
          get(GL_MAX_COMPUTE_UNIFORM_BLOCKS),
          get(GL_MAX_VERTEX_UNIFORM_BLOCKS),
          get(GL_MAX_FRAGMENT_UNIFORM_BLOCKS),
          get(GL_MAX_GEOMETRY_UNIFORM_BLOCKS),
          get(GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS),
          get(GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS),
          get(GL_MAX_COMBINED_UNIFORM_BLOCKS),
          get(GL_MAX_UNIFORM_BUFFER_BINDINGS),
          get(GL_MAX_UNIFORM_BLOCK_SIZE),
      },

      .outputs{
          get(GL_MAX_DRAW_BUFFERS),
          get(GL_MAX_VERTEX_OUTPUT_COMPONENTS),
          get(GL_MAX_GEOMETRY_OUTPUT_COMPONENTS),
          get(GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS),
          get(GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS),
      },

      .inputs{
          get(GL_MAX_FRAGMENT_INPUT_COMPONENTS),
          get(GL_MAX_VERTEX_ATTRIBS),
          get(GL_MAX_GEOMETRY_INPUT_COMPONENTS),
          get(GL_MAX_TESS_CONTROL_INPUT_COMPONENTS),
          get(GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS),
      },

      .textureUnits{
          get(GL_MAX_TEXTURE_IMAGE_UNITS),
          get(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS),
          get(GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS),
          get(GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS),
          get(GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS),
      },
  };
}
