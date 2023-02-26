#pragma once
#include "common/aabb.hpp"
#include "common/array.hpp"
#include "common/resource.hpp"
#include "vertex_type.hpp"

namespace prime::graphics {
struct VertexIndexData;
struct VertexVshData;
struct VertexPshData;
struct VertexAttribute;
struct VertexBuffer;
struct VertexArray;
struct Pipeline;
} // namespace prime::graphics

HASH_CLASS(prime::graphics::VertexAttribute);
HASH_CLASS(prime::graphics::VertexBuffer);
CLASS_EXT(prime::graphics::VertexArray);
CLASS_EXT(prime::graphics::VertexIndexData);
CLASS_EXT(prime::graphics::VertexVshData);
CLASS_EXT(prime::graphics::VertexPshData);

namespace prime::graphics {
struct VertexAttribute {
  VertexType slot;
  uint8 size;
  uint16 type;
  bool normalized;
  uint8 offset;
};

struct VertexBuffer {
  common::LocalArray32<VertexAttribute> attributes;
  common::ResourceHash buffer;
  uint32 size;
  uint16 target;
  uint16 usage;
  uint8 stride;
};

struct VertexArray : common::Resource {
  VertexArray() : CLASS_VERSION(1) {}
  common::LocalArray32<char> transformData;
  common::LocalArray32<VertexBuffer> buffers;
  uint32 index;
  common::ResourcePtr<Pipeline> pipeline;
  common::AABB aabb;
  uint16 maxNumUVs;
  uint16 maxNumIndexedTransforms;
  uint16 mode;
  uint16 type;
  uint16 count;

  void BeginRender();
  void EndRender();
  void SetupTransform(bool staticTransform);
};
} // namespace prime::graphics
