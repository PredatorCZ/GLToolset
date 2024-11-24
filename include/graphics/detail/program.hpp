#pragma once
#include "common/array.hpp"
#include "common/variant.hpp"
#include "common/string.hpp"
#include "graphics/program.hpp"

namespace prime::graphics {
struct StageObject;
struct ShaderProto;
struct ProtoFeature;
struct NuttyProgram;
struct LegacyProgram;
} // namespace prime::graphics

HASH_CLASS(prime::graphics::StageObject);
HASH_CLASS(prime::graphics::ShaderProto);
HASH_CLASS(prime::graphics::ProtoFeature);
HASH_CLASS(prime::graphics::NuttyProgram);
HASH_CLASS(prime::graphics::LegacyProgram);

namespace prime::graphics {
struct StageObject {
  JenHash3 resource;
  JenHash object;
  uint32 type;
};

struct LegacyProgram {
  common::LocalArray32<StageObject> stages;
  common::LocalArray32<common::String> definitions;
};

struct ProtoFeature {
  common::String name;
  common::Variant<uint, int, bool> value;
};

struct NuttyProgram {
  common::Pointer<ShaderProto> proto;
  common::LocalArray32<ProtoFeature> features;
};

struct Program {
  common::Variant<NuttyProgram, LegacyProgram> proto;
  uint32 program = -1;
};
} // namespace prime::graphics
