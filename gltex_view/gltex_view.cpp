#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include "datas/binreader.hpp"
#include "datas/flags.hpp"

#include <glm/ext.hpp>

enum TEXFlag : uint16 {
  Cubemap,
  Array,
  Volume,
  Compressed,
  AlphaMasked,
  NormalDeriveZAxis,
  SignedNormal,
  Swizzle,
};

struct TEXEntry {
  uint16 target;
  uint8 level;
  uint8 reserved;
  uint32 bufferSize;
  uint64 bufferOffset;
};

using TEXFlags = es::Flags<TEXFlag>;

struct TEX {
  static constexpr uint32 ID = CompileFourCC("TEX0");
  uint32 id;
  uint32 dataSize;
  uint16 numEnries;
  uint16 width;
  uint16 height;
  uint16 depth;
  uint16 internalFormat;
  uint16 format;
  uint16 type;
  uint16 target;
  uint8 maxLevel;
  uint8 numDims;
  TEXFlags flags;
  int32 swizzleMask[4];
};

void ResizeWindow(GLFWwindow *, int newWidth, int newHeight) {
  glViewport(0, 0, newWidth, newHeight);
}

unsigned MakeShaderProgram(TEXFlags flags) {
  static const char *const vertexShader =
      R"glsl(
        #version 330 core
        layout (location = 0) in vec3 inPos;
        layout (location = 1) in vec2 inTex;
        layout (location = 2) in vec3 inNormal;

        uniform mat4 transform;

        out vec2 TexCoord;
        out vec3 Normal;

        void main()
        {
            gl_Position = transform * vec4(inPos.xyz, 1.0);
            TexCoord = inTex;
            Normal = inNormal;
        }
    )glsl";

  unsigned vshId = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vshId, 1, &vertexShader, nullptr);
  glCompileShader(vshId);
  static const char fragmentShaderBegin[] = "#version 330 core\n";
  const char *fragmentShaderDef0 =
      flags == TEXFlag::NormalDeriveZAxis ? "#define NORMAL_DERIVZ\n" : "";
  const char *fragmentShaderDef1 =
      flags == TEXFlag::SignedNormal ? "#define NORMAL_SIGNED\n" : "";

  static const char fragmentShaderBody[] =
      R"glsl(
        out vec4 FragColor;

        in vec2 TexCoord;
        in vec3 Normal;
        uniform sampler2D texture0;

        void main()
        {
            #ifdef NORMAL_DERIVZ
                #ifdef NORMAL_SIGNED
                    vec2 normalTexture = texture(texture0, TexCoord).xy;
                #else
                    vec2 normalTexture = -1.f + (texture(texture0, TexCoord).xy) * 2.f;
                #endif

                float derived = clamp(dot(normalTexture, normalTexture), 0, 1);
                vec3 outNormal = vec3(normalTexture, sqrt(1.f - derived));
                FragColor = vec4((1.f + outNormal) * 0.5f, 1);
            #else
                FragColor = texture(texture0, TexCoord);
            #endif
        }
    )glsl";

  const char *fragmentShader[]{
      fragmentShaderBegin,
      fragmentShaderDef0,
      fragmentShaderDef1,
      fragmentShaderBody,
  };

  unsigned pshId = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(pshId, 4, fragmentShader, nullptr);
  glCompileShader(pshId);

  int success;
  glGetShaderiv(pshId, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512]{};
    glGetShaderInfoLog(pshId, 512, NULL, infoLog);
    printf("%s\n", infoLog);
  }

  unsigned program = glCreateProgram();
  glAttachShader(program, vshId);
  glAttachShader(program, pshId);
  glLinkProgram(program);
  glDeleteShader(vshId);
  glDeleteShader(pshId);

  return program;
}

