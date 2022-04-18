
uint32 CompileShader(uint32 type, const char *const *data, size_t numChunks) {
  unsigned shaderId = glCreateShader(type);
  glShaderSource(shaderId, numChunks, data, nullptr);
  glCompileShader(shaderId);

  {
    int success;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
    if (!success) {
      char infoLog[512]{};
      glGetShaderInfoLog(shaderId, 512, NULL, infoLog);
      printf("%s\n", infoLog);
    }
  }

  return shaderId;
}

uint32 CompileShader(uint32 type, const char *const data) {
  return CompileShader(type, &data, 1);
}

struct MainShaderProgram {
  static inline uint32 PROGRAMID = 0;
  static inline uint32 alphaTestThresholdLoc;
  static inline uint32 ambientColorLoc;
  static inline uint32 lightColorLoc;
  static inline uint32 projectionLoc;
  static inline uint32 viewLoc;
  static inline uint32 lightPosLoc;
  static inline uint32 viewPosLoc;
  static inline uint32 specPowerLoc;
  static inline uint32 specLevelLoc;

  static inline float lightColor[3]{1, 1, 1};

  float alphaTestThreshold = 0.1f;
  float ambientColor[3]{0.7, 0.7, 0.7};
  glm::vec3 lightPos{};
  float specPower = 32;
  float specLevel = 1.5;

  MainShaderProgram(TEXFlags flags) {
    if (PROGRAMID) {
      return;
    }
    static const char *const vertexShader = R"glsl(
        #version 330 core
        layout (location = 0) in vec3 inPos;
        layout (location = 1) in vec2 inTex;
        layout (location = 2) in vec3 inNormal;
        layout (location = 3) in vec4 inTangent;
        layout (location = 4) in vec4 inQTangent;

        uniform mat4 projection;
        uniform vec4 view[2];
        uniform vec3 lightPos;
        uniform vec3 viewPos;

        out vec2 TexCoord;
        out vec3 FragPos;
        out vec3 LightPos;
        out vec3 ViewPos;

        vec3 transformDQ(vec4 dq[2], vec3 point) {
          vec3 real = dq[0].yzw;
          vec3 dual = dq[1].yzw;
          vec3 crs0 = cross(real, cross(real, point) + point * dq[0].x + dual);
          vec3 crs1 = (crs0 + dual * dq[0].x - real * dq[1].x) * 2 + point;
          return crs1;
        }

        vec3 transformQ(vec4 q, vec3 point) {
          vec3 qvec = q.yzw;
          vec3 uv = cross(qvec, point);
          vec3 uuv = cross(qvec, uv);

          return point + ((uv * q.x) + uuv) * 2;
        }

        void main()
        {
            gl_Position = projection * vec4(transformDQ(view, inPos.xyz), 1.0);
            TexCoord = inTex;

            /*mat3 TBN;
            TBN[2] = inNormal;
            TBN[0] = inTangent.xyz;
            TBN[1] = cross(inTangent.xyz, inNormal) * inTangent.w;

            mat3 TBNt = transpose(TBN);
            FragPos = TBNt * inPos.xyz;
            LightPos = TBNt * lightPos;
            ViewPos = TBNt * viewPos;*/

            FragPos = transformQ(inQTangent, inPos.xyz);
            LightPos = transformQ(inQTangent, lightPos);
            ViewPos = transformQ(inQTangent, viewPos);
        }
    )glsl";

    uint32 vshId = CompileShader(GL_VERTEX_SHADER, vertexShader);

    static const char fragmentShaderBegin[] = "#version 330 core\n";
    const char *fragmentShaderDef0 =
        flags == TEXFlag::NormalDeriveZAxis ? "#define NORMAL_DERIVZ\n" : "";
    const char *fragmentShaderDef1 =
        flags == TEXFlag::SignedNormal ? "#define NORMAL_SIGNED\n" : "";

    static const char fragmentShaderBody[] = R"glsl(
        out vec4 FragColor;

        in vec2 TexCoord;
        in vec3 FragPos;
        in vec3 LightPos;
        in vec3 ViewPos;

