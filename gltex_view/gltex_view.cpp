#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include "datas/binreader.hpp"
#include "datas/fileinfo.hpp"
#include "datas/flags.hpp"

#include "ImGuiFileDialog.h"
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

#include "graphics/texture.hpp"
#include "tex_format_ref.hpp"

#include "render_cube.hpp"

#include "render_box.hpp"

#include "common/camera.hpp"
#include "common/resource.hpp"
#include "datas/binwritter.hpp"
#include "datas/master_printer.hpp"
#include "graphics/pipeline.hpp"
#include "graphics/sampler.hpp"
#include "utils/builder.hpp"
#include "utils/fixer.hpp"

#include "frame_buffer.hpp"

#include "simplecpp.h"

std::pair<prime::graphics::TextureUnit, prime::graphics::Texture>
MakeTexture(std::string path) {
  using namespace prime::common;
  auto hdrData = LoadResource(path + ".gth");
  auto texelData = LoadResource(path + ".gtb");
  auto &hdr = *hdrData.As<prime::graphics::Texture>();
  prime::utils::FixupClass(hdr);
  auto unit =
      prime::graphics::AddTexture(hdrData.hash, hdr, texelData.buffer.data());

  return {unit, hdr};
}

void PrecompileShaders() {
  prime::utils::SetShadersSourceDir(
      "/home/lukas/github/gltoolset/src/shaders/");

  simplecpp::DUI noDui{};
  auto vertexShader = prime::utils::PreProcess("light/main.vert", noDui);
  prime::common::AddSimpleResource({JenkinsHash3_("light/main.vert"), std::move(vertexShader)});
  auto fragmentShader = prime::utils::PreProcess("light/main.frag", noDui);
  prime::common::AddSimpleResource({JenkinsHash3_("light/main.frag"), std::move(fragmentShader)});

  prime::utils::FragmentShaderFeatures fragFeats{};
  prime::common::AddSimpleResource(prime::utils::PreprocessShaderSource(
      fragFeats, "single_texture/main_albedo.frag"));
  prime::common::AddSimpleResource(prime::utils::PreprocessShaderSource(
      fragFeats, "single_texture/main_normal.frag"));
  fragFeats.deriveZNormal = false;
  prime::common::AddSimpleResource(prime::utils::PreprocessShaderSource(
      fragFeats, "single_texture/main_normal.frag"));
  fragFeats.signedNormal = false;
  prime::common::AddSimpleResource(prime::utils::PreprocessShaderSource(
      fragFeats, "single_texture/main_normal.frag"));
  fragFeats.deriveZNormal = true;
  prime::common::AddSimpleResource(prime::utils::PreprocessShaderSource(
      fragFeats, "single_texture/main_normal.frag"));

  prime::utils::VertexShaderFeatures vertFeats{};
  vertFeats.tangentSpace = prime::utils::VSTSFeat::Quat;
  prime::common::AddSimpleResource(prime::utils::PreprocessShaderSource(
      vertFeats, "single_texture/main.vert"));
}

prime::common::ResourceData
BuildPipeline(prime::graphics::TextureFlags texFlags) {
  prime::utils::Builder<prime::graphics::Pipeline> pipeline;
  pipeline.SetArray(pipeline.Data().stageObjects, 2);
  auto stageArray = pipeline.ArrayData(pipeline.Data().stageObjects);

  auto &vertObj = stageArray[0];
  vertObj.stageType = GL_VERTEX_SHADER;
  prime::utils::VertexShaderFeatures vertFeats{};
  vertFeats.tangentSpace = prime::utils::VSTSFeat::Quat;

  vertObj.object = JenkinsHash3_("single_texture/main.vert", vertFeats.Seed());

  prime::utils::FragmentShaderFeatures fragFeats{};
  fragFeats.signedNormal =
      texFlags == prime::graphics::TextureFlag::SignedNormal;
  fragFeats.deriveZNormal =
      texFlags == prime::graphics::TextureFlag::NormalDeriveZAxis;

  auto &fragObj = stageArray[1];
  fragObj.stageType = GL_FRAGMENT_SHADER;

  const bool isNormal = texFlags == prime::graphics::TextureFlag::NormalMap;

  if (isNormal) {
    fragObj.object =
        JenkinsHash3_("single_texture/main_normal.frag", fragFeats.Seed());
  } else {
    fragObj.object =
        JenkinsHash3_("single_texture/main_albedo.frag", fragFeats.Seed());
  }

  pipeline.SetArray(pipeline.Data().textureUnits, 1);
  auto textureArray = pipeline.ArrayData(pipeline.Data().textureUnits);
  auto &texture = textureArray[0];
  texture.sampler = JenkinsHash3_("res/default.spl");
  texture.slotHash = JenkinsHash_(isNormal ? "smTSNormal" : "smAlbedo");
  texture.texture = JenkinsHash3_("main_texture");

  pipeline.SetArray(pipeline.Data().uniformBlocks, 1);
  auto uniformBlockArray = pipeline.ArrayData(pipeline.Data().uniformBlocks);
  auto &uniformBlock = uniformBlockArray[0];
  uniformBlock.dataObject = JenkinsHash3_("main_uniform");
  uniformBlock.bufferObject = JenkinsHash_("ubFragmentProperties");

  return {JenkinsHash3_("main.ppe"), std::move(pipeline.buffer)};
}

