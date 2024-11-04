#include "graphics/detail/model_single.hpp"
#include "graphics/detail/vertex_array.hpp"
#include "scene_program.hpp"
#include "utils/playground.hpp"

prime::graphics::VertexArray *GetCubeVertexArray() {
  static prime::graphics::VertexArray *VT_ARRAY = nullptr;

  if (!VT_ARRAY) {
    namespace pu = prime::utils;
    pu::PlayGround vayPg;
    pu::PlayGround::Pointer<prime::graphics::VertexArray> newVay =
        vayPg.AddClass<prime::graphics::VertexArray>();

    newVay->count = 36;
    newVay->mode = GL_TRIANGLES;

    auto vayHash = pc::MakeHash<pg::VertexArray>("cube_vertex");

    pc::AddSimpleResource(pc::ResourceData{
        vayHash,
        vayPg.Build(),
    });
    auto &data = pc::LoadResource(vayHash);
    VT_ARRAY = static_cast<prime::graphics::VertexArray *>(
        pc::GetResourceHandle(data));
  }

  return VT_ARRAY;
}

struct CubeObject : MainShaderProgram {
  pg::ModelSingle *model = nullptr;

  static pg::ModelSingle *Model(bool normal) {
    static pg::ModelSingle *modelAlbedo = nullptr;
    static pg::ModelSingle *modelNormal = nullptr;

    pg::ModelSingle *&useModel = normal ? modelNormal : modelAlbedo;

    if (!useModel) {
      pu::PlayGround modelPg;

      pu::PlayGround::Pointer<pg::ModelSingle> newModel =
          modelPg.AddClass<pg::ModelSingle>();

      newModel->vertexArray = GetCubeVertexArray();

      modelPg.ArrayEmplace(newModel->program.stages,
                           JenkinsHash3_("basics/simple_cube.vert"), 0,
                           GL_VERTEX_SHADER);
      modelPg.ArrayEmplace(
          newModel->program.stages,
          normal ? JenkinsHash3_("single_texture/main_normal.frag")
                 : JenkinsHash3_("single_texture/main_albedo.frag"),
          0, GL_FRAGMENT_SHADER);

      modelPg.NewString(*modelPg.ArrayEmplace(newModel->program.definitions),
                        "NUM_LIGHTS=1");
      modelPg.NewString(*modelPg.ArrayEmplace(newModel->program.definitions),
                        "TS_TAGENT_ATTR");
      modelPg.NewString(*modelPg.ArrayEmplace(newModel->program.definitions),
                        "TS_QUAT");

      modelPg.ArrayEmplace(newModel->textures, 0,
                           JenkinsHash_(normal ? "smTSNormal" : "smAlbedo"),
                           JenkinsHash3_("res/default"), 0, 0);

      modelPg.ArrayEmplace(newModel->uniformBlocks,
                           pc::MakeHash<pg::UniformBlockData>("main_uniform"),
                           JenkinsHash_("ubFragmentProperties"), 0, 0, 0);

      auto mdlHash = pc::MakeHash<pg::ModelSingle>(
          normal ? "texture_cube_normal" : "texture_cube_albedo");
      pc::AddSimpleResource(pc::ResourceData{
          mdlHash,
          modelPg.Build(),
      });
      auto &mdlData = pc::LoadResource(mdlHash);
      useModel = static_cast<pg::ModelSingle *>(pc::GetResourceHandle(mdlData));
    }

    return useModel;
  }

  CubeObject(uint32 textureName, pg::TextureFlags flags)
      : MainShaderProgram(&Model(flags == pg::TextureFlag::NormalMap)->program),
        model(Model(flags == pg::TextureFlag::NormalMap)) {
    auto &texPtr = model->textures[0];
    texPtr.texture = textureName;
    pg::RebuildProgram(*model, pc::MakeHash<pg::ModelSingle>(
                                   flags == pg::TextureFlag::NormalMap
                                       ? "texture_cube_normal"
                                       : "texture_cube_albedo"));
  }

  void Render() {
    UseProgram();
    pg::Draw(*model);
  }
};
