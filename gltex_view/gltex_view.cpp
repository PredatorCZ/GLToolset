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

namespace pc = prime::common;
namespace pu = prime::utils;
namespace pg = prime::graphics;

void PrecompileShaders() {
  pu::SetShadersSourceDir("/home/lukas/github/gltoolset/src/shaders/");

  simplecpp::DUI noDui{};
  auto vertexShader = pu::PreProcess("light/main.vert", noDui);
  pc::AddSimpleResource(
      {pc::MakeHash<char>("light/main.vert"), std::move(vertexShader)});
  auto fragmentShader = pu::PreProcess("light/main.frag", noDui);
  pc::AddSimpleResource(
      {pc::MakeHash<char>("light/main.frag"), std::move(fragmentShader)});

  pu::FragmentShaderFeatures fragFeats{};
  pc::AddSimpleResource(
      pu::PreprocessShaderSource(fragFeats, "single_texture/main_normal.frag"));
  fragFeats.deriveZNormal = false;
  pc::AddSimpleResource(
      pu::PreprocessShaderSource(fragFeats, "single_texture/main_normal.frag"));
  fragFeats.signedNormal = false;
  pc::AddSimpleResource(
      pu::PreprocessShaderSource(fragFeats, "single_texture/main_normal.frag"));
  pc::AddSimpleResource(
      pu::PreprocessShaderSource(fragFeats, "single_texture/main_albedo.frag"));
  fragFeats.deriveZNormal = true;
  pc::AddSimpleResource(
      pu::PreprocessShaderSource(fragFeats, "single_texture/main_normal.frag"));

  pu::VertexShaderFeatures vertFeats{};
  vertFeats.tangentSpace = pu::VSTSFeat::Quat;
  pc::AddSimpleResource(
      pu::PreprocessShaderSource(vertFeats, "single_texture/main.vert"));
}

pc::ResourceData BuildPipeline(pg::TextureFlags texFlags,
                               const std::string &path) {
  pu::Builder<pg::Pipeline> pipeline;
  pipeline.SetArray(pipeline.Data().stageObjects, 2);
  auto stageArray = pipeline.ArrayData(pipeline.Data().stageObjects);

  auto &vertObj = stageArray[0];
  vertObj.stageType = GL_VERTEX_SHADER;
  pu::VertexShaderFeatures vertFeats{};
  vertFeats.tangentSpace = pu::VSTSFeat::Quat;

  vertObj.object = JenkinsHash3_("single_texture/main.vert", vertFeats.Seed());

  pu::FragmentShaderFeatures fragFeats{};
  fragFeats.signedNormal = texFlags == pg::TextureFlag::SignedNormal;
  fragFeats.deriveZNormal = texFlags == pg::TextureFlag::NormalDeriveZAxis;

  auto &fragObj = stageArray[1];
  fragObj.stageType = GL_FRAGMENT_SHADER;

  const bool isNormal = texFlags == pg::TextureFlag::NormalMap;

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
  texture.sampler = JenkinsHash3_("res/default");
  texture.slotHash = JenkinsHash_(isNormal ? "smTSNormal" : "smAlbedo");
  texture.texture = JenkinsHash3_(path);

  pipeline.SetArray(pipeline.Data().uniformBlocks, 1);
  auto uniformBlockArray = pipeline.ArrayData(pipeline.Data().uniformBlocks);
  auto &uniformBlock = uniformBlockArray[0];
  uniformBlock.dataObject =
      pc::MakeHash<pg::UniformBlockData>("main_uniform").hash;
  uniformBlock.bufferObject = JenkinsHash_("ubFragmentProperties");

  return {pc::MakeHash<pg::Pipeline>("main"), std::move(pipeline.buffer)};
}

void BuildLightPipeline() {
  pu::Builder<pg::Pipeline> pipeline;
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
  texture.sampler = JenkinsHash3_("res/default");
  texture.slotHash = JenkinsHash_("smTexture");
  texture.texture = JenkinsHash3_("res/light");

  BinWritter wr(pu::ShadersSourceDir() + "../../gltex_view/res/light.ppe");
  wr.WriteContainer(pipeline.buffer);
}

