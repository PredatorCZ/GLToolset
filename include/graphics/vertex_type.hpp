#pragma once
#include "datas/internal/sc_architecture.hpp"

namespace prime::graphics {
enum class VertexType : uint8 {
  Position,
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

static constexpr TSType TSType_None = TSType::None;
} // namespace prime::graphics
