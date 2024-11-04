#pragma once
#include "common/aabb.hpp"
#include "common/array.hpp"
#include "common/resource.hpp"
#include "common/transform.hpp"
#include "graphics/vertex.hpp"

namespace prime::graphics {
struct VertexAttribute;
struct VertexBuffer;
} // namespace prime::graphics

HASH_CLASS(prime::graphics::VertexAttribute);
HASH_CLASS(prime::graphics::VertexBuffer);

namespace prime::graphics {
enum class VertexType : uint8 {
  None = 0xff,
  Position = 0,
  Tangent,
  Normal,
  TexCoord2,
  TexCoord40,
  TexCoord41,
  TexCoord42,
};

enum class TSType : uint8 {
  None,
  Normal,
  Matrix,
  QTangent,
};

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

struct VertexArray : common::Resource<VertexArray> {
  common::LocalArray32<VertexBuffer> buffers;
  common::LocalArray32<common::Transform> transforms;
  common::LocalArray32<glm::vec4> uvTransform;
  common::LocalArray32<uint8> uvTransforRemaps;
  common::AABB aabb;
  uint32 index;
  uint32 count;
  uint16 mode;
  uint16 type;
  TSType tsType;
};
} // namespace prime::graphics

HASH_CLASS(prime::graphics::VertexType);
HASH_CLASS(prime::graphics::TSType);