int main(int, char *argv[]) {
  es::print::AddPrinterFunction(es::Print);

  glfwSetErrorCallback(
      [](int type, const char *msg) { printerror('(' << type << ')' << msg); });

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
  pg::SetDefaultAnisotropy(anisotropy);

  pg::MinimumStreamIndexForDeferredLoading(-1);

  PrecompileShaders();

  std::string argv1(argv[1]);

  auto [texture, textureHdr] = [&] {
    auto mainTexture = pc::AddSimpleResource<pg::Texture>(argv1);
    auto hdrData = pc::LoadResource(mainTexture);
    auto &hdr = *hdrData.As<pg::Texture>();
    pu::FixupClass(hdr);

    for (size_t s = 0; s < hdr.numStreams; s++) {
      auto tex = pg::RedirectTexture({}, s);
      pc::AddSimpleResource(argv1, tex.type);
    }

    auto unit =
        pg::AddTexture(std::make_shared<pc::ResourceData>(std::move(hdrData)));

    return std::make_pair(unit, hdr);
  }();

  AFileInfo finf(argv[0]);
  pc::SetWorkingFolder(finf.GetFolder());

  {
    pc::ResourceHash defaultSampler =
        pc::AddSimpleResource<pg::Sampler>("res/default");
    auto &res = pc::LoadResource(defaultSampler);
    auto *smpl = res.As<pg::Sampler>();
    pu::FixupClass(*smpl);
    pg::AddSampler(defaultSampler.name, *smpl);
  };

  auto [lightTexture, lightTextureHdr] = [&] {
    pc::ResourceHash hdrTex = pc::AddSimpleResource<pg::Texture>("res/light");
    pc::AddSimpleResource<pg::TextureStream<0>>("res/light");
    auto hdrData = pc::LoadResource(hdrTex);
    auto &hdr = *hdrData.As<pg::Texture>();
    pu::FixupClass(hdr);
    auto unit =
        pg::AddTexture(std::make_shared<pc::ResourceData>(std::move(hdrData)));

    return std::make_pair(unit, hdr);
  }();

  using MainUBType = prime::shaders::single_texture::ubFragmentProperties;
  MainUBType *mainUBData = [&] {
    pc::ResourceData mainUniform;
    mainUniform.buffer.resize(sizeof(MainUBType));
    mainUniform.hash = pc::MakeHash<pg::UniformBlockData>("main_uniform");
    pc::AddSimpleResource(std::move(mainUniform));
    auto &res = pc::LoadResource(mainUniform.hash);
    return res.As<MainUBType>();
  }();

  *mainUBData = {{0.7, 0.7, 0.7}, 1.5, 32.f};

  if (texture.flags == pg::TextureFlag::NormalMap) {
    mainUBData->ambientColor = {};
  }

  auto pipeline = [&, textureFlags = texture.flags] {
    auto mainPipeline = BuildPipeline(textureFlags, argv1);
    pc::AddSimpleResource(std::move(mainPipeline));
    auto &pipelineData = pc::LoadResource(mainPipeline.hash);
    auto pipeline = pipelineData.As<pg::Pipeline>();
    pu::FixupClass(*pipeline);
    pg::AddPipeline(*pipeline);
    return pipeline;
  }();

  // BuildLightPipeline();

  auto lightPipeline = [&] {
    pc::ResourceHash resHash = pc::AddSimpleResource<pg::Pipeline>("res/light");
    auto &pipelineData = pc::LoadResource(resHash);
    auto pipeline = pipelineData.As<pg::Pipeline>();
    pu::FixupClass(*pipeline);
    pg::AddPipeline(*pipeline);
    return pipeline;
  }();

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

    using TexFlag = pg::TextureFlag;

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

  uint32 cameraIndex = pc::AddCamera(JenkinsHash3_("default_camera"), {});
  auto &camera = pc::GetCamera(JenkinsHash3_("default_camera"));

  float scale = 1;
  auto UpdateProjection = [&] {
    camera.projection = glm::perspective(glm::radians(45.0f * scale),
                                         float(width) / height, 0.1f, 100.0f);
  };

  int maxLevel;
  glGetTextureParameteriv(texture.id, GL_TEXTURE_MAX_LEVEL, &maxLevel);
  int lastLevel;
  glGetTextureParameteriv(texture.id, GL_TEXTURE_BASE_LEVEL, &lastLevel);

  glm::vec4 lightOrbit{1, 0, 0, 1.5};

  pg::FrameBuffer frameBuffer(width, height);

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

  // std::thread streamer([&]{glfwMakeContextCurrent(window);
  // pg::StreamTextures(0);});

  while (!glfwWindowShouldClose(window)) {
    cubeObj.lights.lightPos[0] = lightOrbit * lightOrbit.w;
    boxObj.localPos = cubeObj.lights.lightPos[0];
    cubeObj.lights.viewPos = glm::vec3{} * camera.transform;

    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer.bufferId);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    pc::SetCurrentCamera(cameraIndex);
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
        pg::SetDefaultAnisotropy(anisotropy);
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
