#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include "datas/binreader.hpp"
#include "datas/fileinfo.hpp"
#include "datas/flags.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glm/ext.hpp>
#include <glm/gtx/dual_quaternion.hpp>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "tex_format_ref.hpp"
#include "texture.hpp"

#include "render_cube.hpp"

#include "render_box.hpp"

/*
unsigned MakeShaderProgram(TEXFlags flags) {
  GLint numActiveAttribs = 0;
  GLint numActiveUniforms = 0;
  glGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES,
                          &numActiveAttribs);
  glGetProgramInterfaceiv(program, GL_UNIFORM, GL_ACTIVE_RESOURCES,
                          &numActiveUniforms);

  std::vector<GLchar> nameData(256);
  std::vector<GLenum> properties;
  properties.push_back(GL_NAME_LENGTH);
  properties.push_back(GL_TYPE);
  properties.push_back(GL_ARRAY_SIZE);
  properties.push_back(GL_LOCATION);
  std::vector<GLint> values(properties.size());
  for (int attrib = 0; attrib < numActiveUniforms; ++attrib) {
    glGetProgramResourceiv(program, GL_UNIFORM, attrib, properties.size(),
                           &properties[0], values.size(), NULL, &values[0]);

    nameData.resize(values[0]); // The length of the name.
    glGetProgramResourceName(program, GL_UNIFORM, attrib, nameData.size(), NULL,
                             &nameData[0]);
    std::string name((char *)&nameData[0], nameData.size() - 1);
  }

  return program;
}*/

static std::function<void(GLFWwindow *, double, double)> scrollFunc;

void ScrollEvent(GLFWwindow *window, double xoffset, double yoffset) {
  scrollFunc(window, xoffset, yoffset);
}

static std::function<void(GLFWwindow *, int, int, int)> mouseFunc;

void MouseEvent(GLFWwindow *window, int button, int action, int mods) {
  mouseFunc(window, button, action, mods);
}

