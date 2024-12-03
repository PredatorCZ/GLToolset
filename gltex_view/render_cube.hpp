#include "graphics/detail/model_single.hpp"
#include "scene_program.hpp"

struct CubeObject : MainShaderProgram {
  pg::ModelSingle *model = nullptr;

  static pg::ModelSingle *Model(bool normal) {
    static pg::ModelSingle *modelAlbedo = nullptr;
    static pg::ModelSingle *modelNormal = nullptr;

    pg::ModelSingle *&useModel = normal ? modelNormal : modelAlbedo;

    if (!useModel) {
      auto mdlHash = pc::MakeHash<pg::ModelSingle>(
          normal ? "editor/texture_cube_normal" : "editor/texture_cube_albedo");
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
                                       ? "editor/texture_cube_normal"
                                       : "editor/texture_cube_albedo"), 1);
  }

  void Render() {
    UseProgram();
    pg::Draw(*model);
  }
};
