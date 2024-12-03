#include "graphics/detail/model_single.hpp"
#include "common/camera.hpp"
#include "graphics/detail/vertex_array.hpp"
#include "graphics/sampler.hpp"
#include "graphics/texture.hpp"
#include "spike/master_printer.hpp"
#include "utils/instancetm_builder.hpp"
#include <GL/glew.h>

namespace prime::graphics {
static void AddModelSingle(ModelSingle &hdr, common::ResourceHash referee) {
  const VertexArray *verts = common::LinkResource<VertexArray>(hdr.vertexArray);

  RebuildProgram(hdr, referee, 0);

  glGenBuffers(1, &hdr.transformBuffer);
  glBindBuffer(GL_UNIFORM_BUFFER, hdr.transformBuffer);

  utils::InstanceTransformsBuilder tmBuilder(verts->transforms.numItems,
                                             verts->uvTransform.numItems);
  size_t i = 0;

  for (auto &t : verts->transforms) {
    tmBuilder.transforms[i++] = t;
  }

  i = 0;
  for (auto &t : verts->uvTransform) {
    tmBuilder.uvTransform[i++] = t;
  }

  glBufferData(GL_UNIFORM_BUFFER, tmBuilder.buffer.size(),
               tmBuilder.buffer.data(), GL_STATIC_DRAW);

  for (auto &u : hdr.uniformBlocks) {
    auto uData = reinterpret_cast<char *>(common::LinkResource(u.data));

    if (u.dataSize == 0) {
      u.dataSize = common::FindResource(uData).buffer.size();
    }

    glGenBuffers(1, &u.bufferIndex);
    glBindBuffer(GL_UNIFORM_BUFFER, u.bufferIndex);
    glBufferData(GL_UNIFORM_BUFFER, u.dataSize, uData, GL_DYNAMIC_DRAW);
  }
}

void Draw(const ModelSingle &model) {
  glUseProgram(model.program.program);

  uint32 curTexture = 0;
  for (auto &t : model.textures) {
    glActiveTexture(GL_TEXTURE0 + curTexture);
    glBindTexture(t.target, t.texture);
    glBindSampler(curTexture, t.sampler);
    glUniform1i(t.location, curTexture++);
  }

  glBindBufferBase(GL_UNIFORM_BUFFER, 0, common::GetUBCamera());
  glBindBufferBase(GL_UNIFORM_BUFFER, model.transformIndex,
                   model.transformBuffer);

  for (auto &u : model.uniformBlocks) {
    glBindBufferBase(GL_UNIFORM_BUFFER, u.bindIndex, u.bufferIndex);
    glBindBuffer(GL_UNIFORM_BUFFER, u.bufferIndex);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, u.dataSize, u.data.operator->());
  }

  for (auto u : model.uniformValues) {
    using UniformSetter = void (*)(const UniformValue &);

    static const UniformSetter uniformSetters[]{
        [](const UniformValue &u) { glUniform1f(u.location, u.data[0]); },
        [](const UniformValue &u) {
          glUniform2f(u.location, u.data[0], u.data[1]);
        },
        [](const UniformValue &u) {
          glUniform3f(u.location, u.data[0], u.data[1], u.data[2]);
        },
        [](const UniformValue &u) {
          glUniform4f(u.location, u.data[0], u.data[1], u.data[2], u.data[3]);
        },
    };

    uniformSetters[u.size](u);
  }

  const VertexArray &verts = *model.vertexArray;

  if (uint16 vtype = verts.type) [[likely]] {
    glBindVertexArray(verts.index);
    glDrawElements(verts.mode, verts.count, vtype, nullptr);
  } else {
    glDrawArrays(verts.mode, 0, verts.count);
  }
}

void UpdateTransform(ModelSingle &model, const glm::dualquat *tm,
                     const glm::vec3 *scale) {
  char *mappedBuffer = static_cast<char *>(
      glMapNamedBuffer(model.transformBuffer, GL_READ_WRITE));
  prime::utils::InstanceTransformSpanner spanner(
      mappedBuffer, model.vertexArray->transforms.numItems,
      model.vertexArray->uvTransform.numItems);

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
  const VertexArray &verts = *model.vertexArray;

  if (verts.transforms.numItems) {
    for (auto &t : verts.transforms) {
      auto &curtm = spanner.transforms[i++];
      curtm.tm = *tm * t.tm;
      curtm.inflate = *scale * t.inflate;
    }
  } else {
    for (auto &t : spanner.transforms) {
      t.tm = *tm;
      t.inflate = *scale;
    }
  }

  glUnmapNamedBuffer(model.transformBuffer);
}

