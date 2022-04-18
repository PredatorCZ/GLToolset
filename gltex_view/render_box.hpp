
struct BoxShaderProgram {
  static inline uint32 PROGRAMID = 0;
  static inline uint32 viewLoc;
  static inline uint32 localPosLoc;
  static inline uint32 projectionLoc;
  static inline uint32 lightColorLoc;

  glm::vec3 localPos{};

  BoxShaderProgram() {
    if (PROGRAMID) {
      return;
    }

    static const char *const vertexShader = R"glsl(
        #version 330 core
        layout (location = 0) in vec2 inPos;
        layout (location = 1) in vec2 inTex;

        out vec2 TexCoord;

        uniform mat4 projection;
        uniform vec4 view[2];
        uniform vec3 localPos;

        vec3 transformDQ(vec4 dq[2], vec3 point) {
          vec3 real = dq[0].yzw;
          vec3 dual = dq[1].yzw;
          vec3 crs0 = cross(real, cross(real, point) + point * dq[0].x + dual);
          vec3 crs1 = (crs0 + dual * dq[0].x - real * dq[1].x) * 2 + point;
          return crs1;
        }

        void main()
        {
          gl_Position = projection * (vec4(transformDQ(view, localPos), 1.0) + vec4(inPos, 0, 0) * 0.1);
          TexCoord = inTex;
        }
    )glsl";

    uint32 vshId = CompileShader(GL_VERTEX_SHADER, vertexShader);

    static const char fragmentShader[] = R"glsl(
        #version 330 core
        out vec4 FragColor;
        in vec2 TexCoord;
        uniform sampler2D texture0;
        uniform vec3 lightColor;

        void main()
        {
          vec4 color = texture(texture0, TexCoord);
          FragColor = vec4(color.r * lightColor, color.r);
        }
    )glsl";

    uint32 pshId = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    PROGRAMID = glCreateProgram();
    glAttachShader(PROGRAMID, vshId);
    glAttachShader(PROGRAMID, pshId);
    glLinkProgram(PROGRAMID);
    glDeleteShader(vshId);
    glDeleteShader(pshId);

    localPosLoc = glGetUniformLocation(PROGRAMID, "localPos");
    viewLoc = glGetUniformLocation(PROGRAMID, "view");
    projectionLoc = glGetUniformLocation(PROGRAMID, "projection");
    lightColorLoc = glGetUniformLocation(PROGRAMID, "lightColor");
  }
};

struct BoxObject : RenderObject, BoxShaderProgram {
  static inline uint32 VAOID = 0;

  BoxObject() {
    if (VAOID) {
      return;
    }

    static int8 boxVerts[]{
        -1, -1, 0, 1, //
        1,  -1, 1, 1, //
        1,  1,  1, 0, //
        1,  1,  1, 0, //
        -1, 1,  0, 0, //
        -1, -1, 0, 1, //
    };

    glGenVertexArrays(1, &VAOID);
    glBindVertexArray(VAOID);
    uint32 vertexBuffer;
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(boxVerts), boxVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_BYTE, GL_FALSE, 4, (void *)0);
    glVertexAttribPointer(1, 2, GL_BYTE, GL_FALSE, 4, (void *)2);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
  }

  void Render() {
    glUseProgram(PROGRAMID);
    glBindVertexArray(VAOID);
    glUniform4fv(viewLoc, 2, reinterpret_cast<float *>(&view));
    glUniform3fv(localPosLoc, 1, reinterpret_cast<float *>(&localPos));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE,
                       reinterpret_cast<float *>(&projection));
    glUniform3fv(lightColorLoc, 1, MainShaderProgram::lightColor);
    glDrawArrays(GL_TRIANGLES, 0, 6);
  }
};