void BuildLightPipeline() {
  prime::utils::Builder<prime::graphics::Pipeline> pipeline;
  pipeline.SetArray(pipeline.Data().stageObjects, 2);
  auto stageArray = pipeline.ArrayData(pipeline.Data().stageObjects);

  auto &vertObj = stageArray[0];
  vertObj.stageType = GL_VERTEX_SHADER;
  vertObj.object = JenkinsHash3_("light/main.vert");

  auto &fragObj = stageArray[1];
  fragObj.stageType = GL_FRAGMENT_SHADER;
  fragObj.object = JenkinsHash3_("light/main.frag");

  pipeline.SetArray(pipeline.Data().textureUnits, 1);
  auto textureArray = pipeline.ArrayData(pipeline.Data().textureUnits);
  auto &texture = textureArray[0];
  texture.sampler = JenkinsHash3_("res/default.spl");
  texture.slotHash = JenkinsHash_("smTexture");
  texture.texture = JenkinsHash3_("res/light.gth");

  BinWritter wr(prime::utils::ShadersSourceDir() + "../res/light.ppe");
  wr.WriteContainer(pipeline.buffer);
}

int main(int, char *argv[]) {
  es::print::AddPrinterFunction(es::Print);
  AFileInfo finf(argv[0]);

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  int width = 1800;
  int height = 1020;

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

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  ImGui::StyleColorsDark();
  bool initedglfw = ImGui_ImplGlfw_InitForOpenGL(window, true);
  bool initedopengl = ImGui_ImplOpenGL3_Init();

  int maxAniso;
  int anisotropy = 8;
  glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
  prime::graphics::SetDefaultAnisotropy(anisotropy);

  PrecompileShaders();

  auto [texture, textureHdr] = [&] {
    auto hdrData = prime::common::LoadResource(std::string(argv[1]) + ".gth");
    auto texelData = prime::common::LoadResource(std::string(argv[1]) + ".gtb");
    auto &hdr = *hdrData.As<prime::graphics::Texture>();
    prime::utils::FixupClass(hdr);
    auto unit = prime::graphics::AddTexture(JenkinsHash3_("main_texture"), hdr,
                                            texelData.buffer.data());

    return std::make_pair(unit, hdr);
  }();

  prime::common::SetWorkingFolder(finf.GetFolder());

  {
    uint32 defaultSampler = prime::common::AddSimpleResource("res/default.spl");
    auto &res = prime::common::LoadResource(defaultSampler);
    auto *smpl = res.As<prime::graphics::Sampler>();
    prime::utils::FixupClass(*smpl);
    prime::graphics::AddSampler(defaultSampler, *smpl);
  };

  auto [lightTexture, lightTextureHdr] = [&] {
    uint32 hdrTex = prime::common::AddSimpleResource("res/light.gth");
    uint32 texels = prime::common::AddSimpleResource("res/light.gtb");

    auto hdrData = prime::common::LoadResource(hdrTex);
    auto texelData = prime::common::LoadResource(texels);
    auto &hdr = *hdrData.As<prime::graphics::Texture>();
    prime::utils::FixupClass(hdr);
    auto unit =
        prime::graphics::AddTexture(hdrData.hash, hdr, texelData.buffer.data());

    return std::make_pair(unit, hdr);
  }();

  using MainUBType = prime::shaders::single_texture::ubFragmentProperties;
  MainUBType *mainUBData = [&] {
    prime::common::ResourceData mainUniform;
    mainUniform.buffer.resize(sizeof(MainUBType));
    mainUniform.hash = JenkinsHash3_("main_uniform");
    prime::common::AddSimpleResource(std::move(mainUniform));
    auto &res = prime::common::LoadResource(mainUniform.hash);
    return res.As<MainUBType>();
  }();

  *mainUBData = {{0.7, 0.7, 0.7}, 1.5, 32.f};

  if (texture.flags == prime::graphics::TextureFlag::NormalMap) {
    mainUBData->ambientColor = {};
  }

  auto pipeline = [&, textureFlags = texture.flags] {
    auto mainPipeline = BuildPipeline(textureFlags);
    prime::common::AddSimpleResource(std::move(mainPipeline));
    auto &pipelineData = prime::common::LoadResource(mainPipeline.hash);
    auto pipeline = pipelineData.As<prime::graphics::Pipeline>();
    prime::utils::FixupClass(*pipeline);
    prime::graphics::AddPipeline(*pipeline);
    return pipeline;
  }();

  auto lightPipeline = [&] {
    uint32 resHash = prime::common::AddSimpleResource("res/light.ppe");
    auto &pipelineData = prime::common::LoadResource(resHash);
    auto pipeline = pipelineData.As<prime::graphics::Pipeline>();
    prime::utils::FixupClass(*pipeline);
    prime::graphics::AddPipeline(*pipeline);
    return pipeline;
  }();

  //BuildLightPipeline();

  auto infoText = [&, &textureHdr = textureHdr, &texture = texture] {
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

    using TexFlag = prime::graphics::TextureFlag;

    auto typeStr =
        texture.flags[TexFlag::Compressed]
            ? FindEnumName(CompressedFormats(textureHdr.internalFormat))
            : FindEnumName(InternalFormats(textureHdr.internalFormat));

    snprintf(
        infoBuffer, sizeof(infoBuffer),
        "Type: %s\nWidth: %u\nHeight: %u\nMaxLevel: %u\nActiveLevel: %%u\n",
        typeStr, textureHdr.width, textureHdr.height, textureHdr.maxLevel);
    std::string retval(infoBuffer);

    return retval;
  }();

  CubeObject cubeObj(texture.flags, *pipeline);

  uint32 cameraIndex =
      prime::common::AddCamera(JenkinsHash3_("default_camera"), {});
  auto &camera = prime::common::GetCamera(JenkinsHash3_("default_camera"));

  float scale = 1;
  auto UpdateProjection = [&] {
    camera.projection = glm::perspective(glm::radians(45.0f * scale),
                                         float(width) / height, 0.1f, 100.0f);
  };

  int maxLevel;
  glGetTextureParameteriv(texture.id, GL_TEXTURE_MAX_LEVEL, &maxLevel);
  int lastLevel = 0;

  glm::vec4 lightOrbit{1, 0, 0, 1.5};

  prime::graphics::FrameBuffer frameBuffer(width, height);

  constexpr float radius = 5;
  glm::vec3 lookAtDirection{1, 0, 0};
  glm::vec2 cameraYawPitch{};
  glm::vec2 lightYawPitch{};
  glm::vec3 cameraPosition{};

  BoxObject boxObj(*lightPipeline);

  auto UpdateCamera = [&] {
    auto cameraTM = glm::lookAt((lookAtDirection * radius), glm::vec3{},
                                glm::vec3{0, 1, 0});
    camera.transform = glm::dualquat(glm::quat(cameraTM), cameraTM[3]);
    camera.transform =
        glm::dualquat(glm::quat{1, 0, 0, 0}, cameraPosition) * camera.transform;
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
    cubeObj.lights.viewPos = glm::vec3{} * camera.transform;

    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer.bufferId);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    prime::common::SetCurrentCamera(cameraIndex);
    pipeline->BeginRender();
    cubeObj.Render();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    lightPipeline->BeginRender();
    boxObj.Render();
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // ImGui::ShowDemoWindow();

    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    if (ImGui::Begin("EditorDockSpace", nullptr, windowFlags)) {
      ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");
      ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f),
                       ImGuiDockNodeFlags_None);
      ImGui::PopStyleVar();
      if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
          if (ImGui::MenuItem("Open", "Ctrl+O")) {
            ImGuiFileDialog::Instance()->OpenDialog(
                "ChooseFileDlgKey", "Choose File", ".gth,.spl,.ppe", ".");
          }
          ImGui::EndMenu();
        }
      }
      ImGui::EndMenuBar();

      if (ImGuiFileDialog::Instance()->Display(
              "ChooseFileDlgKey",
              ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
          std::string filePathName =
              ImGuiFileDialog::Instance()->GetFilePathName();
          std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
        }
        ImGuiFileDialog::Instance()->Close();
      }

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    }
    ImGui::End();

    if (ImGui::Begin("View", nullptr)) {
      if (ImGui::BeginChild("RenderArea", ImVec2{}, false,
                            ImGuiWindowFlags_NoMove)) {
        ImVec2 region = ImGui::GetContentRegionAvail();

        if (width != region.x || height != region.y) {
          width = region.x;
          height = region.y;

          frameBuffer.Resize(width, height);
          glViewport(0, 0, width, height);
          UpdateProjection();
        }

        ImGui::Image((ImTextureID)frameBuffer.textureId,
                     ImGui::GetContentRegionAvail(), ImVec2(0, 1),
                     ImVec2(1, 0));

        if (ImGui::IsWindowHovered()) {
          glm::vec2 mouseDelta{io.MouseDelta.x, io.MouseDelta.y};
          mouseDelta = (mouseDelta / 512.f) * scale;

          if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            constexpr float clampPi = glm::half_pi<float>() - 0.001;
            const bool lightKey = glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS;
            if (lockedLight || lightKey) {
              if (lightKey) {
                lightYawPitch -= mouseDelta * 2.f;
              } else {
                lightYawPitch += mouseDelta * 2.f;
              }

              lightYawPitch.y = glm::clamp(lightYawPitch.y, -clampPi, clampPi);
              lightOrbit.x = (cos(lightYawPitch.x) * cos(lightYawPitch.y));
              lightOrbit.y = sin(lightYawPitch.y);
              lightOrbit.z = (sin(lightYawPitch.x) * cos(lightYawPitch.y));
            }

            if (!lightKey) {
              cameraYawPitch += mouseDelta * 2.f;
              cameraYawPitch.y =
                  glm::clamp(cameraYawPitch.y, -clampPi, clampPi);
              lookAtDirection.x =
                  (cos(cameraYawPitch.x) * cos(cameraYawPitch.y));
              lookAtDirection.y = sin(cameraYawPitch.y);
              lookAtDirection.z =
                  (sin(cameraYawPitch.x) * cos(cameraYawPitch.y));
              UpdateCamera();
            }
          }

          if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            cameraPosition += glm::vec3(mouseDelta.x, -mouseDelta.y, 0.f);
            UpdateCamera();
          }

          if (io.MouseWheel) {
            if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
              if (io.MouseWheel < 0) {
                lightOrbit.w -= 0.05f;
              } else {
                lightOrbit.w += 0.05f;
              }
            } else if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) ==
                       GLFW_PRESS) {
              lastLevel += io.MouseWheel < 0 ? -1 : 1;
              lastLevel = std::max(0, lastLevel);
              lastLevel = std::min(maxLevel, lastLevel);

              glBindTexture(textureHdr.target, texture.id);
              glTexParameteri(textureHdr.target, GL_TEXTURE_BASE_LEVEL,
                              lastLevel);
            } else {
              if (io.MouseWheel < 0) {
                scale *= 0.75f;
              } else {
                scale *= 1.25f;
              }

              scale = std::clamp(scale, 0.001f, 2.f);
              UpdateProjection();
            }
          }
        }
      }
      ImGui::EndChild();
    }
    ImGui::End();

    ImGui::PopStyleVar(3);

    if (ImGui::Begin("Scene Settings", nullptr,
                     ImGuiWindowFlags_AlwaysAutoResize)) {
      if (ImGui::CollapsingHeader("Ambient Color",
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::ColorPicker3("ambColor", &mainUBData->ambientColor.x,
                            ImGuiColorEditFlags_NoLabel |
                                ImGuiColorEditFlags_NoSmallPreview |
                                ImGuiColorEditFlags_NoSidePreview);
      }

      if (ImGui::SliderInt("Anisotropy", &anisotropy, 1, maxAniso)) {
        prime::graphics::SetDefaultAnisotropy(anisotropy);
      }
    }
    ImGui::End();

    if (ImGui::Begin("InfoText", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoBackground |
                         ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_Modal)) {
      // ImGui::SetWindowPos(ImVec2{0.f, 20.f});
      ImGui::Text(infoText.c_str(), lastLevel);
    }
    ImGui::End();

    if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      if (ImGui::CollapsingHeader("Light 0", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::ColorPicker3("color", &cubeObj.lightData.pointLight[0].color.x,
                            ImGuiColorEditFlags_NoLabel |
                                ImGuiColorEditFlags_NoSmallPreview |
                                ImGuiColorEditFlags_NoSidePreview);
        ImGui::SliderFloat("distance", &lightOrbit.w, 0, 30);
        ImGui::SliderFloat(
            "quadAtten", &cubeObj.lightData.pointLight[0].attenuation.y, 0, 1);
        ImGui::SliderFloat(
            "cubicAtten", &cubeObj.lightData.pointLight[0].attenuation.z, 0, 2);
      }

      if (ImGui::CollapsingHeader("Specular", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("specLevel", &mainUBData->specLevel, 0, 100);
        ImGui::SliderFloat("specPower", &mainUBData->specPower, 0, 256);
      }

      ImGui::Checkbox("lockedLight", &lockedLight);

      /*if (ImGui::CollapsingHeader("Debug", ImGuiTreeNodeFlags_DefaultOpen))
     { ImGui::Checkbox("Use TBN", &cubeObj.useTBN);
      }*/
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