        uniform sampler2D texture0;
        uniform float alphaTestThreshold = 0.1f;
        uniform vec3 ambient;
        uniform vec3 lightColor;
        uniform float specPower = 32;
        uniform float specLevel;

        void main()
        {
            #ifdef NORMAL_DERIVZ
                #ifdef NORMAL_SIGNED
                    vec2 normalTexture = texture(texture0, TexCoord).xy;
                #else
                    vec2 normalTexture = -1.f + (texture(texture0, TexCoord).xy) * 2.f;
                #endif

                normalTexture *= vec2(1, -1);

                float derived = clamp(dot(normalTexture, normalTexture), 0, 1);
                vec3 outNormal = vec3(normalTexture, sqrt(1.f - derived));
                vec3 normal = outNormal;

                FragColor = vec4(1.f);
            #else
                vec3 normal = 0.f;
                FragColor = texture(texture0, TexCoord);
            #endif

            vec3 lightDir = normalize(LightPos - FragPos);
            vec3 light = max(dot(normal, lightDir), 0.0) * lightColor;

            vec3 viewDir = normalize(ViewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, normal);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), specPower);
            vec3 specular = specLevel * spec * lightColor;

            FragColor = vec4((ambient + light + specular) * FragColor.xyz, 1);
        }
    )glsl";

    const char *fragmentShader[]{
        fragmentShaderBegin,
        fragmentShaderDef0,
        fragmentShaderDef1,
        fragmentShaderBody,
    };

    uint32 pshId = CompileShader(GL_FRAGMENT_SHADER, fragmentShader, 4);

    PROGRAMID = glCreateProgram();
    glAttachShader(PROGRAMID, vshId);
    glAttachShader(PROGRAMID, pshId);
    glLinkProgram(PROGRAMID);
    glDeleteShader(vshId);
    glDeleteShader(pshId);

    alphaTestThresholdLoc =
        glGetUniformLocation(PROGRAMID, "alphaTestThreshold");
    ambientColorLoc = glGetUniformLocation(PROGRAMID, "ambient");
    lightColorLoc = glGetUniformLocation(PROGRAMID, "lightColor");
    viewPosLoc = glGetUniformLocation(PROGRAMID, "viewPos");
    lightPosLoc = glGetUniformLocation(PROGRAMID, "lightPos");
    projectionLoc = glGetUniformLocation(PROGRAMID, "projection");
    viewLoc = glGetUniformLocation(PROGRAMID, "view");
    specPowerLoc = glGetUniformLocation(PROGRAMID, "specPower");
    specLevelLoc = glGetUniformLocation(PROGRAMID, "specLevel");
  }
};

struct RenderObject {
  static inline glm::mat4 projection;
  static inline glm::dualquat view;
};

#include "mikktspace.h"

struct CubeObject : RenderObject, MainShaderProgram {
  static inline uint32 VAOID = 0;

