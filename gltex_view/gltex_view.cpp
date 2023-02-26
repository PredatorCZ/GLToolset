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

#include "render_box.hpp"
#include "render_cube.hpp"
#include "render_model.hpp"

#include "common/camera.hpp"
#include "common/resource.hpp"
#include "datas/binwritter.hpp"
#include "datas/master_printer.hpp"
#include "graphics/pipeline.hpp"
#include "graphics/sampler.hpp"
#include "utils/builder.hpp"
#include "utils/fixer.hpp"
#include "utils/resource_report.hpp"

#include "frame_buffer.hpp"

#include "simplecpp.h"

#include <optional>
#include <variant>

namespace pc = prime::common;
namespace pu = prime::utils;
namespace pg = prime::graphics;

#include "render_aabb.hpp"
#include "render_ts.hpp"

void PreloadResources() {
  pu::SetShadersSourceDir("/home/lukas/github/gltoolset/src/shaders/");
  pc::AddSimpleResource<char>("single_texture/main_normal.frag");
  pc::AddSimpleResource<char>("single_texture/main_albedo.frag");
  pc::AddSimpleResource<char>("basics/simple_sprite.vert");
  pc::AddSimpleResource<char>("basics/simple_cube.vert");
  pc::AddSimpleResource<char>("light/main.frag");

  pc::AddSimpleResource<char>("simple_model/main.vert");
  pc::AddSimpleResource<char>("simple_model/main.frag");

  pc::AddSimpleResource<char>("basics/ts_normal.vert");
  pc::AddSimpleResource<char>("basics/ts_normal.geom");
  pc::AddSimpleResource<char>("basics/ts_normal.frag");

  pc::AddSimpleResource<char>("basics/aabb_cube.frag");

  {
    pc::AddSimpleResource<pg::Texture>("res/light");
    pc::AddSimpleResource<pg::TextureStream<0>>("res/light");
  }

  {
    pc::ResourceHash defaultSampler =
        pc::AddSimpleResource<pg::Sampler>("res/default");
    auto &res = pc::LoadResource(defaultSampler);
    auto *smpl = res.As<pg::Sampler>();
    pg::AddSampler(defaultSampler.name, *smpl);
  }
}

pc::ResourceData BuildPipeline(pg::TextureFlags texFlags,
                               const std::string &path) {
  pu::Builder<pg::Pipeline> pipeline;
  pipeline.SetArray(pipeline.Data().stageObjects, 2);
  pu::DefineBuilder defBuilder;
  defBuilder.AddDefine("TS_QUAT");
  defBuilder.AddDefine("TS_TANGENT_ATTR");
  defBuilder.AddKeyValue("NUM_LIGHTS", 1);

  const bool isNormal = texFlags == pg::TextureFlag::NormalMap;

  if (isNormal) {
    if (texFlags == pg::TextureFlag::NormalDeriveZAxis) {
      defBuilder.AddDefine("TS_NORMAL_DERIVE_Z");
    }
    if (texFlags != pg::TextureFlag::SignedNormal) {
      defBuilder.AddDefine("TS_NORMAL_UNORM");
    }
  }

  pipeline.SetArray(pipeline.Data().definitions, defBuilder.buffer.size());
  memcpy(pipeline.Data().definitions.begin(), defBuilder.buffer.data(),
         defBuilder.buffer.size());

  auto &stageArray = pipeline.Data().stageObjects;

  auto &vertObj = stageArray[0];
  vertObj.stageType = GL_VERTEX_SHADER;
  vertObj.object = JenkinsHash3_("basics/simple_cube.vert");

  auto &fragObj = stageArray[1];
  fragObj.stageType = GL_FRAGMENT_SHADER;

  if (isNormal) {
    fragObj.object = JenkinsHash3_("single_texture/main_normal.frag");
  } else {
    fragObj.object = JenkinsHash3_("single_texture/main_albedo.frag");
  }

  pipeline.SetArray(pipeline.Data().textureUnits, 1);
  auto &textureArray = pipeline.Data().textureUnits;
  auto &texture = textureArray[0];
  texture.sampler = JenkinsHash3_("res/default");
  texture.slotHash = JenkinsHash_(isNormal ? "smTSNormal" : "smAlbedo");
  texture.texture = JenkinsHash3_(path);

  pipeline.SetArray(pipeline.Data().uniformBlocks, 1);
  auto &uniformBlockArray = pipeline.Data().uniformBlocks;
  auto &uniformBlock = uniformBlockArray[0];
  uniformBlock.data.resourceHash =
      pc::MakeHash<pg::UniformBlockData>("main_uniform");
  uniformBlock.bufferObject = JenkinsHash_("ubFragmentProperties");

  return {pc::MakeHash<pg::Pipeline>("main"), std::move(pipeline.buffer)};
}

