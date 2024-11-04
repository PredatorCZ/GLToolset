#pragma once
#include "common/array.hpp"
#include "graphics/texture.hpp"

namespace prime::graphics {
struct TextureEntry;
} // namespace prime::graphics

HASH_CLASS(prime::graphics::TextureEntry);

namespace prime::graphics {
struct TextureEntry {
  uint8 level;
  uint8 streamIndex;
  uint16 target;
  uint32 bufferSize;
  uint64 bufferOffset;
};

struct Texture : common::Resource<Texture> {
  common::LocalArray16<TextureEntry> entries;
  common::LocalArray16<int32> swizzle;
  uint16 width;
  uint16 height;
  uint16 depth;
  uint16 internalFormat;
  uint16 format;
  uint16 type;
  uint16 target;
  uint8 maxLevel;
  uint8 numDims;
  uint8 reserved;
  uint8 numStreams;
  TextureFlags flags;
};
} // namespace prime::graphics
