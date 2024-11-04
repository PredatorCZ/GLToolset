#pragma once
#include "common/array.hpp"
#include "graphics/program.hpp"

namespace prime::graphics {
struct StageObject;
} // namespace prime::graphics

HASH_CLASS(prime::graphics::StageObject);

namespace prime::graphics {
struct StageObject {
  JenHash3 resource;
  JenHash object;
  uint32 type;
};

struct Program {
  common::LocalArray32<StageObject> stages;
  common::LocalArray32<common::LocalArray16<char>> definitions;
  uint32 program = -1;
};
} // namespace prime::graphics
