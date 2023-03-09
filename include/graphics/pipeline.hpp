#pragma once
#include "common/array.hpp"
#include "common/resource.hpp"
#include "vertex_type.hpp"

namespace prime::graphics {
struct UniformBlockData;
struct SampledTexture;
struct UniformBlock;
struct StageObject;
struct Pipeline;
} // namespace prime::graphics

CLASS_EXT(prime::graphics::UniformBlockData);
HASH_CLASS(prime::graphics::SampledTexture);
HASH_CLASS(prime::graphics::UniformBlock);
HASH_CLASS(prime::graphics::StageObject);
CLASS_EXT(prime::graphics::Pipeline);

namespace prime::graphics {
struct SampledTexture {
  uint32 texture;
  union {
    struct {
      uint16 location;
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
  common::ResourcePtr<UniformBlockData> data;
  uint32 bufferObject;
  uint32 location;
  uint32 dataSize;
};

struct BaseProgramLocations {
  union {
    uint8 attributesLocations[size_t(VertexType::count_)]{
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    };
    struct {
      uint8 inPos;
      uint8 inTangent;
      uint8 inNormal;
      uint8 inTexCoord2;
      uint8 inTexCoord40;
      uint8 inTexCoord41;
      uint8 inTexCoord42;
      uint8 inTransform;
    };
  };

  uint32 ubCamera;
  uint32 ubLights;
  uint32 ubFragmentProperties;
  uint32 ubLightData;
  uint32 ubInstanceTransforms;
};

struct Pipeline : common::Resource {
  Pipeline() : CLASS_VERSION(1) {}
  common::LocalArray32<StageObject> stageObjects;
  common::LocalArray32<SampledTexture> textureUnits;
  common::LocalArray32<UniformBlock> uniformBlocks;
  common::LocalArray32<char> definitions;
  BaseProgramLocations locations{};
  uint32 program;

  void BeginRender() const;
};
} // namespace prime::graphics
