#include "graphics/detail/texture.hpp"
#include "utils/texture.hpp"
#include <GL/glew.h>
#include <deque>
#include <functional>
#include <map>

using namespace prime::graphics;

struct DeferredPayload {
  const prime::graphics::Texture &hdr;
  uint32 hash;
  uint32 object;
  uint32 streamIndex = 0;
};

static std::map<uint32, TextureUnit> TEXTURE_UNITS;
static std::map<uint32, uint32> TEXTURE_REMAPS;
static std::deque<std::function<void()>> DEFERRED_LOADER_QUEUE[3];
static uint32 MIN_DEFER_LEVEL = 1;
static uint32 CLAMP_RES = 4096;
static TextureUnit ERROR_TEXTURE{};

static void MakeErrorTexture() {
  if (ERROR_TEXTURE.id == 0) {
    ERROR_TEXTURE.target = GL_TEXTURE_2D;
    glGenTextures(1, &ERROR_TEXTURE.id);
    glBindTexture(ERROR_TEXTURE.target, ERROR_TEXTURE.id);
    uint32 data = 0xffff00ff;
    glTexImage2D(ERROR_TEXTURE.target, 0, GL_RGBA8, 1, 1, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, &data);
    glBindTexture(ERROR_TEXTURE.target, 0);
  }
}

static void LoadBindedTextureLevels(DeferredPayload pl) {
  auto &hdr = pl.hdr;
  auto resource = prime::utils::RedirectTexture(
      prime::common::ResourceHash(pl.hash), pl.streamIndex);
  auto &data = prime::common::LoadResource(resource);
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
          // uint32 width = meta.width() >> e->level();

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
          // uint32 width = meta.width() >> e->level();

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
  prime::common::FreeResource(data);

  /*if (hdr.numStreams == pl.streamIndex + 1) {
    auto &hdrData = prime::common::FindResource(&pl.hdr);
    prime::common::FreeResource(hdrData);
  }*/
};

static TextureUnit AddTexture(const prime::graphics::Texture &hdr,
                              uint32 nameHash) {
  TextureUnit unit;
  glGenTextures(1, &unit.id);
  unit.target = hdr.target;
  unit.flags = hdr.flags;
  LoadBindedTextureLevels({hdr, nameHash, unit.id});

  if (hdr.swizzle.numItems == 4) {
    glTexParameteriv(hdr.target, GL_TEXTURE_SWIZZLE_RGBA, hdr.swizzle.begin());
  }

  for (uint32 i = 1; i < hdr.numStreams; i++) {
    if (i >= MIN_DEFER_LEVEL) {
      DEFERRED_LOADER_QUEUE[i - 1].emplace_back(std::bind(
          LoadBindedTextureLevels, DeferredPayload{hdr, nameHash, unit.id, i}));
    } else {
      LoadBindedTextureLevels({hdr, nameHash, unit.id, i});
    }
  }

  return unit;
}

TextureUnit prime::graphics::LookupTexture(uint32 hash) {
  if (TEXTURE_UNITS.contains(hash)) {
    return TEXTURE_UNITS.at(hash);
  }

  if (TEXTURE_REMAPS.contains(hash)) {
    return TEXTURE_UNITS.at(TEXTURE_REMAPS.at(hash));
  }

  try {
    common::LoadResource(
        common::ResourceHash(hash, common::GetClassHash<graphics::Texture>()));
  } catch (const es::FileNotFoundError &) {
    MakeErrorTexture();
    return ERROR_TEXTURE;
  }

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

template <> class prime::common::InvokeGuard<Texture> {
  static inline const bool data = prime::common::AddResourceHandle<Texture>({
      .Process =
          [](ResourceData &data) {
            auto hdr = data.As<graphics::Texture>();
            auto unit = AddTexture(*hdr, data.hash.name);
            TEXTURE_UNITS.emplace(data.hash.name, unit);
            TEXTURE_REMAPS.emplace(unit.id, data.hash.name);
          },
      .Delete = nullptr,
  });
};

REGISTER_CLASS(prime::graphics::Texture);
REGISTER_CLASS(prime::graphics::TextureStream<0>);
REGISTER_CLASS(prime::graphics::TextureStream<1>);
REGISTER_CLASS(prime::graphics::TextureStream<2>);
REGISTER_CLASS(prime::graphics::TextureStream<3>);
