#pragma once
#include "common/array.hpp"
#include "datas/flags.hpp"
#include <memory>

namespace prime::common {
struct ResourceData;
union ResourceHash;
}

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
  uint8 streamIndex;
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
  uint8 reserved;
  uint8 numStreams;
  TextureFlags flags;
  int32 swizzleMask[4];
};

struct TextureUnit {
  uint32 id;
  uint16 target;
  TextureFlags flags;
};

template <uint32 streamId> struct TextureStream;

TextureUnit AddTexture(std::shared_ptr<common::ResourceData> res);
TextureUnit LookupTexture(uint32 hash);
void ClampTextureResolution(uint32 clampedRes);
// Range [1, 3], default 1
void MinimumStreamIndexForDeferredLoading(uint32 minId);

void StreamTextures(size_t streamIndex);

common::ResourceHash RedirectTexture(common::ResourceHash tex, size_t index);
} // namespace prime::graphics

HASH_CLASS(prime::graphics::Texture);
HASH_CLASS(prime::graphics::TextureStream<0>);
HASH_CLASS(prime::graphics::TextureStream<1>);
HASH_CLASS(prime::graphics::TextureStream<2>);
HASH_CLASS(prime::graphics::TextureStream<3>);

template <>
constexpr std::string_view
prime::common::GetClassExtension<prime::graphics::Texture>() {
  return "gth";
}

template <>
constexpr std::string_view
prime::common::GetClassExtension<prime::graphics::TextureStream<0>>() {
  return "gtb0";
}

template <>
constexpr std::string_view
prime::common::GetClassExtension<prime::graphics::TextureStream<1>>() {
  return "gtb1";
}

template <>
constexpr std::string_view
prime::common::GetClassExtension<prime::graphics::TextureStream<2>>() {
  return "gtb2";
}

template <>
constexpr std::string_view
prime::common::GetClassExtension<prime::graphics::TextureStream<3>>() {
  return "gtb3";
}
