#include "common/camera.hpp"
#include "graphics/sampler.hpp"
#include "graphics/texture.hpp"
#include "spike/master_printer.hpp"
#include "utils/flatbuffers.hpp"
#include "utils/instancetm_builder.hpp"
#include "vertex.fbs.hpp"
#include <GL/glew.h>

#include "model_single.fbs.hpp"

namespace prime::graphics {
static void AddModelSingle(ModelSingle &hdr, common::ResourceHash referee) {
  const VertexArray *verts = nullptr;

  if (hdr.vertexArray_type() == common::ResourceVar_hash) {
    reinterpret_cast<flatbuffers::Table &>(hdr).SetField(
        hdr.VT_VERTEXARRAY_TYPE, common::ResourceVar_ptr,
        common::ResourceVar_NONE);
    verts = common::LinkResource<VertexArray>(
        *static_cast<common::ResourceHash *>(hdr.mutable_vertexArray()));
  } else {
    verts = *static_cast<VertexArray *const *>(hdr.mutable_vertexArray());
  }

  RebuildProgram(hdr, referee);
  auto runtime = hdr.mutable_runtime();

  uint32 transformBuffer;
  glGenBuffers(1, &transformBuffer);
  glBindBuffer(GL_UNIFORM_BUFFER, transformBuffer);
  runtime->mutate_transformBuffer(transformBuffer);
  runtime->mutate_numUVTransforms(
      verts->uvTransforms() ? verts->uvTransforms()->size() : 0);
  runtime->mutate_numTransforms(
      verts->transforms() ? verts->transforms()->size() : 1);

  utils::InstanceTransformsBuilder tmBuilder(runtime->numTransforms(),
                                             runtime->numUVTransforms());
  size_t i = 0;
  if (auto tms = verts->transforms()) {
    for (auto t : *tms) {
      tmBuilder.transforms[i++] = *t;
    }
  }

  if (auto uvTms = verts->uvTransforms()) {
    i = 0;
    for (auto t : *uvTms) {
      tmBuilder.uvTransform[i++] = *t;
    }
  }

  glBufferData(GL_UNIFORM_BUFFER, tmBuilder.buffer.size(),
               tmBuilder.buffer.data(), GL_STATIC_DRAW);

  if (auto uniforms = hdr.uniformBlocks()) {
    for (auto u_ : *uniforms) {
      auto u = const_cast<UniformBlock *>(u_);
      auto uData = reinterpret_cast<char *>(
          common::LinkResource<UniformBlockData>(u->mutable_data()));

      if (u->dataSize() == 0) {
        u->mutate_dataSize(common::FindResource(uData).buffer.size());
      }

      uint32 bufferIndex;
      glGenBuffers(1, &bufferIndex);
      glBindBuffer(GL_UNIFORM_BUFFER, bufferIndex);
      glBufferData(GL_UNIFORM_BUFFER, u->dataSize(), uData, GL_DYNAMIC_DRAW);
      u->mutate_bufferIndex(bufferIndex);
    }
  }
}

void Draw(const ModelSingle &model) {
  glUseProgram(model.program()->program());

  uint32 curTexture = 0;
  if (auto textures = model.textures()) {
    for (auto t : *textures) {
      glActiveTexture(GL_TEXTURE0 + curTexture);
      glBindTexture(t->target(), t->texture());
      glBindSampler(curTexture, t->sampler());
      glUniform1i(t->location(), curTexture++);
    }
  }

  glBindBufferBase(GL_UNIFORM_BUFFER, 0, common::GetUBCamera());
  glBindBufferBase(GL_UNIFORM_BUFFER, model.runtime()->transformIndex(),
                   model.runtime()->transformBuffer());

  if (auto uniforms = model.uniformBlocks()) {
    for (auto u : *uniforms) {
      common::ResourcePtr<char> uData{u->data()};
      glBindBufferBase(GL_UNIFORM_BUFFER, u->bindIndex(), u->bufferIndex());
      glBindBuffer(GL_UNIFORM_BUFFER, u->bufferIndex());
      glBufferSubData(GL_UNIFORM_BUFFER, 0, u->dataSize(), uData.resourcePtr);
    }
  }

  if (auto uniforms = model.uniformValues()) {
    for (auto u : *uniforms) {
      using UniformSetter = void (*)(const UniformValue &);

      static const UniformSetter uniformSetters[]{
          [](const UniformValue &u) {
            glUniform1f(u.location(), (*u.data())[0]);
          },
          [](const UniformValue &u) {
            glUniform2f(u.location(), (*u.data())[0], (*u.data())[1]);
          },
          [](const UniformValue &u) {
            glUniform3f(u.location(), (*u.data())[0], (*u.data())[1],
                        (*u.data())[2]);
          },
          [](const UniformValue &u) {
            glUniform4f(u.location(), (*u.data())[0], (*u.data())[1],
                        (*u.data())[2], (*u.data())[3]);
          },
      };

      uniformSetters[u->size()](*u);
    }
  }

  auto verts = *static_cast<VertexArray *const *>(model.vertexArray());

  if (uint16 vtype = verts->type()) [[likely]] {
    glBindVertexArray(verts->index());
    glDrawElements(verts->mode(), verts->count(), vtype, nullptr);
  } else {
    glDrawArrays(verts->mode(), 0, verts->count());
  }
}

void UpdateTransform(ModelSingle &model, const glm::dualquat *tm,
                     const glm::vec3 *scale) {
  auto runtime = model.runtime();
  char *mappedBuffer = static_cast<char *>(
      glMapNamedBuffer(runtime->transformBuffer(), GL_READ_WRITE));
  prime::utils::InstanceTransformSpanner spanner(
      mappedBuffer, runtime->numTransforms(), runtime->numUVTransforms());

  static const prime::common::Transform tmDef = {
      {{1, 0, 0, 0}, {0, 0, 0}},
      {1, 1, 1},
  };

  if (!tm) {
    tm = &tmDef.tm;
  }

  if (!scale) {
    scale = &tmDef.inflate;
  }

  size_t i = 0;
  VertexArray *const *vertsPtr =
      static_cast<VertexArray *const *>(model.vertexArray());
  VertexArray *verts = *vertsPtr;
  if (auto tms = verts->transforms()) {
    for (auto t : *tms) {
      auto &curtm = spanner.transforms[i++];
      curtm.tm = *tm * t->tm;
      curtm.inflate = *scale * t->inflate;
    }
  } else {
    for (auto &t : spanner.transforms) {
      t.tm = *tm;
      t.inflate = *scale;
    }
  }

  glUnmapNamedBuffer(runtime->transformBuffer());
}

void RebuildProgram(ModelSingle &model, common::ResourceHash referee) {
  bool normalUFlag = false;
  bool normalZFlag = false;
  bool hasNormal = false;

  if (auto textures = model.textures()) {
    for (auto t : *textures) {
      if (t->texture() == 0) {
        continue;
      }

      auto unit = LookupTexture(t->texture());

      if (unit.flags == TextureFlag::NormalMap) {
        bool normalUFlag_ = unit.flags != TextureFlag::SignedNormal;
        bool normalZFlag_ = unit.flags == TextureFlag::NormalDeriveZAxis;

        if (hasNormal) {
          if (normalUFlag_ != normalUFlag) {
            printwarning("Secondary normal map has signed flag mismatch");
          }

          if (normalZFlag_ != normalZFlag) {
            printwarning("Secondary normal map has derive z flag mismatch");
          }
        } else {
          normalUFlag = normalUFlag_;
          normalZFlag = normalZFlag_;
        }

        hasNormal = true;
      }
    }
  }

  std::vector<std::string> secondaryDefs;

  if (normalUFlag) {
    secondaryDefs.emplace_back("TS_NORMAL_UNORM");
  }

  if (normalZFlag) {
    secondaryDefs.emplace_back("TS_NORMAL_DERIVE_Z");
  }

  VertexArray *const *vertsPtr =
      static_cast<VertexArray *const *>(model.vertexArray());
  VertexArray *verts = *vertsPtr;

  if (auto uvtms = verts->uvTransforms(); uvtms && uvtms->size() > 0) {
    secondaryDefs.emplace_back("VS_NUMUVTMS=" + std::to_string(uvtms->size()));
  }

  if (auto uvremaps = verts->uvTransformRemaps()) {
    if (size_t numUVs = uvremaps->size()) {
      std::string remapsDef("VS_UVREMAPS=");

      for (auto r : *uvremaps) {
        remapsDef.append(std::to_string(r)).push_back(',');
      }

      remapsDef.pop_back();
      secondaryDefs.emplace_back(std::move(remapsDef));

      if (numUVs % 2) {
        secondaryDefs.emplace_back("VS_NUMUVS2=1");
      } else {
        secondaryDefs.emplace_back("VS_NUMUVS2=0");
      }

      if (numUVs > 1) {
        secondaryDefs.emplace_back("VS_NUMUVS4=" + std::to_string(numUVs / 2));
      }
    }
  }

  switch (verts->tsType()) {
  case TSType::Matrix:
    secondaryDefs.emplace_back("TS_TANGENT_ATTR");
    [[fallthrough]];
  case TSType::Normal:
    secondaryDefs.emplace_back("TS_NORMAL_ATTR");
    break;
  case TSType::QTangent:
    secondaryDefs.emplace_back("TS_TANGENT_ATTR");
    secondaryDefs.emplace_back("TS_QUAT");
    break;
  default:
    break;
  }

  auto &intro =
      CreateProgram(*model.mutable_program(), referee, &secondaryDefs);
  auto runtime = model.mutable_runtime();
  runtime->mutate_transformIndex(
      intro.uniformBlockBinds.at("ubInstanceTransforms"));

  if (auto textures = model.textures()) {
    for (auto t_ : *textures) {
      if (!intro.textureLocations.contains(JenHash(t_->slot()))) {
        continue;
      }

      auto t = const_cast<SampledTexture *>(t_);

      if (t->sampler() > 0xffff) {
        t->mutate_sampler(LookupSampler(t->sampler()));
      }

      if (t->texture() > 0xffffff) {
        auto unit = LookupTexture(t->texture());
        t->mutate_texture(unit.id);
        t->mutate_target(unit.target);
      }

      t->mutate_location(intro.textureLocations.at(JenHash(t->slot())));
    }
  }

  if (auto uniforms = model.uniformBlocks()) {
    for (auto u_ : *uniforms) {
      auto u = const_cast<UniformBlock *>(u_);
      if (intro.uniformBlockBinds.contains(JenHash(u->bufferSlot()))) {
        uint32 bindIndex = intro.uniformBlockBinds.at(JenHash(u->bufferSlot()));
        u->mutate_bindIndex(bindIndex);
      } else {
        printerror("Cannot find unform block bind " << std::hex
                                                    << u->bufferSlot());
      }
    }
  }

  if (auto uniforms = model.uniformValues()) {
    for (auto u_ : *uniforms) {
      auto u = const_cast<UniformValue *>(u_);
      auto iu = intro.uniformLocations.at(JenHash(u->name()));

      u->mutate_location(iu.location);
      u->mutate_size(iu.size);
    }
  }
}

} // namespace prime::graphics

template <> class prime::common::InvokeGuard<prime::graphics::ModelSingle> {
  static inline const bool data =
      common::AddResourceHandle<graphics::ModelSingle>({
          .Process =
              [](ResourceData &data) {
                auto hdr = utils::GetFlatbuffer<graphics::ModelSingle>(data);
                graphics::AddModelSingle(*hdr, data.hash);
              },
          .Delete = nullptr,
          .Handle = [](ResourceData &data) -> void * {
            return utils::GetFlatbuffer<graphics::ModelSingle>(data);
          },
          .Update =
              [](ResourceHash hash) {
                if (hash.type !=
                    common::GetClassHash<graphics::ModelSingle>()) {
                  return;
                }

                graphics::RebuildProgram(
                    *utils::GetFlatbuffer<graphics::ModelSingle>(
                        common::LoadResource(hash)),
                    hash);
              },
      });
};

REGISTER_CLASS(prime::graphics::ModelSingle);
