#include "graphics/pipeline.hpp"
#include "shader_classes.hpp"
#include "utils/shader_preprocessor.hpp"

struct MainShaderProgram {
  static constexpr size_t NUM_LIGHTS = 1;
  static inline uint32 LUB;
  static inline uint32 LDUB;

  static inline prime::shaders::ubLightData<NUM_LIGHTS> lightData{
      prime::shaders::PointLight{{1.f, 1.f, 1.f}, true, {1.f, 0.05f, 0.025f}},
      {}};
  prime::shaders::ubLights<NUM_LIGHTS> lights{};

  MainShaderProgram(prime::graphics::TextureFlags flags,
                    prime::graphics::Pipeline &pipeline) {
    if (flags[prime::graphics::TextureFlag::NormalMap]) {
      lightData.pointLight[0].color = glm::vec3{0.7f};
    }

    auto &locations = pipeline.locations;

    glUniformBlockBinding(pipeline.program, locations.ubLights,
                          locations.ubLights);
    glUniformBlockBinding(pipeline.program, locations.ubLightData,
                          locations.ubLightData);

    glGenBuffers(1, &LUB);
    glBindBuffer(GL_UNIFORM_BUFFER, LUB);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(lights), &lights, GL_STREAM_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, locations.ubLights, LUB);

    glGenBuffers(1, &LDUB);
    glBindBuffer(GL_UNIFORM_BUFFER, LDUB);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(lightData), &lightData,
                 GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, locations.ubLightData, LDUB);
  }

  void UseProgram() {
    glBindBuffer(GL_UNIFORM_BUFFER, LUB);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lights), &lights);
    glBindBuffer(GL_UNIFORM_BUFFER, LDUB);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lightData), &lightData);
  }
};

#include "mikktspace.h"

struct CubeObject : MainShaderProgram {
  static inline uint32 VAOID = 0;
  static inline uint32 ITID = 0;
  prime::shaders::InstanceTransforms<1, 1> transforms{
      {{0.f, 0.f, 1.f, 1.f}}, {glm::quat{1, 0, 0, 0}}, {{1.f, 1.f, 1.f}}};