pc::ResourceData BuildLightPipeline() {
  pu::Builder<pg::Pipeline> pipeline;
  pipeline.SetArray(pipeline.Data().stageObjects, 2);
  auto &stageArray = pipeline.Data().stageObjects;

  auto &vertObj = stageArray[0];
  vertObj.stageType = GL_VERTEX_SHADER;
  vertObj.object = JenkinsHash3_("basics/simple_sprite.vert");

  auto &fragObj = stageArray[1];
  fragObj.stageType = GL_FRAGMENT_SHADER;
  fragObj.object = JenkinsHash3_("light/main.frag");

  pipeline.SetArray(pipeline.Data().textureUnits, 1);
  auto &textureArray = pipeline.Data().textureUnits;
  auto &texture = textureArray[0];
  texture.sampler = JenkinsHash3_("res/default");
  texture.slotHash = JenkinsHash_("smTexture");
  texture.texture = JenkinsHash3_("res/light");

  return {pc::MakeHash<pg::Pipeline>("res/light"), std::move(pipeline.buffer)};
}

using MainUBType = prime::shaders::single_texture::ubFragmentProperties;

struct MainTexture {
  pg::TextureUnit texture;
  pg::Texture texHdr;
  pg::Pipeline *pipeline;
  CubeObject object;
  std::string infoText;
  int maxLevel{};
  int lastLevel{};

  void Render() { object.Render(); }

  void InfoText() { ImGui::Text(infoText.c_str(), lastLevel); }
};

MainTexture BuildFromTexture(MainUBType &ub, std::string path) {
  auto LoadTexture = [&path] {
    auto mainTexture = pc::AddSimpleResource<pg::Texture>(path);

    for (size_t s = 0; s < 4; s++) {
      auto tex = pg::RedirectTexture({}, s);
      pc::AddSimpleResource(path, tex.type);
    }

    auto hdrData = pc::LoadResource(mainTexture);
    auto &hdr = *hdrData.template As<pg::Texture>();
    auto unit = pg::LookupTexture(mainTexture.name);

    return std::make_pair(unit, hdr);
  };

  auto [texture, textureHdr] = LoadTexture();

  if (texture.flags == pg::TextureFlag::NormalMap) {
    ub.ambientColor = {};
  } else {
    ub.ambientColor = {0.7, 0.7, 0.7};
  }

  auto pipeline = [&, textureFlags = texture.flags] {
    auto mainPipeline = BuildPipeline(textureFlags, path);
    pc::AddSimpleResource(std::move(mainPipeline));
    auto &pipelineData = pc::LoadResource(mainPipeline.hash);
    auto pipeline = pipelineData.As<pg::Pipeline>();
    return pipeline;
  }();

  MainTexture retVal{
      .texture = texture,
      .texHdr = textureHdr,
      .pipeline = pipeline,
      .object = CubeObject(retVal.pipeline),
      .infoText =
          [&, &textureHdr = textureHdr, &texture = texture] {
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

            snprintf(infoBuffer, sizeof(infoBuffer),
                     "Type: %s\nWidth: %u\nHeight: %u\nMaxLevel: "
                     "%u\nActiveLevel: %%u\n",
                     typeStr, textureHdr.width, textureHdr.height,
                     textureHdr.maxLevel);
            std::string retval(infoBuffer);

            return retval;
          }(),
  };

  if (texture.flags[prime::graphics::TextureFlag::NormalMap]) {
    retVal.object.lightDataSpans.pointLights[0].color = glm::vec3{0.7f};
  }

  glGetTextureParameteriv(texture.id, GL_TEXTURE_MAX_LEVEL, &retVal.maxLevel);
  glGetTextureParameteriv(texture.id, GL_TEXTURE_BASE_LEVEL, &retVal.lastLevel);

  return retVal;
}

