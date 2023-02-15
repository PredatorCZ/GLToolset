#include "graphics/texture.hpp"
#include "common/resource.hpp"
#include <GL/glew.h>
#include <deque>
#include <functional>
#include <map>

using namespace prime::graphics;

struct DeferredPayload {
  std::shared_ptr<prime::common::ResourceData> hdrRes;
  uint32 object;
  uint32 streamIndex = 0;
};

static std::map<uint32, TextureUnit> TEXTURE_UNITS;
static std::deque<std::function<void()>> DEFERRED_LOADER_QUEUE[3];
static uint32 MIN_DEFER_LEVEL = 1;
static uint32 CLAMP_RES = 4096;

prime::common::ResourceHash
prime::graphics::RedirectTexture(prime::common::ResourceHash tex,
                                 size_t index) {
  uint32 classHash = [&] {
    switch (index) {
    case 0:
      return prime::common::GetClassHash<TextureStream<0>>();
    case 1:
      return prime::common::GetClassHash<TextureStream<1>>();
    case 2:
      return prime::common::GetClassHash<TextureStream<2>>();
    case 3:
      return prime::common::GetClassHash<TextureStream<3>>();
    }
  }();

  return prime::common::ResourceHash{tex.name, classHash};
}

static void LoadBindedTextureLevels(DeferredPayload pl) {
  auto hdrPtr = pl.hdrRes->As<Texture>();
  auto &hdr = *hdrPtr;
  auto &data = prime::common::LoadResource(
      RedirectTexture(pl.hdrRes->hash, pl.streamIndex));
  uint8 minLevel = 255;

  glBindTexture(hdr.target, pl.object);

  if (hdr.flags == TextureFlag::Compressed) {
    if (hdr.flags == TextureFlag::Volume) {
      // not implemented
    } else if (hdr.flags == TextureFlag::Array) {
      // not implemented
    } else {
      if (hdr.numDims == 1) {
        for (auto &e : hdr.entries) {
          uint32 width = hdr.width >> e.level;

          if (e.streamIndex != pl.streamIndex) {
            continue;
          }

          minLevel = std::min(e.level, minLevel);
        }
      } else if (hdr.numDims == 2) {
        for (auto &e : hdr.entries) {
          uint32 height = hdr.height >> e.level;
          uint32 width = hdr.width >> e.level;

          if (std::max(height, width) >= CLAMP_RES ||
              e.streamIndex != pl.streamIndex) {
            continue;
          }

          minLevel = std::min(e.level, minLevel);
        }
      }
    }
  } else {
    if (hdr.flags == TextureFlag::Volume) {
      // not implemented
    } else if (hdr.flags == TextureFlag::Array) {
      // not implemented
    } else {
      if (hdr.numDims == 1) {
        for (auto &e : hdr.entries) {
          uint32 width = hdr.width >> e.level;

          if (e.streamIndex != pl.streamIndex) {
            continue;
          }

          minLevel = std::min(e.level, minLevel);
        }
      } else if (hdr.numDims == 2) {
        for (auto &e : hdr.entries) {
          uint32 height = hdr.height >> e.level;
          uint32 width = hdr.width >> e.level;

          if (std::max(height, width) >= CLAMP_RES ||
              e.streamIndex != pl.streamIndex) {
            continue;
          }

          minLevel = std::min(e.level, minLevel);
        }
      }
    }
  }

  glTexParameteri(hdr.target, GL_TEXTURE_BASE_LEVEL, minLevel);
  glTexParameteri(hdr.target, GL_TEXTURE_MAX_LEVEL, hdr.maxLevel);

  if (hdr.flags == TextureFlag::Compressed) {
    if (hdr.flags == TextureFlag::Volume) {
      // not implemented
    } else if (hdr.flags == TextureFlag::Array) {
      // not implemented
    } else {
      if (hdr.numDims == 1) {
        for (auto &e : hdr.entries) {
          uint32 width = hdr.width >> e.level;

          if (e.streamIndex != pl.streamIndex) {
            continue;
          }

          glCompressedTexImage1D(e.target, e.level, hdr.internalFormat, width,
                                 0, e.bufferSize,
                                 data.buffer.data() + e.bufferOffset);
        }
      } else if (hdr.numDims == 2) {
        for (auto &e : hdr.entries) {
          uint32 height = hdr.height >> e.level;
          uint32 width = hdr.width >> e.level;

          if (std::max(height, width) >= CLAMP_RES ||
              e.streamIndex != pl.streamIndex) {
            continue;
          }

          glCompressedTexImage2D(e.target, e.level, hdr.internalFormat, width,
                                 height, 0, e.bufferSize,
                                 data.buffer.data() + e.bufferOffset);
        }
      }
    }
  } else {
    if (hdr.flags == TextureFlag::Volume) {
      // not implemented
    } else if (hdr.flags == TextureFlag::Array) {
      // not implemented
    } else {
      if (hdr.numDims == 1) {
        for (auto &e : hdr.entries) {
          uint32 width = hdr.width >> e.level;

          if (e.streamIndex != pl.streamIndex) {
            continue;
          }

          glTexImage1D(e.target, e.level, hdr.internalFormat, width, 0,
                       hdr.format, hdr.type,
                       data.buffer.data() + e.bufferOffset);
        }
      } else if (hdr.numDims == 2) {
        for (auto &e : hdr.entries) {
          uint32 height = hdr.height >> e.level;
          uint32 width = hdr.width >> e.level;

          if (std::max(height, width) >= CLAMP_RES ||
              e.streamIndex != pl.streamIndex) {
            continue;
          }

          glTexImage2D(e.target, e.level, hdr.internalFormat, width, height, 0,
                       hdr.format, hdr.type,
                       data.buffer.data() + e.bufferOffset);
        }
      }
    }
  }

  glBindTexture(hdr.target, 0);
};

TextureUnit
prime::graphics::AddTexture(std::shared_ptr<prime::common::ResourceData> res) {
  auto hdrPtr = res->As<Texture>();
  auto &hdr = *hdrPtr;
  TextureUnit unit;
  glGenTextures(1, &unit.id);
  unit.target = hdr.target;
  unit.flags = hdr.flags;
  TEXTURE_UNITS.insert({res->hash.name, unit});
  LoadBindedTextureLevels({res, unit.id});

  if (hdr.flags == TextureFlag::Swizzle) {
    glTexParameteriv(hdr.target, GL_TEXTURE_SWIZZLE_RGBA, hdr.swizzleMask);
  }

  for (uint32 i = 1; i < hdr.numStreams; i++) {
    if (i >= MIN_DEFER_LEVEL) {
      DEFERRED_LOADER_QUEUE[i - 1].emplace_back(
          std::bind(LoadBindedTextureLevels, DeferredPayload{res, unit.id, i}));
    } else {
      LoadBindedTextureLevels({res, unit.id, i});
    }
  }

  return unit;
}

TextureUnit prime::graphics::LookupTexture(uint32 hash) {
  return TEXTURE_UNITS.at(hash);
}

void prime::graphics::ClampTextureResolution(uint32 clampedRes) {
  CLAMP_RES = clampedRes;
}

void prime::graphics::MinimumStreamIndexForDeferredLoading(uint32 minId) {
  MIN_DEFER_LEVEL = minId;
}

void prime::graphics::StreamTextures(size_t streamIndex) {
  while (!DEFERRED_LOADER_QUEUE[streamIndex].empty()) {
    DEFERRED_LOADER_QUEUE[streamIndex].back()();
    DEFERRED_LOADER_QUEUE[streamIndex].pop_back();
  }
}
