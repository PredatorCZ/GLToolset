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
  Transforms,
  count_,
};
}
