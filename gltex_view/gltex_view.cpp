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

std::string APP_PATH;

#include "render_cube.hpp"

#include "render_box.hpp"

static std::function<void(GLFWwindow *, double, double)> scrollFunc;

void ScrollEvent(GLFWwindow *window, double xoffset, double yoffset) {
  scrollFunc(window, xoffset, yoffset);
}

static std::function<void(GLFWwindow *, int, int, int)> mouseFunc;

void MouseEvent(GLFWwindow *window, int button, int action, int mods) {
  mouseFunc(window, button, action, mods);
}

static std::function<void(GLFWwindow *, int, int)> resizeFunc;

void ResizeWindow(GLFWwindow *window, int newWidth, int newHeight) {
  resizeFunc(window, newWidth, newHeight);
}

#include "datas/master_printer.hpp"

int main(int argc, char *argv[]) {
  es::print::AddPrinterFunction(es::Print);
  AFileInfo finf(argv[0]);
  APP_PATH = finf.GetFolder();

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  int width = 1024;
  int height = 1024;

  GLFWwindow *window =
      glfwCreateWindow(width, height, "GLTex Viewer", nullptr, nullptr);

  if (!window) {
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);

  GLenum err = glewInit();

  if (GLEW_OK != err) {
    glfwTerminate();
    return 2;
  }

  auto [lightTexture, lightTextureHdr] = MakeTexture(APP_PATH + "res/light");
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
    cubeObj.vsPosition.projection = glm::perspective(
        glm::radians(45.0f * scale), float(width) / height, 0.1f, 100.0f);
  };

  resizeFunc = [&](GLFWwindow *, int newWidth, int newHeight) {
    width = newWidth;
    height = newHeight;
    glViewport(0, 0, newWidth, newHeight);
    UpdateProjection();
  };

  glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);

  int maxLevel;
  glGetTextureParameteriv(texture, GL_TEXTURE_MAX_LEVEL, &maxLevel);
  int lastLevel = 0;

  glm::vec4 lightOrbit{1, 0, 0, 1.5};

  scrollFunc = [&, &textureTarget = textureHdr.target, &texture = texture](
                   GLFWwindow *window, double, double yoffset) {
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
      return;
    }
    /*if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
      if (yoffset < 0) {
        cubeObj.alphaTestThreshold -= 0.05f;
      } else {
        cubeObj.alphaTestThreshold += 0.05f;
      }

      cubeObj.alphaTestThreshold =
          std::clamp(cubeObj.alphaTestThreshold, 0.f, 1.f);

      return;
    }*/

    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
      if (yoffset < 0) {
        lightOrbit.w -= 0.05f;
      } else {
        lightOrbit.w += 0.05f;
      }

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
  glfwSetFramebufferSizeCallback(window, ResizeWindow);

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

  auto UpdateCamera = [&] {
    auto camera = glm::lookAt((lookAtDirection * radius), glm::vec3{},
                              glm::vec3{0, 1, 0});
    cubeObj.vsPosition.view = glm::dualquat(glm::quat(camera), camera[3]);
    cubeObj.vsPosition.view =
        glm::dualquat(glm::quat{1, 0, 0, 0}, cameraPosition) *
        cubeObj.vsPosition.view;
  };

  UpdateCamera();
  UpdateProjection();

  bool lockedLight = false;

  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  while (!glfwWindowShouldClose(window)) {
    cubeObj.lights.lightPos[0] = lightOrbit * lightOrbit.w;
    boxObj.localPos = cubeObj.lights.lightPos[0];

    if (!io.WantCaptureMouse) {
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
        constexpr float clampPi = glm::half_pi<float>() - 0.001;
        const bool lightKey = glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS;
        if (lockedLight || lightKey) {
          if (lightKey) {
            lightYawPitch.x -= cursorDelta.x * 2;
            lightYawPitch.y -= cursorDelta.y * 2;
          } else {
            lightYawPitch.x += cursorDelta.x * 2;
            lightYawPitch.y += cursorDelta.y * 2;
          }

          lightYawPitch.y = glm::clamp(lightYawPitch.y, -clampPi, clampPi);
          lightOrbit.x = (cos(lightYawPitch.x) * cos(lightYawPitch.y));
          lightOrbit.y = sin(lightYawPitch.y);
          lightOrbit.z = (sin(lightYawPitch.x) * cos(lightYawPitch.y));
        }

        if (!lightKey) {
          cameraYawPitch.x += cursorDelta.x * 2;
          cameraYawPitch.y += cursorDelta.y * 2;
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

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

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
                 ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoSavedSettings);
    // ImGui::SetWindowPos(ImVec2{0.f, 0.f});
    //  ImGui::SetWindowSize(ImVec2{256.f, 1024.f});
    if (ImGui::CollapsingHeader("Ambient Color",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::ColorPicker3("ambColor", &cubeObj.fragProps.ambientColor.x,
                          ImGuiColorEditFlags_NoLabel |
                              ImGuiColorEditFlags_NoSmallPreview |
                              ImGuiColorEditFlags_NoSidePreview);
    }

    if (ImGui::CollapsingHeader("Light 0", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::ColorPicker3("color", &cubeObj.lightData.pointLight[0].color.x,
                          ImGuiColorEditFlags_NoLabel |
                              ImGuiColorEditFlags_NoSmallPreview |
                              ImGuiColorEditFlags_NoSidePreview);
      ImGui::SliderFloat("distance", &lightOrbit.w, 0, 30);
      ImGui::SliderFloat("quadAtten",
                         &cubeObj.lightData.pointLight[0].attenuation.y, 0, 1);
      ImGui::SliderFloat("cubicAtten",
                         &cubeObj.lightData.pointLight[0].attenuation.z, 0, 2);
    }

    if (ImGui::CollapsingHeader("Specular", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::SliderFloat("specLevel", &cubeObj.fragProps.specLevel, 0, 100);
      ImGui::SliderFloat("specPower", &cubeObj.fragProps.specPower, 0, 256);
    }

    ImGui::Checkbox("lockedLight", &lockedLight);

    /*if (ImGui::CollapsingHeader("Debug", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Checkbox("Use TBN", &cubeObj.useTBN);
    }*/
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
