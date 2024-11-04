#pragma once
#include "common/array.hpp"
#include "common/transform.hpp"
#include "graphics/detail/program.hpp"
#include "graphics/model_single.hpp"
#include "graphics/vertex.hpp"

namespace prime::graphics {
struct SampledTexture;
struct UniformBlock;
struct UniformValue;
struct Texture;
struct Sampler;
} // namespace prime::graphics

HASH_CLASS(prime::graphics::SampledTexture);
HASH_CLASS(prime::graphics::UniformBlock);
HASH_CLASS(prime::graphics::UniformValue);

namespace prime::graphics {
struct SampledTexture {
  common::ResourceToHandle<Texture> texture;
  JenHash slot;
  common::ResourceToHandle<Sampler> sampler;
  uint16 target = 0;
  uint16 location = 0;
};

struct UniformBlock {
  common::Pointer<UniformBlockData> data;
  JenHash bufferSlot;
  uint32 bufferIndex = 0;
  uint32 bindIndex = 0;
  uint32 dataSize = 0;
};

struct UniformValue {
  uint8 size;
  uint16 location;
  JenHash name;
  float data[4];
};

struct ModelSingle : common::Resource<ModelSingle> {
  Program program;
  common::Pointer<VertexArray> vertexArray;
  common::LocalArray32<SampledTexture> textures;
  common::LocalArray32<UniformBlock> uniformBlocks;
  common::LocalArray32<UniformValue> uniformValues;
  uint16 transformIndex;
  uint32 transformBuffer;
};

} // namespace prime::graphics
