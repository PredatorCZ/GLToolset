#pragma once
#include "common/array.hpp"
#include "datas/flags.hpp"

namespace prime::graphics {
enum TextureFlag : uint16 {
  Cubemap,
  Array,
  Volume,
  Compressed,
  AlphaMasked,
  NormalMap,
  NormalDeriveZAxis,
  SignedNormal,
  Swizzle,
};

struct TextureEntry {
  uint16 target;
  uint8 level;
  uint8 reserved;
  uint32 bufferSize;
  uint64 bufferOffset;
};
} // namespace prime::graphics

HASH_CLASS(prime::graphics::TextureEntry);

namespace prime::graphics {
using TextureFlags = es::Flags<TextureFlag>;

struct Texture {
  common::Array<TextureEntry> entries;
  uint16 width;
  uint16 height;
  uint16 depth;
  uint16 internalFormat;
  uint16 format;
  uint16 type;
  uint16 target;
  uint8 maxLevel;
  uint8 numDims;
  TextureFlags flags;
  int32 swizzleMask[4];
};

struct TextureUnit {
  uint32 id;
  uint16 target;
  TextureFlags flags;
};

TextureUnit AddTexture(uint32 hash, const Texture &hdr, const char *data);
TextureUnit LookupTexture(uint32 hash);

} // namespace prime::graphics
