#pragma once
#include "common/resource.hpp"
#include <map>
#include <vector>

namespace prime::graphics {
struct UniformBlockData;
struct Program;

struct ProgramIntrospection {
  struct Uniform {
    uint16 location;
    uint16 size;
  };
  uint32 program;
  std::map<JenHash, uint32> uniformBlockBinds;
  std::map<JenHash, Uniform> uniformLocations;
  std::map<JenHash, uint32> textureLocations;
  std::map<JenHash, uint32> storageBufferLocations;
};

const ProgramIntrospection &
CreateProgram(Program &pgm, common::ResourceHash referee,
              std::vector<std::string> *secondaryDefs = nullptr);
const ProgramIntrospection &ProgramIntrospect(uint32 program);
} // namespace prime::graphics

CLASS_EXT(prime::graphics::UniformBlockData);
HASH_CLASS(prime::graphics::Program);
