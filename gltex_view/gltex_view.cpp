#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include "ImGuiFileDialog.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "spike/io/binreader.hpp"
#include "spike/io/binwritter.hpp"
#include "spike/io/fileinfo.hpp"
#include "spike/master_printer.hpp"
#include "spike/type/flags.hpp"
#include <optional>
#include <variant>

#include "common/camera.hpp"
#include "common/constants.hpp"
#include "common/resource.hpp"
#include "graphics/frame_buffer.hpp"
#include "graphics/post_process.hpp"
#include "graphics/program.hpp"
#include "graphics/sampler.hpp"
#include "script/scriptapi.hpp"
#include "tex_format_ref.hpp"
#include "utils/instancetm_builder.hpp"
#include "utils/resource_report.hpp"
#include "utils/shader_preprocessor.hpp"
#include "utils/texture.hpp"

namespace pc = prime::common;
namespace pu = prime::utils;
namespace pg = prime::graphics;

#include "render_aabb.hpp"
#include "render_box.hpp"
#include "render_model.hpp"
#include "render_ts.hpp"

using MainUBType = prime::shaders::single_texture::ubFragmentProperties;

struct MainTexture {
  pg::TextureUnit texture;
  CubeObject object;
  std::string infoText;
  int maxLevel{};
  int lastLevel{};

  void Render() { object.Render(); }

  void InfoText() { ImGui::Text(infoText.c_str(), lastLevel); }
};

MainTexture BuildFromTexture(MainUBType &ub, std::string path) {
  auto LoadTexture = [&path] {
    auto hash = pc::MakeHash<pg::Texture>(path);
    pc::LoadResource(hash);
    return hash.name;
  };

  auto textureName = LoadTexture();
  auto texture = pg::LookupTexture(textureName);

  if (texture.flags == pg::TextureFlag::NormalMap) {
    ub.ambientColor = {};
  } else {
    ub.ambientColor = {0.7, 0.7, 0.7};
  }

  MainTexture retVal{
      .texture = texture,
      .object = CubeObject(textureName, texture.flags),
      .infoText =
          [&] {
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

            int internalFormat;
            int width;
            int height;
            int maxLevel;

            glGetTextureLevelParameteriv(
                texture.id, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
            glGetTextureLevelParameteriv(texture.id, 0, GL_TEXTURE_WIDTH,
                                         &width);
            glGetTextureLevelParameteriv(texture.id, 0, GL_TEXTURE_HEIGHT,
                                         &height);
            glGetTextureParameteriv(texture.id, GL_TEXTURE_MAX_LEVEL,
                                    &maxLevel);

            auto typeStr = texture.flags == TexFlag::Compressed
                               ? FindEnumName(CompressedFormats(internalFormat))
                               : FindEnumName(InternalFormats(internalFormat));

            snprintf(infoBuffer, sizeof(infoBuffer),
                     "Type: %s\nWidth: %u\nHeight: %u\nMaxLevel: "
                     "%u\nActiveLevel: %%u\n",
                     typeStr, width, height, maxLevel);
            std::string retval(infoBuffer);

            return retval;
          }(),
  };

  if (texture.flags == pg::TextureFlag::NormalMap) {
    retVal.object.lightDataSpans.pointLights[0].color = glm::vec3{0.7f};
  }

  glGetTextureParameteriv(texture.id, GL_TEXTURE_MAX_LEVEL, &retVal.maxLevel);
  glGetTextureParameteriv(texture.id, GL_TEXTURE_BASE_LEVEL, &retVal.lastLevel);

  return retVal;
}

ModelObject BuildFromModel(std::string path) {
  auto hash = pc::MakeHash<pg::ModelSingle>(path);
  auto &hdrData = pc::LoadResource(hash);
  auto hdr = pc::GetResourceHandle(hdrData);
  hdrData.numRefs++;
  auto mdl = static_cast<pg::ModelSingle *>(hdr);
  prime::graphics::RebuildProgram(*mdl, hdrData.hash, 1);
  return ModelObject(mdl);
}

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id,
                                GLenum severity, GLsizei length,
                                const GLchar *message, const void *userParam) {
  const char *sevName = " unknown";
  switch (severity) {
  case GL_DEBUG_SEVERITY_HIGH:
    sevName = " high";
    break;
  case GL_DEBUG_SEVERITY_MEDIUM:
    sevName = " medium";
    break;
  case GL_DEBUG_SEVERITY_LOW:
    sevName = " low";
    break;
  }

  if (type == GL_DEBUG_TYPE_ERROR) {
    printerror(sevName << " severity: " << message);
  } else if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
    printinfo(message);
  } else {
    printwarning(sevName << " severity: " << message);
  }
}

