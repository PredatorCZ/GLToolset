#include "shader_compiler.hpp"
#include "shaders/shader_classes.hpp"

uint32 CompileShader(uint32 type, const char *const data) {
  return CompileShader(type, &data, 1);
}

struct MainShaderProgram {
  static constexpr size_t NUM_LIGHTS = 1;
  static inline uint32 PROGRAMID = 0;
  static inline uint64 PROGRAMHASH = 0;
  static inline BaseShaderLocations locations;
  static inline uint32 LUB;
  static inline uint32 LDUB;
  static inline uint32 FPUB;

  static inline prime::shaders::ubLightData<NUM_LIGHTS> lightData{
      prime::shaders::PointLight{{1.f, 1.f, 1.f}, true, {1.f, 0.05f, 0.025f}},
      {}};
  prime::shaders::ubLights<NUM_LIGHTS> lights{};
  prime::shaders::single_texture::ubFragmentProperties fragProps{
      {0.7, 0.7, 0.7}, 1.5, 32.f};

  MainShaderProgram(TEXFlags flags) {
    if (flags[TEXFlag::NormalMap]) {
      fragProps.ambientColor = {};
      lightData.pointLight[0].color = glm::vec3{0.7f};
    }

    if (PROGRAMID) {
      return;
    }

    VertexShaderFeatures vsFeats;
    vsFeats.tangentSpace = VSTSFeat::Quat;
    vsFeats.numLights = NUM_LIGHTS;
    // vsFeats.useInstances = true;

    ShaderObject vsh =
        CompileShaderObject(vsFeats, "shaders/single_texture/main.vert");

    FragmentShaderFeatures fsFeats;
    fsFeats.signedNormal = flags == TEXFlag::SignedNormal;
    fsFeats.deriveZNormal = flags == TEXFlag::NormalDeriveZAxis;
    fsFeats.numLights = NUM_LIGHTS;

    ShaderObject fsh = CompileShaderObject(
        fsFeats, flags == TEXFlag::NormalMap
                     ? "shaders/single_texture/main_normal.frag"
                     : "shaders/single_texture/main_albedo.frag");

    PROGRAMID = glCreateProgram();
    glAttachShader(PROGRAMID, vsh.objectId);
    glAttachShader(PROGRAMID, fsh.objectId);
    glLinkProgram(PROGRAMID);
    glDeleteShader(vsh.objectId);
    glDeleteShader(fsh.objectId);

    PROGRAMHASH = vsh.objectHash | (uint64(fsh.objectHash) << 32);

    locations = IntrospectShader(PROGRAMID);

    glUniformBlockBinding(PROGRAMID, locations.ubPosition,
                          locations.ubPosition);
    glUniformBlockBinding(PROGRAMID, locations.ubLights, locations.ubLights);
    glUniformBlockBinding(PROGRAMID, locations.ubFragmentProperties,
                          locations.ubFragmentProperties);
    glUniformBlockBinding(PROGRAMID, locations.ubLightData,
                          locations.ubLightData);
    glUniformBlockBinding(PROGRAMID, locations.ubInstanceTransforms,
                          locations.ubInstanceTransforms);

    glGenBuffers(1, &LUB);
    glBindBuffer(GL_UNIFORM_BUFFER, LUB);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(lights), &lights, GL_STREAM_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, locations.ubLights, LUB);

    glGenBuffers(1, &LDUB);
    glBindBuffer(GL_UNIFORM_BUFFER, LDUB);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(lightData), &lightData,
                 GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, locations.ubLightData, LDUB);

    glGenBuffers(1, &FPUB);
    glBindBuffer(GL_UNIFORM_BUFFER, FPUB);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(fragProps), &fragProps,
                 GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, locations.ubFragmentProperties, FPUB);
  }

  void UseProgram() {
    glUseProgram(PROGRAMID);
    glBindBuffer(GL_UNIFORM_BUFFER, LUB);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lights), &lights);
    glBindBuffer(GL_UNIFORM_BUFFER, LDUB);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lightData), &lightData);
    glBindBuffer(GL_UNIFORM_BUFFER, FPUB);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(fragProps), &fragProps);
  }
};

struct RenderObject {
  static inline uint32 UBID = 0;
  static inline prime::shaders::ubPosition vsPosition;

  RenderObject() {
    if (UBID) {
      return;
    }

    glGenBuffers(1, &UBID);
    glBindBuffer(GL_UNIFORM_BUFFER, UBID);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(vsPosition), &vsPosition,
                 GL_STREAM_DRAW);
  }
};

#include "mikktspace.h"

struct CubeObject : RenderObject, MainShaderProgram {
  static inline uint32 VAOID = 0;
  static inline uint32 ITID = 0;
  prime::shaders::InstanceTransforms<1, 1> transforms{
      {{0.f, 0.f, 1.f, 1.f}}, {glm::quat{1, 0, 0, 0}}, {{1.f, 0.5f, 1.f}}};

  CubeObject(TEXFlags flags) : MainShaderProgram(flags) {
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

    glBindBufferBase(GL_UNIFORM_BUFFER, locations.ubPosition, UBID);

    glGenBuffers(1, &ITID);
    glBindBuffer(GL_UNIFORM_BUFFER, ITID);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(transforms), &transforms,
                 GL_STREAM_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, locations.ubInstanceTransforms, ITID);
  }

  void Render() {
    lights.viewPos = glm::vec3{} * vsPosition.view;
    UseProgram();
    glBindVertexArray(VAOID);
    glBindBuffer(GL_UNIFORM_BUFFER, UBID);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(vsPosition), &vsPosition);
    glDrawArrays(GL_TRIANGLES, 0, 36);
  }
};