  CubeObject(prime::graphics::TextureFlags flags,
             prime::graphics::Pipeline &pipeline)
      : MainShaderProgram(flags, pipeline) {
    if (VAOID) {
      return;
    }

    // pos3 uv2 normal3 tangent4
    static int8 boxVerts[]{
        1,  -1, 1,  0, 1, 0,  -1, 0,  -1, 0, 0,  -1, //
        -1, -1, 1,  1, 1, 0,  -1, 0,  -1, 0, 0,  -1, //
        -1, -1, -1, 1, 0, 0,  -1, 0,  -1, 0, 0,  -1, //
        1,  1,  -1, 1, 0, 0,  1,  0,  1,  0, 0,  -1, //
        -1, 1,  -1, 0, 0, 0,  1,  0,  1,  0, 0,  -1, //
        -1, 1,  1,  0, 1, 0,  1,  0,  1,  0, 0,  -1, //
        1,  1,  -1, 1, 0, 1,  0,  0,  0,  0, -1, -1, //
        1,  1,  1,  0, 0, 1,  0,  0,  0,  0, -1, -1, //
        1,  -1, 1,  0, 1, 1,  0,  0,  0,  0, -1, -1, //
        1,  -1, 1,  1, 1, 0,  0,  1,  1,  0, 0,  -1, //
        1,  1,  1,  1, 0, 0,  0,  1,  1,  0, 0,  -1, //
        -1, 1,  1,  0, 0, 0,  0,  1,  1,  0, 0,  -1, //
        -1, 1,  1,  1, 0, -1, 0,  0,  0,  0, 1,  -1, //
        -1, 1,  -1, 0, 0, -1, 0,  0,  0,  0, 1,  -1, //
        -1, -1, -1, 0, 1, -1, 0,  0,  0,  0, 1,  -1, //
        1,  1,  -1, 0, 0, 0,  0,  -1, -1, 0, 0,  -1, //
        1,  -1, -1, 0, 1, 0,  0,  -1, -1, 0, 0,  -1, //
        -1, -1, -1, 1, 1, 0,  0,  -1, -1, 0, 0,  -1, //
        1,  -1, -1, 0, 0, 0,  -1, 0,  -1, 0, 0,  -1, //
        1,  -1, 1,  0, 1, 0,  -1, 0,  -1, 0, 0,  -1, //
        -1, -1, -1, 1, 0, 0,  -1, 0,  -1, 0, 0,  -1, //
        1,  1,  1,  1, 1, 0,  1,  0,  1,  0, 0,  -1, //
        1,  1,  -1, 1, 0, 0,  1,  0,  1,  0, 0,  -1, //
        -1, 1,  1,  0, 1, 0,  1,  0,  1,  0, 0,  -1, //
        1,  -1, -1, 1, 1, 1,  0,  0,  0,  0, -1, -1, //
        1,  1,  -1, 1, 0, 1,  0,  0,  0,  0, -1, -1, //
        1,  -1, 1,  0, 1, 1,  0,  0,  0,  0, -1, -1, //
        -1, -1, 1,  0, 1, 0,  0,  1,  1,  0, 0,  -1, //
        1,  -1, 1,  1, 1, 0,  0,  1,  1,  0, 0,  -1, //
        -1, 1,  1,  0, 0, 0,  0,  1,  1,  0, 0,  -1, //
        -1, -1, 1,  1, 1, -1, 0,  0,  0,  0, 1,  -1, //
        -1, 1,  1,  1, 0, -1, 0,  0,  0,  0, 1,  -1, //
        -1, -1, -1, 0, 1, -1, 0,  0,  0,  0, 1,  -1, //
        -1, 1,  -1, 1, 0, 0,  0,  -1, -1, 0, 0,  -1, //
        1,  1,  -1, 0, 0, 0,  0,  -1, -1, 0, 0,  -1, //
        -1, -1, -1, 1, 1, 0,  0,  -1, -1, 0, 0,  -1, //
    };

    static glm::quat tangentSpace[36];

    for (size_t i = 0; i < 36; i++) {
      const size_t normalOffset = 12 * i + 5;
      glm::mat3 mtx{1.f};
      mtx[2].x = boxVerts[normalOffset];
      mtx[2].y = boxVerts[normalOffset + 1];
      mtx[2].z = boxVerts[normalOffset + 2];
      mtx[0].x = boxVerts[normalOffset + 3];
      mtx[0].y = boxVerts[normalOffset + 4];
      mtx[0].z = boxVerts[normalOffset + 5];
      mtx[1] = glm::cross(mtx[0], mtx[2]) * float(boxVerts[normalOffset + 6]);

      tangentSpace[i] = mtx;
      tangentSpace[i] = glm::conjugate(tangentSpace[i]);
    }

    /*SMikkTSpaceContext ctx{};
    SMikkTSpaceInterface it{};
    it.m_getNumFaces = [](const SMikkTSpaceContext *) { return 12; };
    it.m_getNumVerticesOfFace = [](const SMikkTSpaceContext *, int) {
      return 3;
    };
    it.m_getPosition = [](const SMikkTSpaceContext *, float *out, int face,
                          int vert) {
      static float pos[3]{};
      const size_t strideOffset = 8 * 3 * face;
      const size_t vertexOffset = 8 * vert;
      const size_t offset = strideOffset + vertexOffset;
      pos[0] = boxVerts[offset + 0];
      pos[1] = boxVerts[offset + 1];
      pos[2] = boxVerts[offset + 2];
      memcpy(out, pos, sizeof(pos));
    };

    it.m_getNormal = [](const SMikkTSpaceContext *, float *out, int face,
                        int vert) {
      static float norm[3]{};
      const size_t strideOffset = 8 * 3 * face;
      const size_t vertexOffset = 8 * vert;
      const size_t offset = strideOffset + vertexOffset;
      norm[0] = boxVerts[offset + 5];
      norm[1] = boxVerts[offset + 6];
      norm[2] = boxVerts[offset + 7];
      memcpy(out, norm, sizeof(norm));
    };

    it.m_getTexCoord = [](const SMikkTSpaceContext *, float *out, int face,
                          int vert) {
      static float uv[2]{};
      const size_t strideOffset = 8 * 3 * face;
      const size_t vertexOffset = 8 * vert;
      const size_t offset = strideOffset + vertexOffset;
      uv[0] = boxVerts[offset + 3];
      uv[1] = boxVerts[offset + 4];
      memcpy(out, uv, sizeof(uv));
    };

    static float tangentSpace[36 * 4]{};

    it.m_setTSpaceBasic = [](const SMikkTSpaceContext *, const float *fvTangent,
                             float fSign, int iFace, int iVert) {
      const size_t strideOffset = 4 * 3 * iFace;
      const size_t vertexOffset = 4 * iVert;
      const size_t offset = strideOffset + vertexOffset;
      memcpy(&tangentSpace[offset], fvTangent, 12);
      tangentSpace[offset + 3] = fSign;
    };

    ctx.m_pInterface = &it;

    genTangSpaceDefault(&ctx);
    for (size_t i = 0; i < 36; i++) {
      printf("%.0f, %.0f, %.0f, %.0f\n", tangentSpace[i * 4], tangentSpace[1 + i
    * 4], tangentSpace[2 + i * 4], tangentSpace[3 + i * 4]);
    }*/

    glGenVertexArrays(1, &VAOID);
    glBindVertexArray(VAOID);
    uint32 vertexBuffer;
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(boxVerts), boxVerts, GL_STATIC_DRAW);

    auto &locations = pipeline.locations;

    glVertexAttribPointer(locations.inPos, 3, GL_BYTE, GL_FALSE, 12, (void *)0);
    glVertexAttribPointer(locations.inTexCoord20, 2, GL_BYTE, GL_FALSE, 12,
                          (void *)3);
    // glVertexAttribPointer(2, 3, GL_BYTE, GL_FALSE, 12, (void *)5);
    // glVertexAttribPointer(3, 4, GL_BYTE, GL_FALSE, 12, (void *)8);

    glEnableVertexAttribArray(locations.inPos);
    glEnableVertexAttribArray(locations.inTexCoord20);
    // glEnableVertexAttribArray(2);
    // glEnableVertexAttribArray(3);

    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tangentSpace), tangentSpace,
                 GL_STATIC_DRAW);
    glVertexAttribPointer(locations.inQTangent, 4, GL_FLOAT, GL_FALSE, 16,
                          (void *)0);
    glEnableVertexAttribArray(locations.inQTangent);

    glGenBuffers(1, &ITID);
    glBindBuffer(GL_UNIFORM_BUFFER, ITID);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(transforms), &transforms,
                 GL_STREAM_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, locations.ubInstanceTransforms, ITID);
    glUniformBlockBinding(pipeline.program, locations.ubInstanceTransforms,
                          locations.ubInstanceTransforms);
  }

  void Render() {
    UseProgram();
    glBindVertexArray(VAOID);
    glDrawArrays(GL_TRIANGLES, 0, 36);
  }
};