unsigned MakeVertexObjects() {
  static int8 boxVerts[]{
      1,  -1, 1,  0, 1, 0,  -1, 0,  //
      -1, -1, 1,  1, 1, 0,  -1, 0,  //
      -1, -1, -1, 1, 0, 0,  -1, 0,  //
      1,  1,  -1, 1, 0, 0,  1,  0,  //
      -1, 1,  -1, 0, 0, 0,  1,  0,  //
      -1, 1,  1,  0, 1, 0,  1,  0,  //
      1,  1,  -1, 1, 0, 1,  0,  0,  //
      1,  1,  1,  0, 0, 1,  0,  0,  //
      1,  -1, 1,  0, 1, 1,  0,  0,  //
      1,  -1, 1,  1, 1, 0,  0,  1,  //
      1,  1,  1,  1, 0, 0,  0,  1,  //
      -1, 1,  1,  0, 0, 0,  0,  1,  //
      -1, 1,  1,  1, 0, -1, 0,  0,  //
      -1, 1,  -1, 0, 0, -1, 0,  0,  //
      -1, -1, -1, 0, 1, -1, 0,  0,  //
      1,  1,  -1, 0, 0, 0,  0,  -1, //
      1,  -1, -1, 0, 1, 0,  0,  -1, //
      -1, -1, -1, 1, 1, 0,  0,  -1, //
      1,  -1, -1, 0, 0, 0,  -1, 0,  //
      1,  -1, 1,  0, 1, 0,  -1, 0,  //
      -1, -1, -1, 1, 0, 0,  -1, 0,  //
      1,  1,  1,  1, 1, 0,  1,  0,  //
      1,  1,  -1, 1, 0, 0,  1,  0,  //
      -1, 1,  1,  0, 1, 0,  1,  0,  //
      1,  -1, -1, 1, 1, 1,  0,  0,  //
      1,  1,  -1, 1, 0, 1,  0,  0,  //
      1,  -1, 1,  0, 1, 1,  0,  0,  //
      -1, -1, 1,  0, 1, 0,  0,  1,  //
      1,  -1, 1,  1, 1, 0,  0,  1,  //
      -1, 1,  1,  0, 0, 0,  0,  1,  //
      -1, -1, 1,  1, 1, -1, 0,  0,  //
      -1, 1,  1,  1, 0, -1, 0,  0,  //
      -1, -1, -1, 0, 1, -1, 0,  0,  //
      -1, 1,  -1, 1, 0, 0,  0,  -1, //
      1,  1,  -1, 0, 0, 0,  0,  -1, //
      -1, -1, -1, 1, 1, 0,  0,  -1, //
  };

  unsigned vertArrays;
  glGenVertexArrays(1, &vertArrays);
  unsigned buf[1];
  glGenBuffers(1, buf);

  glBindVertexArray(vertArrays);

  glBindBuffer(GL_ARRAY_BUFFER, buf[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(boxVerts), boxVerts, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_BYTE, GL_FALSE, 8, (void *)0);
  glVertexAttribPointer(1, 2, GL_BYTE, GL_FALSE, 8, (void *)3);
  glVertexAttribPointer(2, 3, GL_BYTE, GL_FALSE, 8, (void *)5);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  return vertArrays;
}

struct NewTexture {
  uint32 object;
  uint32 type;
  TEXFlags flags;
};

int max_aniso;

NewTexture MakeTexture(std::string fileName) {
  unsigned int texture;
  glGenTextures(1, &texture);

  TEX hdr;
  std::vector<TEXEntry> entries;
  std::string buffer;

  {
    BinReader rd(fileName + ".gth");
    rd.Read(hdr);
    rd.ReadContainer(entries, hdr.numEnries);
  }
  glBindTexture(hdr.target, texture);
  glTexParameteri(hdr.target, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(hdr.target, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(hdr.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
  glTexParameteri(hdr.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(hdr.target, GL_TEXTURE_BASE_LEVEL, 0);
  glTexParameteri(hdr.target, GL_TEXTURE_MAX_LEVEL, hdr.maxLevel);
  glTexParameterf(hdr.target, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);

  if (hdr.flags == TEXFlag::Swizzle) {
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, hdr.swizzleMask);
  }

  {
    BinReader rd(fileName + ".gtb");
    rd.ReadContainer(buffer, hdr.dataSize);
  }

  if (hdr.flags == TEXFlag::Compressed) {
    if (hdr.flags == TEXFlag::Volume) {
      // not implemented
    } else if (hdr.flags == TEXFlag::Array) {
      // not implemented
    } else {
      if (hdr.numDims == 1) {
        for (auto &e : entries) {
          uint32_t width = hdr.width >> e.level;
          glCompressedTexImage1D(e.target, e.level, hdr.internalFormat, width,
                                 0, e.bufferSize,
                                 buffer.data() + e.bufferOffset);
        }
      } else if (hdr.numDims == 2) {
        for (auto &e : entries) {
          uint32_t height = hdr.height >> e.level;
          uint32_t width = hdr.width >> e.level;
          glCompressedTexImage2D(e.target, e.level, hdr.internalFormat, width,
                                 height, 0, e.bufferSize,
                                 buffer.data() + e.bufferOffset);
        }
      }
    }
  } else {
    if (hdr.flags == TEXFlag::Volume) {
      // not implemented
    } else if (hdr.flags == TEXFlag::Array) {
      // not implemented
    } else {
      if (hdr.numDims == 1) {
        for (auto &e : entries) {
          uint32_t width = hdr.width >> e.level;
          glTexImage1D(e.target, e.level, hdr.internalFormat, width, 0,
                       hdr.format, hdr.type, buffer.data() + e.bufferOffset);
        }
      } else if (hdr.numDims == 2) {
        for (auto &e : entries) {
          uint32_t height = hdr.height >> e.level;
          uint32_t width = hdr.width >> e.level;
          glTexImage2D(e.target, e.level, hdr.internalFormat, width, height, 0,
                       hdr.format, hdr.type, buffer.data() + e.bufferOffset);
        }
      }
    }
  }

  return {texture, hdr.target, hdr.flags};
};

static std::function<void(GLFWwindow *, double, double)> scrollFunc;

void ScrollEvent(GLFWwindow *window, double xoffset, double yoffset) {
  scrollFunc(window, xoffset, yoffset);
}

static std::function<void(GLFWwindow *, int, int, int)> mouseFunc;

void MouseEvent(GLFWwindow *window, int button, int action, int mods) {
  mouseFunc(window, button, action, mods);
}

int main(int argc, char *argv[]) {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window =
      glfwCreateWindow(1024, 1024, "GLTex Viewer", nullptr, nullptr);

  if (!window) {
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, ResizeWindow);

  GLenum err = glewInit();

  if (GLEW_OK != err) {
    glfwTerminate();
    return 2;
  }

  glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);

  auto vertexArrays = MakeVertexObjects();
  auto [texture, textureTarget, flags] = MakeTexture(argv[1]);
  auto mainProg = MakeShaderProgram(flags);
  int maxLevel;
  glGetTextureParameteriv(texture, GL_TEXTURE_MAX_LEVEL, &maxLevel);
  int lastLevel = 0;
  float scale = 1;
  scrollFunc = [&, &textureTarget = textureTarget, &texture = texture](
                   GLFWwindow *window, double, double yoffset) {
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) != GLFW_PRESS) {
      if (yoffset < 0) {
        scale *= 0.75f;
      } else {
        scale *= 1.25f;
      }

      scale = std::clamp(scale, 0.001f, 2.f);
      return;
    }

    lastLevel += yoffset < 0 ? -1 : 1;
    lastLevel = std::max(0, lastLevel);
    lastLevel = std::min(maxLevel, lastLevel);

    glTexParameteri(textureTarget, GL_TEXTURE_BASE_LEVEL, lastLevel);
  };

  glm::vec2 lastCursorPos;

  mouseFunc = [&](GLFWwindow *window, int, int action, int) {
    if (action == GLFW_PRESS) {
      glm::dvec2 cursorPos_;
      glfwGetCursorPos(window, &cursorPos_.x, &cursorPos_.y);
      lastCursorPos = cursorPos_;
    }
  };

  glfwSetScrollCallback(window, ScrollEvent);
  glfwSetMouseButtonCallback(window, MouseEvent);

  unsigned int transformLoc = glGetUniformLocation(mainProg, "transform");
  glm::mat4 modelTM(1.f);
  modelTM[3].z = -5;

  glEnable(GL_CULL_FACE);

  while (!glfwWindowShouldClose(window)) {
    glm::vec2 cursorDelta;
    int leftState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    int middleState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE);

    if (leftState == GLFW_PRESS || middleState == GLFW_PRESS) {
      glm::dvec2 cursorPos_;
      glfwGetCursorPos(window, &cursorPos_.x, &cursorPos_.y);
      glm::vec2 cursorPos = cursorPos_;
      cursorDelta = cursorPos - lastCursorPos;
      lastCursorPos = cursorPos;
      cursorDelta = (cursorDelta / 512.f) * scale;
    }

    if (middleState == GLFW_PRESS) {
      modelTM = glm::rotate(modelTM, cursorDelta.x, glm::vec3(0.f, 1.f, 0.f));
      modelTM = glm::rotate(modelTM, cursorDelta.y, glm::vec3(1.f, 0.f, 0.f));
    }

    if (leftState == GLFW_PRESS) {
      modelTM[3] += glm::vec4(cursorDelta.x, -cursorDelta.y, 0.f, 0.f);
    }

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(mainProg);

    /*glm::mat4 mtx2(1.f);
    mtx2 = glm::scale(mtx2, glm::vec3(scale));*/
    auto proj = glm::perspective(glm::radians(45.0f * scale), 800.0f / 600.0f,
                                 0.1f, 100.0f);

    auto mtx = proj * modelTM;

    glUniformMatrix4fv(transformLoc, 1, GL_FALSE,
                       reinterpret_cast<float *>(&mtx));
    glBindVertexArray(vertexArrays);
    glBindTexture(textureTarget, texture);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}