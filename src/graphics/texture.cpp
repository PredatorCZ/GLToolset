#include "graphics/texture.hpp"
#include <GL/glew.h>
#include <map>

using namespace prime::graphics;

static std::map<uint32, TextureUnit> textureUnits;

TextureUnit prime::graphics::AddTexture(uint32 hash, const Texture &hdr,
                                        const char *data) {
  TextureUnit unit;
  glGenTextures(1, &unit.id);
  unit.target = hdr.target;
  unit.flags = hdr.flags;
  textureUnits.insert({hash, unit});
  glBindTexture(hdr.target, unit.id);
  glTexParameteri(hdr.target, GL_TEXTURE_BASE_LEVEL, 0);
  glTexParameteri(hdr.target, GL_TEXTURE_MAX_LEVEL, hdr.maxLevel);

  if (hdr.flags == TextureFlag::Swizzle) {
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, hdr.swizzleMask);
  }

  if (hdr.flags == TextureFlag::Compressed) {
    if (hdr.flags == TextureFlag::Volume) {
      // not implemented
    } else if (hdr.flags == TextureFlag::Array) {
      // not implemented
    } else {
      if (hdr.numDims == 1) {
        for (auto &e : hdr.entries) {
          uint32 width = hdr.width >> e.level;
          glCompressedTexImage1D(e.target, e.level, hdr.internalFormat, width,
                                 0, e.bufferSize, data + e.bufferOffset);
        }
      } else if (hdr.numDims == 2) {
        for (auto &e : hdr.entries) {
          uint32 height = hdr.height >> e.level;
          uint32 width = hdr.width >> e.level;
          glCompressedTexImage2D(e.target, e.level, hdr.internalFormat, width,
                                 height, 0, e.bufferSize,
                                 data + e.bufferOffset);
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
          glTexImage1D(e.target, e.level, hdr.internalFormat, width, 0,
                       hdr.format, hdr.type, data + e.bufferOffset);
        }
      } else if (hdr.numDims == 2) {
        for (auto &e : hdr.entries) {
          uint32 height = hdr.height >> e.level;
          uint32 width = hdr.width >> e.level;
          glTexImage2D(e.target, e.level, hdr.internalFormat, width, height, 0,
                       hdr.format, hdr.type, data + e.bufferOffset);
        }
      }
    }
  }

  return unit;
}

TextureUnit prime::graphics::LookupTexture(uint32 hash) {
  return textureUnits.at(hash);
}
