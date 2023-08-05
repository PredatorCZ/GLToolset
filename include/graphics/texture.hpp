#pragma once
#include "common/core.hpp"
#include "spike/type/flags.hpp"

namespace prime::common {
union ResourceHash;
} // namespace prime::common

namespace prime::graphics {
struct Texture;
template <uint32 streamId> struct TextureStream;

enum TextureFlag : uint16 {
  Cubemap,
  Array,
  Volume,
  Compressed,
  AlphaMasked,
  NormalMap,
  NormalDeriveZAxis,
  SignedNormal,
};

using TextureFlags = es::Flags<TextureFlag>;

struct TextureUnit {
  uint32 id;
  uint16 target;
  TextureFlags flags;
};

TextureUnit LookupTexture(uint32 hash);
void ClampTextureResolution(uint32 clampedRes);
// Range [1, 3], default 1
void MinimumStreamIndexForDeferredLoading(uint32 minId);

void StreamTextures(size_t streamIndex);
} // namespace prime::graphics

CLASS_RESOURCE(1, prime::graphics::Texture);
CLASS_EXT(prime::graphics::TextureStream<0>);
CLASS_EXT(prime::graphics::TextureStream<1>);
CLASS_EXT(prime::graphics::TextureStream<2>);
CLASS_EXT(prime::graphics::TextureStream<3>);
