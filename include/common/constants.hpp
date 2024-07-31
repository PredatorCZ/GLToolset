#pragma once
#include <cstdint>
namespace prime::common {
struct BufferLimits {
  uint32_t computeBlocks;
  uint32_t vertexBlocks;
  uint32_t fragmentBlocks;
  uint32_t geometryBlocks;
  uint32_t tessControlBlocks;
  uint32_t tessEvalBlocks;
  uint32_t totalBlocks;
  uint32_t bindings;
  uint64_t size;
};

struct UniformLimits {
  uint64_t computeComponents;
  uint64_t fragmentComponents;
  uint64_t vertexComponents;
  uint64_t geometryComponents;
  uint64_t tessControlComponents;
  uint64_t tessEvalComponents;
};

struct StageLimits {
  uint32_t fragment;
  uint32_t vertex;
  uint32_t geometry;
  uint32_t tessControl;
  uint32_t tessEval;
};

struct Constants {
  UniformLimits uniform;
  UniformLimits uniformBuffer;
  BufferLimits ssbo;
  BufferLimits ubo;
  StageLimits outputs;
  StageLimits inputs;
  StageLimits textureUnits;
};

Constants GetConstants();
}; // namespace prime::common