void ResizeWindow(GLFWwindow *, int newWidth, int newHeight) {
  glViewport(0, 0, newWidth, newHeight);
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

  auto [lightTexture, lightTextureHdr] = MakeTexture([&] {
    AFileInfo finf(argv[0]);
    std::string retval(finf.GetFolder());
    return retval + "res/light";
  }());
  auto [texture, textureHdr] = MakeTexture(argv[1]);

  auto infoText = [&, &textureHdr = textureHdr] {
    char infoBuffer[0x100];

    auto FindEnumName = [](auto what) {
      auto refl = GetReflectedEnum<decltype(what)>();
      for (size_t r = 0; r < refl->numMembers; r++) {
        if (refl->values[r] == uint64(what)) {
          return refl->names[r];
        }
      }

      return (const char *)nullptr;
    };

    auto typeStr =
        textureHdr.flags[TEXFlag::Compressed]
            ? FindEnumName(CompressedFormats(textureHdr.internalFormat))
            : FindEnumName(InternalFormats(textureHdr.internalFormat));

    snprintf(
        infoBuffer, sizeof(infoBuffer),
        "Type: %s\nWidth: %u\nHeight: %u\nMaxLevel: %u\nActiveLevel: %%u\n",
        typeStr, textureHdr.width, textureHdr.height, textureHdr.maxLevel);
    std::string retval(infoBuffer);

    return retval;
  }();

  CubeObject cubeObj(textureHdr.flags);

  float scale = 1;
  auto UpdateProjection = [&] {
    cubeObj.projection =
        glm::perspective(glm::radians(45.0f * scale), 1.f, 0.1f, 100.0f);
  };

  glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);

  int maxLevel;
  glGetTextureParameteriv(texture, GL_TEXTURE_MAX_LEVEL, &maxLevel);
  int lastLevel = 0;

  scrollFunc = [&, &textureTarget = textureHdr.target, &texture = texture](
                   GLFWwindow *window, double, double yoffset) {
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
      if (yoffset < 0) {
        cubeObj.alphaTestThreshold -= 0.05f;
      } else {
        cubeObj.alphaTestThreshold += 0.05f;
      }

      cubeObj.alphaTestThreshold =
          std::clamp(cubeObj.alphaTestThreshold, 0.f, 1.f);

      return;
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) != GLFW_PRESS) {
      if (yoffset < 0) {
        scale *= 0.75f;
      } else {
        scale *= 1.25f;
      }

      scale = std::clamp(scale, 0.001f, 2.f);
      UpdateProjection();
      return;
    }

    lastLevel += yoffset < 0 ? -1 : 1;
    lastLevel = std::max(0, lastLevel);
    lastLevel = std::min(maxLevel, lastLevel);

    glBindTexture(textureTarget, texture);
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

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  ImGui::StyleColorsDark();
  bool initedglfw = ImGui_ImplGlfw_InitForOpenGL(window, true);
  bool initedopengl = ImGui_ImplOpenGL3_Init();

  constexpr float radius = 5;
  glm::vec3 lookAtDirection{1, 0, 0};
  glm::vec2 cameraYawPitch{};
  glm::vec2 lightYawPitch{};
  glm::vec3 cameraPosition{};

  BoxObject boxObj;
  boxObj.localPos.x = 2.f;
  cubeObj.lightPos.x = radius;

  auto UpdateCamera = [&] {
    auto camera = glm::lookAt((lookAtDirection * radius), glm::vec3{},
                              glm::vec3{0, 1, 0});
    cubeObj.view = glm::dualquat(glm::quat(camera), camera[3]);
    cubeObj.view =
        glm::dualquat(glm::quat{1, 0, 0, 0}, cameraPosition) * cubeObj.view;
  };

  UpdateCamera();
  UpdateProjection();

  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  while (!glfwWindowShouldClose(window)) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) {
      glm::vec2 cursorDelta;
      int leftState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
      int rightState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);

      if (leftState == GLFW_PRESS || rightState == GLFW_PRESS) {
        glm::dvec2 cursorPos_;
        glfwGetCursorPos(window, &cursorPos_.x, &cursorPos_.y);
        glm::vec2 cursorPos = cursorPos_;
        cursorDelta = cursorPos - lastCursorPos;
        lastCursorPos = cursorPos;
        cursorDelta = (cursorDelta / 512.f) * scale;
      }

      if (leftState == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
          lightYawPitch.x -= cursorDelta.x * 2;
          lightYawPitch.y -= cursorDelta.y * 2;
          constexpr float clampPi = glm::half_pi<float>() - 0.001;
          lightYawPitch.y = glm::clamp(lightYawPitch.y, -clampPi, clampPi);
          cubeObj.lightPos.x = (cos(lightYawPitch.x) * cos(lightYawPitch.y));
          cubeObj.lightPos.y = sin(lightYawPitch.y);
          cubeObj.lightPos.z = (sin(lightYawPitch.x) * cos(lightYawPitch.y));
          boxObj.localPos = cubeObj.lightPos * 2.f;
          cubeObj.lightPos *= radius;
        } else {
          cameraYawPitch.x += cursorDelta.x * 2;
          cameraYawPitch.y += cursorDelta.y * 2;
          constexpr float clampPi = glm::half_pi<float>() - 0.001;
          cameraYawPitch.y = glm::clamp(cameraYawPitch.y, -clampPi, clampPi);
          lookAtDirection.x = (cos(cameraYawPitch.x) * cos(cameraYawPitch.y));
          lookAtDirection.y = sin(cameraYawPitch.y);
          lookAtDirection.z = (sin(cameraYawPitch.x) * cos(cameraYawPitch.y));
          UpdateCamera();
        }
      }

      if (rightState == GLFW_PRESS) {
        cameraPosition += glm::vec3(cursorDelta.x, -cursorDelta.y, 0.f);
        UpdateCamera();
      }
    }

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindTexture(textureHdr.target, texture);
    cubeObj.Render();
    glBindTexture(lightTextureHdr.target, lightTexture);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    boxObj.Render();
    glDisable(GL_BLEND);

    // ImGui::ShowDemoWindow();
    ImGui::Begin("InfoText", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoBackground |
                     ImGuiWindowFlags_NoSavedSettings);
    ImGui::SetWindowPos(ImVec2{260.f, 0.f});
    ImGui::Text(infoText.c_str(), lastLevel);
    ImGui::End();
    ImGui::Begin("Settings", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);
    ImGui::SetWindowPos(ImVec2{0.f, 0.f});
    ImGui::SetWindowSize(ImVec2{256.f, 1024.f});
    if (ImGui::CollapsingHeader("Ambient Color",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::ColorPicker3("ambColor", cubeObj.ambientColor,
                          ImGuiColorEditFlags_NoLabel |
                              ImGuiColorEditFlags_NoSmallPreview |
                              ImGuiColorEditFlags_NoSidePreview);
    }

    if (ImGui::CollapsingHeader("Light Color",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::ColorPicker3("lightColor", cubeObj.lightColor,
                          ImGuiColorEditFlags_NoLabel |
                              ImGuiColorEditFlags_NoSmallPreview |
                              ImGuiColorEditFlags_NoSidePreview);
    }

    if (ImGui::CollapsingHeader("Specular", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::SliderFloat("specLevel", &cubeObj.specLevel, 0, 100);
      ImGui::SliderFloat("specPower", &cubeObj.specPower, 0, 256);
    }
    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}