int main(int, char *argv[]) {
  es::print::AddPrinterFunction(es::Print);

  glfwSetErrorCallback(
      [](int type, const char *msg) { printerror('(' << type << ')' << msg); });

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  int width = 1800;
  int height = 1020;

  GLFWwindow *window =
      glfwCreateWindow(width, height, "GLTex Viewer", nullptr, nullptr);

  if (!window) {
    glfwTerminate();
    return 1;
  }

  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  GLFWwindow *sharedWindow = glfwCreateWindow(1, 1, "", nullptr, window);

  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_CONTEXT_RELEASE_BEHAVIOR, GLFW_RELEASE_BEHAVIOR_FLUSH);

  glfwMakeContextCurrent(window);

  GLenum err = glewInit();

  if (GLEW_OK != err) {
    glfwTerminate();
    return 2;
  }

  pc::Constants constants = pc::GetConstants();

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init();

  int maxAniso;
  int anisotropy = 8;
  glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
  pg::SetDefaultAnisotropy(anisotropy);

  pg::MinimumStreamIndexForDeferredLoading(-1);
  pg::ClampTextureResolution(-1);

  pc::AddWorkingFolder("/home/lukas/github/gltoolset/src/shaders/");
  pc::AddWorkingFolder("/home/lukas/github/gltoolset/gltex_view/res/");
  pc::ProjectDataFolder(argv[1]);
  prime::script::AutogenerateScriptClasses();

  MainUBType *mainUBData = [&] {
    pc::ResourceData mainUniform;
    mainUniform.buffer.resize(sizeof(MainUBType));
    mainUniform.hash = pc::MakeHash<pg::UniformBlockData>("main_uniform");
    pc::AddSimpleResource(std::move(mainUniform));
    auto &res = pc::LoadResource(mainUniform.hash);
    return res.As<MainUBType>().retVal;
  }();

  *mainUBData = {{0.7, 0.7, 0.7}, 1.5, 32.f, 1.f, 0.3f};

  uint32 cameraIndex = pc::AddCamera(JenkinsHash3_("default_camera"), {});
  auto &camera = pc::GetCamera(JenkinsHash3_("default_camera"));

  float scale = 1;
  auto UpdateProjection = [&] {
    camera.projection = glm::perspective(glm::radians(45.0f * scale),
                                         float(width) / height, 0.1f, 1000.0f);
  };

  glm::vec4 lightOrbit{1, 0, 0, 1.5};

  pg::FrameBuffer canvas(pg::CreateFrameBuffer(width, height));
  canvas.AddTexture(pg::FrameBufferTextureType::Color);
  canvas.AddTexture(pg::FrameBufferTextureType::Glow);
  canvas.Finalize();

  pg::PostProcess ppGlowH(pg::CreatePostProcess(width, height));
  ppGlowH.stages.emplace_back(
      pg::AddPostProcessStage("post_process/glow_horizontal", canvas));

  pg::PostProcess ppGlowV(pg::CreatePostProcess(width, height));
  {
    pg::PostProcessStage stage(
        pg::AddPostProcessStage("post_process/glow_vertical", canvas));
    stage.textures.front().id = ppGlowH.texture;
    ppGlowV.stages.emplace_back(stage);
  }

  pg::PostProcess *ppGlow[]{
      &ppGlowH,
      &ppGlowV,
  };

  pg::PostProcess ppCombine(pg::CreatePostProcess(width, height));
  {
    pg::PostProcessStage stage(
        pg::AddPostProcessStage("post_process/combine", canvas));
    auto &bgclu = stage.uniforms.at("BGColor");
    bgclu.data[0] = 0.2;
    bgclu.data[1] = 0.3;
    bgclu.data[2] = 0.3;
    stage.textures.back().id = ppGlowV.texture;
    ppCombine.stages.emplace_back(stage);
  }

  constexpr float radius = 500;
  glm::vec3 lookAtDirection{0, 0, 1};
  glm::vec2 cameraYawPitch{glm::half_pi<float>(), 0};
  glm::vec2 lightYawPitch{};
  glm::vec3 cameraPosition{};

  BoxObject boxObj;

  auto UpdateCamera = [&] {
    auto cameraTM = glm::lookAt((lookAtDirection * radius), glm::vec3{},
                                glm::vec3{0, 1, 0});
    camera.transform =
        glm::dualquat(glm::quat(cameraTM), glm::vec3{cameraTM[3]});
    camera.transform =
        glm::dualquat(glm::quat{1, 0, 0, 0}, cameraPosition) * camera.transform;
  };

  UpdateCamera();
  UpdateProjection();

  bool lockedLight = false;

  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, 0);

  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  using MainObject = std::variant<ModelObject, MainTexture, std::monostate>;
  MainObject mainObject(std::monostate{});
  MainShaderProgram *mainProgram = nullptr;
  bool isModel = false;
  std::optional<NormalVisualizer> visModel;
  std::optional<AABBObject> visAABB;
  bool showTS = false;
  bool showAABB = false;
  float lightScale = 0.1;
  int renderBuffer = canvas.outTextures.size();

  auto LoadAsset = [&](std::string path) {
    AFileInfo assInf(path);
    visModel.reset();
    visAABB.reset();

    std::string_view ext = assInf.GetExtension().substr(1);

    uint32 classId = pc::GetClassFromExtension(ext).retVal;

    if (classId == pc::GetClassHash<pg::ModelSingle>() || ext == "md2") {
      mainObject = BuildFromModel(std::string(assInf.GetFullPathNoExt()));
      auto &modelObj = std::get<ModelObject>(mainObject);
      lightOrbit.w = modelObj.vtArray->aabb.bounds.w;
      lightScale = lightOrbit.w / 10;
      mainProgram = &modelObj;
      isModel = true;

      if (showAABB) {
        visAABB.emplace(modelObj.vtArray);
      }

      if (showTS) {
        visModel.emplace(modelObj.mdl);
      }

    } else if (classId == pc::GetClassHash<pg::Texture>()) {
      mainObject =
          BuildFromTexture(*mainUBData, std::string(assInf.GetFullPathNoExt()));
      mainProgram = &std::get<MainTexture>(mainObject).object;
      lightOrbit.w = 1.5;
      isModel = false;
      lightScale = 0.1;
    } else {
      throw std::runtime_error("Invalid asset type");
    }
  };

  LoadAsset(argv[2]);

  /*std::thread streamer([&, &texture = texture] {
    glfwMakeContextCurrent(sharedWindow);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    pg::StreamTextures(0);
    glfwSwapBuffers(sharedWindow);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    glGetTextureParameteriv(texture.id, GL_TEXTURE_BASE_LEVEL, &lastLevel);
    glGetTextureParameteriv(texture.id, GL_TEXTURE_MAX_LEVEL, &maxLevel);
  });*/

  while (!glfwWindowShouldClose(window)) {
    pc::PollUpdates();
    mainProgram->lightDataSpans.pointLights[0].position =
        glm::vec3{lightOrbit * lightOrbit.w};
    *boxObj.localPos =
        glm::vec4(glm::vec3{lightOrbit * lightOrbit.w}, lightScale);
    *boxObj.lightColor = mainProgram->lightDataSpans.pointLights[0].color;
    *mainProgram->lightDataSpans.viewPos =
        glm::vec<3, float, glm::highp>{} * camera.transform;

    glBindFramebuffer(GL_FRAMEBUFFER, canvas.bufferId);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    pc::SetCurrentCamera(cameraIndex);

    std::visit(
        [](auto &i) {
          if constexpr (!std::is_same_v<std::decay_t<decltype(i)>,
                                        std::monostate>) {
            i.Render();
          }
        },
        mainObject);

    if (showTS && visModel) {
      visModel->Render();
    }

    if (showAABB && visAABB) {
      glDisable(GL_DEPTH_TEST);
      glEnable(GL_BLEND);
      visAABB->Render();
      glEnable(GL_DEPTH_TEST);
      glDisable(GL_BLEND);
    }

    glEnable(GL_BLEND);
    boxObj.Render();
    glDisable(GL_BLEND);

    glDisable(GL_DEPTH_TEST);

    for (size_t i = 0; i < 10; i++) {
      auto &pp = *ppGlow[i & 1];
      glBindFramebuffer(GL_FRAMEBUFFER, pp.bufferId);

      auto &stage = pp.stages.front();

      stage.RenderToResult();

      if (i == 0) {
        stage.textures.front().id = ppGlow[1]->texture;
      }
    }

    ppGlow[0]->stages.front().textures.front().id =
        canvas.FindTexture(pg::FrameBufferTextureType::Glow);

    glBindFramebuffer(GL_FRAMEBUFFER, ppCombine.bufferId);
    ppCombine.stages.front().RenderToResult();

    glEnable(GL_DEPTH_TEST);

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
                "ChooseFileDlgKey", "Choose File", ".md2", argv[1]);
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
          if (filePathName.starts_with(argv[1])) {
            LoadAsset(filePathName.substr(strlen(argv[1])));
          }
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

          canvas.Resize(width, height);
          ppGlowH.Resize(width, height);
          ppGlowV.Resize(width, height);
          ppCombine.Resize(width, height);
          glViewport(0, 0, width, height);
          UpdateProjection();
        }

        uint32 texId = renderBuffer == canvas.outTextures.size()
                           ? ppCombine.texture
                           : canvas.outTextures.at(renderBuffer).id;

        ImGui::Image((ImTextureID)texId, ImGui::GetContentRegionAvail(),
                     ImVec2(0, 1), ImVec2(1, 0));

        if (ImGui::IsWindowHovered()) {
          glm::vec2 mouseDelta{io.MouseDelta.x, io.MouseDelta.y};
          mouseDelta = mouseDelta / 512.f;

          if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            const float clampPi = glm::half_pi<float>() - 0.001;
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
            } else if (!isModel && glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) ==
                                       GLFW_PRESS) {
              auto &texObj = std::get<MainTexture>(mainObject);

              texObj.lastLevel += io.MouseWheel < 0 ? -1 : 1;
              texObj.lastLevel = std::max(0, texObj.lastLevel);
              texObj.lastLevel = std::min(texObj.maxLevel, texObj.lastLevel);

              glBindTexture(texObj.texture.target, texObj.texture.id);
              glTexParameteri(texObj.texture.target, GL_TEXTURE_BASE_LEVEL,
                              texObj.lastLevel);
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
        if (ImGui::Begin(
                "InfoText", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs |
                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground |
                    ImGuiWindowFlags_NoSavedSettings |
                    ImGuiWindowFlags_AlwaysAutoResize)) {
          std::visit(
              [](auto &i) {
                if constexpr (!std::is_same_v<std::decay_t<decltype(i)>,
                                              std::monostate>) {
                  i.InfoText();
                }
              },
              mainObject);
        }
        ImGui::End();
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

      if (isModel && ImGui::Checkbox("Show Bounds", &showAABB)) {
        if (showAABB && !visAABB) {
          visAABB.emplace(std::get<ModelObject>(mainObject).vtArray);
        }
      }

      if (isModel && ImGui::Checkbox("Show Tangent Space", &showTS)) {
        if (showTS && !visModel) {
          visModel.emplace(std::get<ModelObject>(mainObject).mdl);
        }
      }

      if (showTS) {
        ImGui::SliderFloat("Normal Size", visModel->magnitude, 0, 300);
      }

      ImGui::SliderFloat("Glow Level", &mainUBData->glowLevel, 0, 10);
      ImGui::SliderFloat("Alpha Cutoff", &mainUBData->alphaCutOff, 0, 1);
      ImGui::SliderFloat("Hue Rotation", &mainUBData->hueRotation, 0, 1);
      ImGui::SliderInt("Render Buffer", &renderBuffer, 0,
                       canvas.outTextures.size());
    }
    ImGui::End();

    if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      if (ImGui::CollapsingHeader("Light 0", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::ColorPicker3(
            "color", &mainProgram->lightDataSpans.pointLights[0].color.x,
            ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoSmallPreview |
                ImGuiColorEditFlags_NoSidePreview);
        ImGui::SliderFloat("distance", &lightOrbit.w, 0, 300);
        ImGui::SliderFloat(
            "quadAtten",
            &mainProgram->lightDataSpans.pointLights[0].attenuation.y, 0, 1);
        ImGui::SliderFloat(
            "cubicAtten",
            &mainProgram->lightDataSpans.pointLights[0].attenuation.z, 0, 2);
      }

      if (ImGui::CollapsingHeader("Specular", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("specLevel", &mainUBData->specLevel, 0, 100);
        ImGui::SliderFloat("specPower", &mainUBData->specPower, 0, 256);
      }

      ImGui::Checkbox("lockedLight", &lockedLight);
    }
    ImGui::End();

    if (ImGui::Begin("Resources", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      if (ImGui::BeginTable("ResourceTable", 6, ImGuiTableFlags_BordersInner)) {
        // ImGui::TableSetupScrollFreeze(3, 1);

        ImGui::TableSetupColumn("Class", ImGuiTableColumnFlags_WidthStretch,
                                20);
        ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch, 65);
        ImGui::TableSetupColumn("Num refs", ImGuiTableColumnFlags_WidthStretch,
                                5);
        ImGui::TableSetupColumn("Hash", ImGuiTableColumnFlags_WidthStretch, 5);
        ImGui::TableSetupColumn("Data size", ImGuiTableColumnFlags_WidthStretch,
                                5);
        ImGui::TableHeadersRow();

        pc::IterateResources([](std::string_view path,
                                std::string_view className, uint32 nameHash,
                                int32 numRefs, size_t dataSize) {
          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::TextUnformatted(className.begin(), className.end());
          ImGui::TableNextColumn();
          ImGui::TextUnformatted(path.begin(), path.end());
          ImGui::TableNextColumn();
          ImGui::Text("%i", numRefs);
          ImGui::TableNextColumn();
          ImGui::Text("%.8X", nameHash);
          ImGui::TableNextColumn();
          ImGui::Text("%zu", dataSize);
          ImGui::TableNextColumn();
        });

        ImGui::EndTable();
      }
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  /*if (streamer.joinable()) {
    streamer.join();
  }*/

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwDestroyWindow(sharedWindow);
  glfwTerminate();
  return 0;
}
