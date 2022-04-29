#pragma once
#include "datas/flags.hpp"

enum TEXFlag : uint16 {
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

struct TEXEntry {
  uint16 target;
  uint8 level;
  uint8 reserved;
  uint32 bufferSize;
  uint64 bufferOffset;
};

using TEXFlags = es::Flags<TEXFlag>;

struct TEX {
  static constexpr uint32 ID = CompileFourCC("TEX0");
  uint32 id;
  uint32 dataSize;
  uint16 numEnries;
  uint16 width;
  uint16 height;
  uint16 depth;
  uint16 internalFormat;
  uint16 format;
  uint16 type;
  uint16 target;
  uint8 maxLevel;
  uint8 numDims;
  TEXFlags flags;
  int32 swizzleMask[4];
};
