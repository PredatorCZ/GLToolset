#pragma once
#include "spike/util/supercore.hpp"
#include <vector>

namespace prime::graphics {
enum class FrameBufferTextureType : uint16 {
  Color,
  Glow,
};

struct FrameBufferTexture {
  uint32 id;
  FrameBufferTextureType texturetype;
  uint16 internalFormat;
  uint16 format;
  uint16 type;

  void Init();
  void Resize(uint32 width, uint32 height);
};

struct FrameBuffer {
  uint32 bufferId;
  uint32 renderBufferId;
  std::vector<FrameBufferTexture> outTextures;
  uint32 width;
  uint32 height;

  void Finalize();
  void Resize(uint32 width_, uint32 height_);
  uint32 FindTexture(FrameBufferTextureType type) const;
  void AddTexture(FrameBufferTextureType type);
};

FrameBuffer CreateFrameBuffer(uint32 width, uint32 height);
} // namespace prime::graphics
