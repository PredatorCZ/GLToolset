#include "model_single.fbs.hpp"
#include "scene_program.hpp"
#include "vertex.fbs.hpp"

const prime::graphics::VertexArray *GetCubeVertexArray() {
  static const prime::graphics::VertexArray *VT_ARRAY = nullptr;

  if (!VT_ARRAY) {
    flatbuffers::FlatBufferBuilder builder;
    prime::graphics::VertexArrayBuilder vtBuilder(builder);
    vtBuilder.add_count(36);
    vtBuilder.add_mode(GL_TRIANGLES);
    prime::utils::FinishFlatBuffer(vtBuilder);
    auto vayHash = pc::MakeHash<pg::VertexArray>("cube_vertex");

    pc::AddSimpleResource(pc::ResourceData{
        vayHash,
        {
            reinterpret_cast<const char *>(builder.GetBufferPointer()),
            builder.GetSize(),
        },
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
      flatbuffers::FlatBufferBuilder builder;

      auto stagesPtr = [&] {
        std::vector<pg::StageObject> objects;
        objects.emplace_back(JenkinsHash3_("basics/simple_cube.vert"), 0,
                             GL_VERTEX_SHADER);
        objects.emplace_back(
            normal ? JenkinsHash3_("single_texture/main_normal.frag")
                   : JenkinsHash3_("single_texture/main_albedo.frag"),
            0, GL_FRAGMENT_SHADER);
        return builder.CreateVectorOfStructs(objects);
      }();

      auto definesPtr = [&] {
        std::vector<flatbuffers::Offset<flatbuffers::String>> offsets;
        offsets.emplace_back(builder.CreateString("NUM_LIGHTS=1"));
        offsets.emplace_back(builder.CreateString("TS_TANGENT_ATTR"));
        offsets.emplace_back(builder.CreateString("TS_QUAT"));
        return builder.CreateVector(offsets);
      }();

      pg::ProgramBuilder pgmBuild(builder);
      pgmBuild.add_stages(stagesPtr);
      pgmBuild.add_program(-1);
      pgmBuild.add_definitions(definesPtr);
      auto pgmPtr = pgmBuild.Finish();
      auto vtxPtr = builder.CreateStruct(GetCubeVertexArray());

      auto texturesPtr = [&] {
        std::vector<pg::SampledTexture> textures;

        textures.emplace_back(0,
                              JenkinsHash_(normal ? "smTSNormal" : "smAlbedo"),
                              JenkinsHash3_("res/default"), 0, 0);

        return builder.CreateVectorOfStructs(textures);
      }();

      auto uniBlocksPtr = [&] {
        std::vector<pg::UniformBlock> ublocks;
        ublocks.emplace_back(
            pg::UniformBlock{pc::MakeHash<pg::UniformBlockData>("main_uniform"),
                             JenkinsHash_("ubFragmentProperties"), 0, 0, 0});

        return builder.CreateVectorOfStructs(ublocks);
      }();

      pg::ModelRuntime runtimeDummy{};
      pg::ModelSingleBuilder mdlBuild(builder);
      mdlBuild.add_program(pgmPtr);
      mdlBuild.add_runtime(&runtimeDummy);
      mdlBuild.add_vertexArray_type(pc::ResourceVar_ptr);
      mdlBuild.add_vertexArray(vtxPtr.Union());
      mdlBuild.add_textures(texturesPtr);
      mdlBuild.add_uniformBlocks(uniBlocksPtr);

      prime::utils::FinishFlatBuffer(mdlBuild);

      auto mdlHash = pc::MakeHash<pg::ModelSingle>(
          normal ? "texture_cube_normal" : "texture_cube_albedo");
      pc::AddSimpleResource(pc::ResourceData{
          mdlHash,
          {
              reinterpret_cast<const char *>(builder.GetBufferPointer()),
              builder.GetSize(),
          },
      });
      auto &mdlData = pc::LoadResource(mdlHash);
      useModel = static_cast<pg::ModelSingle *>(pc::GetResourceHandle(mdlData));
    }

    return useModel;
  }

  CubeObject(uint32 textureName, pg::TextureFlags flags)
      : MainShaderProgram(
            Model(flags == pg::TextureFlag::NormalMap)->mutable_program()),
        model(Model(flags == pg::TextureFlag::NormalMap)) {
    auto texPtr =
        const_cast<pg::SampledTexture *>((*model->mutable_textures())[0]);
    texPtr->mutate_texture(textureName);
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
