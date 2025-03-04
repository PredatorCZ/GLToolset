#pragma once
#include "common/array.hpp"
#include "common/string.hpp"

namespace prime::utils {
enum class TextureCompilerRGBAType : uint8 {
  RGBA,
  BC1,
  BC3,
  BC7,
  RGBA5551,
};

enum class TextureCompilerRGBType : uint8 {
  RGBX,
  BC1,
  BC7,
  RGB565,
};

enum class TextureCompilerRGType : uint8 {
  RG,
  BC5,
  BC7,
};

enum class TextureCompilerMonochromeType : uint8 {
  Monochrome,
  BC4,
  BC7,
};

enum class TextureCompilerNormalType : uint8 {
  BC3,
  BC5,
  BC5S,
  BC7,
  RG,
  RGS,
};

enum class TextureCompilerEntryType : uint8 {
  Mipmap,
  FrontFace,
  BackFace,
  LeftFace,
  RightFace,
  TopFace,
  BottomFace,
};
} // namespace prime::utils

HASH_CLASS(prime::utils::TextureCompilerRGBAType);
HASH_CLASS(prime::utils::TextureCompilerRGBType);
HASH_CLASS(prime::utils::TextureCompilerRGType);
HASH_CLASS(prime::utils::TextureCompilerMonochromeType);
HASH_CLASS(prime::utils::TextureCompilerNormalType);
HASH_CLASS(prime::utils::TextureCompilerEntryType);

namespace prime::utils {
struct TextureCompilerEntry {
  uint8 level = 0;
  TextureCompilerEntryType type = TextureCompilerEntryType::Mipmap;
  common::LocalArray32<common::String> paths;
  common::LocalArray32<uint32> crcs;
  uint32 stride = 0;
};
} // namespace prime::utils

HASH_CLASS(prime::utils::TextureCompilerEntry);

namespace prime::utils {
struct TextureCompiler : common::Resource<TextureCompiler> {
  constexpr static size_t NUM_STREAMS = 4;
  TextureCompilerRGBAType rgbaType = TextureCompilerRGBAType::BC3;
  TextureCompilerRGBType rgbType = TextureCompilerRGBType::BC1;
  TextureCompilerRGType rgType = TextureCompilerRGType::BC5;
  TextureCompilerMonochromeType monochromeType =
      TextureCompilerMonochromeType::BC4;
  TextureCompilerNormalType normalType = TextureCompilerNormalType::BC5S;
  bool isNormalMap = false;
  bool generateMipmaps = true;
  bool isVolumetric = false;
  int32 streamLimit[NUM_STREAMS]{128, 2048, 4096, -1};
  common::LocalArray16<TextureCompilerEntry> entries;
};
} // namespace prime::utils

CLASS_RESOURCE(1, prime::utils::TextureCompiler);