ModelObject BuildFromModel(std::string path) {
  BinReader refs(path + ".refs");
  char buffer[0x1000]{};
  while (!refs.BaseStream().getline(buffer, sizeof(buffer)).eof()) {
    AFileInfo inf(buffer);
    uint32 clHash = pc::GetClassFromExtension(
        {inf.GetExtension().data() + 1, inf.GetExtension().size()});
    pc::AddSimpleResource(std::string(inf.GetFullPathNoExt()), clHash);
  }

  auto mainModel = pc::AddSimpleResource<pg::VertexArray>(path);
  auto &hdrData = pc::LoadResource(mainModel);
  auto hdr = hdrData.template As<pg::VertexArray>();
  hdrData.numRefs++;
  hdr->SetupTransform(true);
  return ModelObject(hdr);
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

  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  GLFWwindow *sharedWindow = glfwCreateWindow(1, 1, "", nullptr, window);

  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_CONTEXT_RELEASE_BEHAVIOR, GLFW_RELEASE_BEHAVIOR_FLUSH);

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
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init();

  int maxAniso;
  int anisotropy = 8;
  glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
  pg::SetDefaultAnisotropy(anisotropy);

  pg::MinimumStreamIndexForDeferredLoading(-1);

  AFileInfo finf(argv[0]);
  pc::SetWorkingFolder(std::string(finf.GetFolder()));
  PreloadResources();

  MainUBType *mainUBData = [&] {
    pc::ResourceData mainUniform;
    mainUniform.buffer.resize(sizeof(MainUBType));
    mainUniform.hash = pc::MakeHash<pg::UniformBlockData>("main_uniform");
    pc::AddSimpleResource(std::move(mainUniform));
    auto &res = pc::LoadResource(mainUniform.hash);
    return res.As<MainUBType>();
  }();

  *mainUBData = {{0.7, 0.7, 0.7}, 1.5, 32.f};

  auto lightPipeline = [&] {
    pc::ResourceData pipelineData_ = BuildLightPipeline();
    pc::AddSimpleResource(std::move(pipelineData_));
    auto &pipelineData = pc::LoadResource(pipelineData_.hash);
    auto pipeline = pipelineData.As<pg::Pipeline>();
    pipelineData.numRefs++;
    return pipeline;
  }();

  uint32 cameraIndex = pc::AddCamera(JenkinsHash3_("default_camera"), {});
  auto &camera = pc::GetCamera(JenkinsHash3_("default_camera"));

  float scale = 1;
  auto UpdateProjection = [&] {
    camera.projection = glm::perspective(glm::radians(45.0f * scale),
                                         float(width) / height, 0.1f, 1000.0f);
  };

  glm::vec4 lightOrbit{1, 0, 0, 1.5};

  pg::FrameBuffer frameBuffer(width, height);

  constexpr float radius = 500;
  glm::vec3 lookAtDirection{0, 0, 1};
  glm::vec2 cameraYawPitch{glm::half_pi<float>(), 0};
  glm::vec2 lightYawPitch{};
  glm::vec3 cameraPosition{};

  BoxObject boxObj(*lightPipeline);

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

  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, 0);

  using MainObject = std::variant<ModelObject, MainTexture, std::monostate>;
  MainObject mainObject(std::monostate{});
  MainShaderProgram *mainProgram = nullptr;
  bool isModel = false;
  std::optional<NormalVisualizer> visModel;
  std::optional<AABBObject> visAABB;
  bool showTS = false;
  bool showAABB = false;
  float lightScale = 0.1;

  auto LoadAsset = [&](std::string path) {
    AFileInfo assInf(path);
    visModel.reset();
    visAABB.reset();

    uint32 classId = pc::GetClassFromExtension(assInf.GetExtension().substr(1));

    if (classId == pc::GetClassHash<pg::VertexArray>()) {
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
        visModel.emplace(modelObj.vtArray);
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

  LoadAsset(argv[1]);

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
    mainProgram->lightSpans.pointLightTMs[0] = lightOrbit * lightOrbit.w;
    boxObj.localPos =
        glm::vec4(glm::vec3{lightOrbit * lightOrbit.w}, lightScale);
    boxObj.lightColor = mainProgram->lightDataSpans.pointLights[0].color;
    *mainProgram->lightSpans.viewPos =
        glm::vec<3, float, glm::highp>{} * camera.transform;

    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer.bufferId);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
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

    glEnable(GL_BLEND);

    if (showAABB && visAABB) {
      glDisable(GL_DEPTH_TEST);
      visAABB->Render();
      glEnable(GL_DEPTH_TEST);
    }

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
                "ChooseFileDlgKey", "Choose File", ".gtte,.gvay", argv[1]);
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
          LoadAsset(ImGuiFileDialog::Instance()->GetFilePathName());
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

              glBindTexture(texObj.texHdr.target, texObj.texture.id);
              glTexParameteri(texObj.texHdr.target, GL_TEXTURE_BASE_LEVEL,
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
                    ImGuiWindowFlags_NoSavedSettings)) {
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
          visModel.emplace(std::get<ModelObject>(mainObject).vtArray);
        }
      }

      if (showTS) {
        ImGui::SliderFloat("Normal Size", &visModel->magnitude, 0, 300);
      }
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

          if (numRefs < 0) {
            ImGui::TextUnformatted("N/A");
          } else {
            ImGui::Text("%u", numRefs);
          }

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