void RebuildProgram(ModelSingle &model, common::ResourceHash referee,
                    uint32 numLights) {
  bool normalUFlag = false;
  bool normalZFlag = false;
  bool hasNormal = false;

  for (auto &t : model.textures) {
    if (t.texture.raw() == 0) {
      continue;
    }

    auto unit = LookupTexture(t.texture);

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

  std::vector<std::string> secondaryDefs;

  if (numLights > 0) {
    secondaryDefs.emplace_back("NUM_LIGHTS=" + std::to_string(numLights));
  }

  if (normalUFlag) {
    secondaryDefs.emplace_back("TS_NORMAL_UNORM=true");
  } else {
    secondaryDefs.emplace_back("TS_NORMAL_UNORM=false");
  }

  if (normalZFlag) {
    secondaryDefs.emplace_back("TS_NORMAL_DERIVE_Z=true");
  } else {
    secondaryDefs.emplace_back("TS_NORMAL_DERIVE_Z=false");
  }

  const VertexArray &verts = *model.vertexArray;

  if (verts.uvTransform.numItems) {
    secondaryDefs.emplace_back("VS_NUMUVTMS=" +
                               std::to_string(verts.uvTransform.numItems));
  }

  if (size_t numUVs = verts.uvTransforRemaps.numItems) {
    std::string remapsDef("VS_UVREMAPS=");

    for (auto r : verts.uvTransforRemaps) {
      remapsDef.append(std::to_string(r)).push_back(',');
    }

    remapsDef.pop_back();
    secondaryDefs.emplace_back(std::move(remapsDef));
    secondaryDefs.emplace_back("VS_NUMUVS=" + std::to_string(numUVs));
  } else {
    secondaryDefs.emplace_back("VS_NUMUVS=1");
    secondaryDefs.emplace_back("VS_UVREMAPS");
  }

  switch (verts.tsType) {
  case TSType::Matrix:
    secondaryDefs.emplace_back("TS_TYPE=2");
    break;
  case TSType::QTangent:
    secondaryDefs.emplace_back("TS_TYPE=1");
    break;
  default:
    secondaryDefs.emplace_back("TS_TYPE=0");
    break;
  }

  auto &intro = CreateProgram(model.program, referee, &secondaryDefs);
  model.transformIndex = intro.uniformBlockBinds.at("ubInstanceTransforms");

  for (auto &t : model.textures) {
    if (!intro.textureLocations.contains(JenHash(t.slot))) {
      continue;
    }

    if (t.sampler) {
      t.sampler = LookupSampler(t.sampler);
    }

    if (t.texture) {
      auto unit = LookupTexture(t.texture);
      t.texture = unit.id;
      t.target = unit.target;
    }

    t.location = intro.textureLocations.at(JenHash(t.slot));
  }

  for (auto &u : model.uniformBlocks) {
    if (intro.uniformBlockBinds.contains(JenHash(u.bufferSlot))) {
      uint32 bindIndex = intro.uniformBlockBinds.at(JenHash(u.bufferSlot));
      u.bindIndex = bindIndex;
    } else {
      printerror("Cannot find unform block bind " << std::hex
                                                  << u.bufferSlot.raw());
    }
  }

  for (auto &u : model.uniformValues) {
    auto iu = intro.uniformLocations.at(JenHash(u.name));

    u.location = iu.location;
    u.size = iu.size;
  }
}

} // namespace prime::graphics

template <> class prime::common::InvokeGuard<prime::graphics::ModelSingle> {
  static inline const bool data =
      common::AddResourceHandle<graphics::ModelSingle>({
          .Process =
              [](ResourceData &data) {
                auto hdr = data.As<graphics::ModelSingle>();
                graphics::AddModelSingle(*hdr, data.hash);
              },
          .Delete = nullptr,
          .Update =
              [](ResourceHash hash) {
                if (hash.type !=
                    common::GetClassHash<graphics::ModelSingle>()) {
                  return;
                }

                graphics::RebuildProgram(
                    *common::LoadResource(hash).As<graphics::ModelSingle>(),
                    hash, 0);
              },
      });
};

REGISTER_CLASS(prime::graphics::ModelSingle);
