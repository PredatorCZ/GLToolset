#include "graphics/texture.hpp"
#include "utils/flatbuffers.hpp"
#include "utils/texture.hpp"
#include <GL/glew.h>
#include <deque>
#include <functional>
#include <map>

#include "texture.fbs.hpp"

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

static void LoadBindedTextureLevels(DeferredPayload pl) {
  auto &hdr = pl.hdr;
  auto resource = prime::utils::RedirectTexture(
      prime::common::ResourceHash(pl.hash), pl.streamIndex);
  auto &data = prime::common::LoadResource(resource);
  uint8 minLevel = 255;
  auto &meta = *hdr.info();

  glBindTexture(meta.target(), pl.object);

  if (meta.flags() == TextureFlag::Compressed) {
    if (meta.flags() == TextureFlag::Volume) {
      // not implemented
    } else if (meta.flags() == TextureFlag::Array) {
      // not implemented
    } else {
      if (meta.numDims() == 1) {
        for (auto e : *hdr.entries()) {
          // uint32 width = meta.width() >> e->level();

          if (e->streamIndex() != pl.streamIndex) {
            continue;
          }

          minLevel = std::min(e->level(), minLevel);
        }
      } else if (meta.numDims() == 2) {
        for (auto e : *hdr.entries()) {
          uint32 height = meta.height() >> e->level();
          uint32 width = meta.width() >> e->level();

          if (std::max(height, width) >= CLAMP_RES ||
              e->streamIndex() != pl.streamIndex) {
            continue;
          }

          minLevel = std::min(e->level(), minLevel);
        }
      }
    }
  } else {
    if (meta.flags() == TextureFlag::Volume) {
      // not implemented
    } else if (meta.flags() == TextureFlag::Array) {
      // not implemented
    } else {
      if (meta.numDims() == 1) {
        for (auto e : *hdr.entries()) {
          // uint32 width = meta.width() >> e->level();

          if (e->streamIndex() != pl.streamIndex) {
            continue;
          }

          minLevel = std::min(e->level(), minLevel);
        }
      } else if (meta.numDims() == 2) {
        for (auto e : *hdr.entries()) {
          uint32 height = meta.height() >> e->level();
          uint32 width = meta.width() >> e->level();

          if (std::max(height, width) >= CLAMP_RES ||
              e->streamIndex() != pl.streamIndex) {
            continue;
          }

          minLevel = std::min(e->level(), minLevel);
        }
      }
    }
  }

  glTexParameteri(meta.target(), GL_TEXTURE_BASE_LEVEL, minLevel);
  glTexParameteri(meta.target(), GL_TEXTURE_MAX_LEVEL, meta.maxLevel());

  if (meta.flags() == TextureFlag::Compressed) {
    if (meta.flags() == TextureFlag::Volume) {
      // not implemented
    } else if (meta.flags() == TextureFlag::Array) {
      // not implemented
    } else {
      if (meta.numDims() == 1) {
        for (auto e : *hdr.entries()) {
          uint32 width = meta.width() >> e->level();

          if (e->streamIndex() != pl.streamIndex) {
            continue;
          }

          glCompressedTexImage1D(e->target(), e->level(), meta.internalFormat(),
                                 width, 0, e->bufferSize(),
                                 data.buffer.data() + e->bufferOffset());
        }
      } else if (meta.numDims() == 2) {
        for (auto e : *hdr.entries()) {
          uint32 height = meta.height() >> e->level();
          uint32 width = meta.width() >> e->level();

          if (std::max(height, width) >= CLAMP_RES ||
              e->streamIndex() != pl.streamIndex) {
            continue;
          }

          glCompressedTexImage2D(e->target(), e->level(), meta.internalFormat(),
                                 width, height, 0, e->bufferSize(),
                                 data.buffer.data() + e->bufferOffset());
        }
      }
    }
  } else {
    if (meta.flags() == TextureFlag::Volume) {
      // not implemented
    } else if (meta.flags() == TextureFlag::Array) {
      // not implemented
    } else {
      if (meta.numDims() == 1) {
        for (auto e : *hdr.entries()) {
          uint32 width = meta.width() >> e->level();

          if (e->streamIndex() != pl.streamIndex) {
            continue;
          }

          glTexImage1D(e->target(), e->level(), meta.internalFormat(), width, 0,
                       meta.format(), meta.type(),
                       data.buffer.data() + e->bufferOffset());
        }
      } else if (meta.numDims() == 2) {
        for (auto e : *hdr.entries()) {
          uint32 height = meta.height() >> e->level();
          uint32 width = meta.width() >> e->level();

          if (std::max(height, width) >= CLAMP_RES ||
              e->streamIndex() != pl.streamIndex) {
            continue;
          }

          glTexImage2D(e->target(), e->level(), meta.internalFormat(), width,
                       height, 0, meta.format(), meta.type(),
                       data.buffer.data() + e->bufferOffset());
        }
      }
    }
  }

  glBindTexture(meta.target(), 0);
  prime::common::FreeResource(data);

  if (meta.numStreams() == pl.streamIndex + 1) {
    auto &hdrData = prime::common::FindResource(
        flatbuffers::GetBufferStartFromRootPointer(&pl.hdr));
    prime::common::FreeResource(hdrData);
  }
};

static TextureUnit AddTexture(const prime::graphics::Texture &hdr,
                              uint32 nameHash) {
  TextureUnit unit;
  glGenTextures(1, &unit.id);
  auto &meta = *hdr.info();
  unit.target = meta.target();
  unit.flags = meta.flags();
  LoadBindedTextureLevels({hdr, nameHash, unit.id});

  if (auto swizzle = hdr.swizzle(); swizzle) {
    glTexParameteriv(meta.target(), GL_TEXTURE_SWIZZLE_RGBA,
                     swizzle->mask()->data());
  }

  for (uint32 i = 1; i < meta.numStreams(); i++) {
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

  common::LoadResource(
      common::ResourceHash(hash, common::GetClassHash<graphics::Texture>()));
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
            auto hdr = utils::GetFlatbuffer<Texture>(data);
            auto unit = AddTexture(*hdr, data.hash.name);
            TEXTURE_UNITS.emplace(data.hash.name, unit);
            TEXTURE_REMAPS.emplace(unit.id, data.hash.name);
          },
      .Delete = nullptr,
      .Handle = [](ResourceData &data) -> void * {
        return utils::GetFlatbuffer<Texture>(data);
      },
  });
};

REGISTER_CLASS(prime::graphics::Texture);
REGISTER_CLASS(prime::graphics::TextureStream<0>);
REGISTER_CLASS(prime::graphics::TextureStream<1>);
REGISTER_CLASS(prime::graphics::TextureStream<2>);
REGISTER_CLASS(prime::graphics::TextureStream<3>);
