
struct BoxObject {
  static inline uint32 VAOID = 0;
  static inline uint32 localPosLoc = 0;
  static inline uint32 lightColorLoc = 0;
  glm::vec3 localPos;

  BoxObject(prime::graphics::Pipeline &pipeline) {
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

    localPosLoc = glGetUniformLocation(pipeline.program, "localPos");
    lightColorLoc = glGetUniformLocation(pipeline.program, "lightColor");
  }

  void Render() {
    glBindVertexArray(VAOID);
    glUniform3fv(localPosLoc, 1, reinterpret_cast<float *>(&localPos));
    glUniform3fv(lightColorLoc, 1,
                 &MainShaderProgram::lightData.pointLight[0].color.x);
    glDrawArrays(GL_TRIANGLES, 0, 6);
  }
};