  CubeObject(TEXFlags flags) : MainShaderProgram(flags) {
    if (VAOID) {
      return;
    }

    static int8 boxVerts[]{
        1,  -1, 1,  0, 1, 0,  -1, 0,  -1, 0, 0, -1,//
        -1, -1, 1,  1, 1, 0,  -1, 0,  -1, 0, 0, -1,//
        -1, -1, -1, 1, 0, 0,  -1, 0,  -1, 0, 0, -1,//
        1,  1,  -1, 1, 0, 0,  1,  0,  1, 0, 0, -1,//
        -1, 1,  -1, 0, 0, 0,  1,  0,  1, 0, 0, -1,//
        -1, 1,  1,  0, 1, 0,  1,  0,  1, 0, 0, -1,//
        1,  1,  -1, 1, 0, 1,  0,  0,  0, 0, -1, -1,//
        1,  1,  1,  0, 0, 1,  0,  0,  0, 0, -1, -1,//
        1,  -1, 1,  0, 1, 1,  0,  0,  0, 0, -1, -1,//
        1,  -1, 1,  1, 1, 0,  0,  1,  1, 0, 0, -1,//
        1,  1,  1,  1, 0, 0,  0,  1,  1, 0, 0, -1,//
        -1, 1,  1,  0, 0, 0,  0,  1,  1, 0, 0, -1,//
        -1, 1,  1,  1, 0, -1, 0,  0,  0, 0, 1, -1,//
        -1, 1,  -1, 0, 0, -1, 0,  0,  0, 0, 1, -1,//
        -1, -1, -1, 0, 1, -1, 0,  0,  0, 0, 1, -1,//
        1,  1,  -1, 0, 0, 0,  0,  -1, -1, 0, 0, -1,//
        1,  -1, -1, 0, 1, 0,  0,  -1, -1, 0, 0, -1,//
        -1, -1, -1, 1, 1, 0,  0,  -1, -1, 0, 0, -1,//
        1,  -1, -1, 0, 0, 0,  -1, 0,  -1, 0, 0, -1,//
        1,  -1, 1,  0, 1, 0,  -1, 0,  -1, 0, 0, -1,//
        -1, -1, -1, 1, 0, 0,  -1, 0,  -1, 0, 0, -1,//
        1,  1,  1,  1, 1, 0,  1,  0,  1, 0, 0, -1,//
        1,  1,  -1, 1, 0, 0,  1,  0,  1, 0, 0, -1,//
        -1, 1,  1,  0, 1, 0,  1,  0,  1, 0, 0, -1,//
        1,  -1, -1, 1, 1, 1,  0,  0,  0, 0, -1, -1,//
        1,  1,  -1, 1, 0, 1,  0,  0,  0, 0, -1, -1,//
        1,  -1, 1,  0, 1, 1,  0,  0,  0, 0, -1, -1,//
        -1, -1, 1,  0, 1, 0,  0,  1,  1, 0, 0, -1,//
        1,  -1, 1,  1, 1, 0,  0,  1,  1, 0, 0, -1,//
        -1, 1,  1,  0, 0, 0,  0,  1,  1, 0, 0, -1,//
        -1, -1, 1,  1, 1, -1, 0,  0,  0, 0, 1, -1,//
        -1, 1,  1,  1, 0, -1, 0,  0,  0, 0, 1, -1,//
        -1, -1, -1, 0, 1, -1, 0,  0,  0, 0, 1, -1,//
        -1, 1,  -1, 1, 0, 0,  0,  -1, -1, 0, 0, -1,//
        1,  1,  -1, 0, 0, 0,  0,  -1, -1, 0, 0, -1,//
        -1, -1, -1, 1, 1, 0,  0,  -1, -1, 0, 0, -1,//
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
      printf("%.0f, %.0f, %.0f, %.0f\n", tangentSpace[i * 4], tangentSpace[1 + i * 4],
             tangentSpace[2 + i * 4], tangentSpace[3 + i * 4]);
    }*/


    glGenVertexArrays(1, &VAOID);
    glBindVertexArray(VAOID);
    uint32 vertexBuffer;
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(boxVerts), boxVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_BYTE, GL_FALSE, 12, (void *)0);
    glVertexAttribPointer(1, 2, GL_BYTE, GL_FALSE, 12, (void *)3);
    glVertexAttribPointer(2, 3, GL_BYTE, GL_FALSE, 12, (void *)5);
    glVertexAttribPointer(3, 4, GL_BYTE, GL_FALSE, 12, (void *)8);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);

    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tangentSpace), tangentSpace, GL_STATIC_DRAW);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 16, (void *)0);
    glEnableVertexAttribArray(4);
  }

  void Render() {
    glUseProgram(PROGRAMID);
    glBindVertexArray(VAOID);
    glUniform1f(alphaTestThresholdLoc, alphaTestThreshold);
    glUniform3fv(ambientColorLoc, 1, ambientColor);
    glUniform3fv(lightColorLoc, 1, lightColor);
    glm::vec3 viewPos = glm::vec3{} * view;
    glUniform3fv(viewPosLoc, 1, reinterpret_cast<float *>(&viewPos));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE,
                       reinterpret_cast<float *>(&projection));
    glUniform4fv(viewLoc, 2, reinterpret_cast<float *>(&view));
    glUniform3fv(lightPosLoc, 1, reinterpret_cast<float *>(&lightPos));
    glUniform1f(specLevelLoc, specLevel);
    glUniform1f(specPowerLoc, specPower);
    glDrawArrays(GL_TRIANGLES, 0, 36);
  }
};