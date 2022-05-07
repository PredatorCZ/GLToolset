#pragma once
#include "common/array.hpp"

namespace prime::graphics {
struct SampledTexture {
  uint32 texture;
  union {
    struct {
      uint16 slot;
      uint16 target;
    };
    uint32 slotHash;
  };
  uint32 sampler;
};

struct StageObject {
  uint32 object;
  uint16 stageType;
};

struct UniformBlock {
  uint64 dataObject;
  uint32 bufferObject;
  uint32 dataSize;
};
} // namespace prime::graphics

HASH_CLASS(prime::graphics::SampledTexture);
HASH_CLASS(prime::graphics::StageObject);
HASH_CLASS(prime::graphics::UniformBlock);

namespace prime::graphics {
enum class VertexType {
  Position,
  Tangent,
  QTangent,
  Normal,
  TexCoord20,
  TexCoord21,
  TexCoord22,
  TexCoord23,
  TexCoord40,
  TexCoord41,
  TexCoord42,
  TexCoord43,
  Transforms,
  count_,
};

struct BaseProgramLocations {
  union {
    uint8 attributesLocations[size_t(VertexType::count_)]{};
    struct {
      uint8 inPos;
      uint8 inTangent;
      uint8 inQTangent;
      uint8 inNormal;
      uint8 inTexCoord20;
      uint8 inTexCoord21;
      uint8 inTexCoord22;
      uint8 inTexCoord23;
      uint8 inTexCoord40;
      uint8 inTexCoord41;
      uint8 inTexCoord42;
      uint8 inTexCoord43;
      uint8 inTransform;
    };
  };

  uint32 ubCamera;
  uint32 ubLights;
  uint32 ubFragmentProperties;
  uint32 ubLightData;
  uint32 ubInstanceTransforms;
};

struct Pipeline {
  common::Array<StageObject> stageObjects;
  common::Array<SampledTexture> textureUnits;
  common::Array<UniformBlock> uniformBlocks;
  BaseProgramLocations locations{};
  uint32 program;

  void BeginRender() const;
};

void AddPipeline(Pipeline &pipeline);
} // namespace prime::graphics
