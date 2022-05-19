/*  GLTEX
    Copyright(C) 2022 Lukas Cone

    This program is free software : you can redistribute it and / or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.If not, see <https://www.gnu.org/licenses/>.
*/

#include "datas/app_context.hpp"
#include "datas/binreader_stream.hpp"
#include "datas/binwritter.hpp"
#include "datas/directory_scanner.hpp"
#include "datas/fileinfo.hpp"
#include "datas/master_printer.hpp"
#include "datas/reflector.hpp"
#include "datas/vectors_simd.hpp"
#include "graphics/texture.hpp"
#include "ispc_texcomp.h"
#include "project.h"
#include "stb_image.h"
#include "stb_image_resize.h"
#include <GL/gl.h>
#include <GL/glext.h>

es::string_view filters[]{
    ".jpeg$", ".jpg$", ".bmp$", ".psd$", ".tga$", ".gif$",
    ".hdr$",  ".pic$", ".ppm$", ".pgm$", {},
};

MAKE_ENUM(ENUMSCOPE(class RGBAType
                    : uint8, RGBAType),
          EMEMBER(RGBA, "8bit unsigned"),
          // EMEMBER(RGBA5551, "5bit RGB with 1bit alpha"),
          EMEMBER(BC3, "aka DXT5"), EMEMBER(BC7));
MAKE_ENUM(ENUMSCOPE(class RGBType
                    : uint8, RGBType),
          EMEMBER(RGBX, "8bit unsigned RGB with 8bit clean alpha padding"),
          // EMEMBER(RGB565, "5bit R and B, 6bit G"),
          EMEMBER(BC1, "aka DXT1"), EMEMBER(BC7));
MAKE_ENUM(ENUMSCOPE(class RGType
                    : uint8, RGType),
          EMEMBER(RG, "8bit unsigned"), EMEMBER(BC5, "aka ATI2"), EMEMBER(BC7));
MAKE_ENUM(ENUMSCOPE(class MonochromeType
                    : uint8, MonochromeType),
          EMEMBER(Monochrome, "8bit unsigned"), EMEMBER(BC4, "aka ATI1"),
          EMEMBER(BC7));
MAKE_ENUM(ENUMSCOPE(class NormalType
                    : uint8, NormalType),
          EMEMBER(BC3, "RGB are X axis, Alpha is Y axis"),
          EMEMBER(BC5, "Z axis and input alpha are ommited"),
          EMEMBER(BC5S, "signed values"), EMEMBER(BC7),
          EMEMBER(RG, "8bit unsigned"), EMEMBER(RGS, "8bit signed"));

constexpr size_t NUM_STREAMS = 4;

struct GLTEX : ReflectorBase<GLTEX> {
  RGBAType rgbaType = RGBAType::BC3;
  RGBType rgbType = RGBType::BC1;
  RGType rgType = RGType::BC5;
  MonochromeType monochromeType = MonochromeType::BC4;
  NormalType normalType = NormalType::BC5S;
  std::string normalMapPatterns;
  int32 streamLimit[NUM_STREAMS]{128, 2048, 4096, -1};
  PathFilter normalExts;
} settings;

REFLECT(
    CLASS(GLTEX),
    MEMBERNAME(rgbaType, "rgba-type", "4",
               ReflDesc{"Set output format for RGBA."}),
    MEMBERNAME(rgbType, "rgb-type", "3",
               ReflDesc{"Set output format for RGB."}),
    MEMBERNAME(rgType, "rg-type", "2",
               ReflDesc{
                   "Set output format for RG (aka greyscale with alpha)."}),
    MEMBERNAME(monochromeType, "monochrome-type", "1",
               ReflDesc{"Set output format for greyscale."}),
    MEMBERNAME(normalType, "normal-type", "n",
               ReflDesc{"Set output format for normal maps. Z axis (B "
                        "channel) and Alpha are ommitted."}),
    MEMBERNAME(normalMapPatterns, "normalmap-patterns", "p",
               ReflDesc{"Specify filename patterns for detecting normal "
                        "maps separated by comma."}),
    MEMBERNAME(streamLimit, "stream-limit",
               ReflDesc{
                   "Exclusive pixel limit per stream. Must be power of 2."}), )

static AppInfo_s appInfo{
    AppInfo_s::CONTEXT_VERSION,
    AppMode_e::CONVERT,
    ArchiveLoadType::FILTERED,
    GLTEX_DESC " v" GLTEX_VERSION ", " GLTEX_COPYRIGHT "Lukas Cone",
    reinterpret_cast<ReflectorFriend *>(&settings),
    filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

bool AppInitContext(const std::string &) {
  if (!settings.normalMapPatterns.empty()) {
    es::string_view sv(settings.normalMapPatterns);
    size_t lastPost = 0;
    auto found = sv.find(',');

    while (found != sv.npos) {
      auto sub = sv.substr(lastPost, found - lastPost);
      settings.normalExts.AddFilter(es::TrimWhitespace(sub));
      lastPost = ++found;
      found = sv.find(',', lastPost);
    }

    if (lastPost < sv.size()) {
      settings.normalExts.AddFilter(es::TrimWhitespace(sv.substr(lastPost)));
    }
  }

  return true;
}

uint16 texture2DCompressedFormats[]{
    GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
    GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
    GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
    GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
    GL_COMPRESSED_SRGB_S3TC_DXT1_EXT,
    GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT,
    GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT,
    GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,
    GL_COMPRESSED_RGBA_BPTC_UNORM,
    GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM,
    GL_COMPRESSED_RED_RGTC1,
    GL_COMPRESSED_RG_RGTC2,
    GL_COMPRESSED_SIGNED_RG_RGTC2,
};

uint16 texture2DTargets[]{
    GL_TEXTURE_2D,
    // GL_PROXY_TEXTURE_2D,
    GL_TEXTURE_1D_ARRAY,
    // GL_PROXY_TEXTURE_1D_ARRAY,
    GL_TEXTURE_RECTANGLE,
    // GL_PROXY_TEXTURE_RECTANGLE,
    GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
    // GL_PROXY_TEXTURE_CUBE_MAP,
};

uint16 texture2DFormats[]{
    GL_RED,           GL_RG,
    GL_RGB,           GL_BGR,
    GL_RGBA,          GL_BGRA,
    GL_RED_INTEGER,   GL_RG_INTEGER,
    GL_RGB_INTEGER,   GL_BGR_INTEGER,
    GL_RGBA_INTEGER,  GL_BGRA_INTEGER,
    GL_STENCIL_INDEX, GL_DEPTH_COMPONENT,
    GL_DEPTH_STENCIL,
};

uint16 texture2DTypes[]{
    GL_UNSIGNED_BYTE,
    GL_BYTE,
    GL_UNSIGNED_SHORT,
    GL_SHORT,
    GL_UNSIGNED_INT,
    GL_INT,
    GL_HALF_FLOAT,
    GL_FLOAT,
    GL_UNSIGNED_BYTE_3_3_2,
    GL_UNSIGNED_BYTE_2_3_3_REV,
    GL_UNSIGNED_SHORT_5_6_5,
    GL_UNSIGNED_SHORT_5_6_5_REV,
    GL_UNSIGNED_SHORT_4_4_4_4,
    GL_UNSIGNED_SHORT_4_4_4_4_REV,
    GL_UNSIGNED_SHORT_5_5_5_1,
    GL_UNSIGNED_SHORT_1_5_5_5_REV,
    GL_UNSIGNED_INT_8_8_8_8,
    GL_UNSIGNED_INT_8_8_8_8_REV,
    GL_UNSIGNED_INT_10_10_10_2,
    GL_UNSIGNED_INT_2_10_10_10_REV,
};

uint16 texture2DInternalFormat[]{
    GL_DEPTH_COMPONENT,
    GL_DEPTH_STENCIL,
    GL_RED,
    GL_RG,
    GL_RGB,
    GL_RGBA,
    // Sized internal formats
    GL_R8,
    GL_R8_SNORM,
    GL_R16,
    GL_R16_SNORM,
    GL_RG8,
    GL_RG8_SNORM,
    GL_RG16,
    GL_RG16_SNORM,
    GL_R3_G3_B2,
    GL_RGB4,
    GL_RGB5,
    GL_RGB8,
    GL_RGB8_SNORM,
    GL_RGB10,
    GL_RGB12,
    GL_RGB16_SNORM,
    GL_RGBA2,
    GL_RGBA4,
    GL_RGB5_A1,
    GL_RGBA8,
    GL_RGBA8_SNORM,
    GL_RGB10_A2,
    GL_RGB10_A2UI,
    GL_RGBA12,
    GL_RGBA16,
    GL_SRGB8,
    GL_SRGB8_ALPHA8,
    GL_R16F,
    GL_RG16F,
    GL_RGB16F,
    GL_RGBA16F,
    GL_R32F,
    GL_RG32F,
    GL_RGB32F,
    GL_RGBA32F,
    GL_R11F_G11F_B10F,
    GL_RGB9_E5,
    GL_R8I,
    GL_R8UI,
    GL_R16I,
    GL_R16UI,
    GL_R32I,
    GL_R32UI,
    GL_RG8I,
    GL_RG8UI,
    GL_RG16I,
    GL_RG16UI,
    GL_RG32I,
    GL_RG32UI,
    GL_RGB8I,
    GL_RGB8UI,
    GL_RGB16I,
    GL_RGB16UI,
    GL_RGB32I,
    GL_RGB32UI,
    GL_RGBA8I,
    GL_RGBA8UI,
    GL_RGBA16I,
    GL_RGBA16UI,
    GL_RGBA32I,
    GL_RGBA32UI,
};

struct RawImageData {
  void *data;
  uint32 rawSize;
  uint32 numChannels;
  uint32 origChannels;
  uint32 width;
  uint32 height;
  uint32 bcSize;

  void MipMap() {
    width /= 2;
    height /= 2;
    rawSize = width * height * numChannels;
    bcSize = (width / 4) * (height / 4);
  }
};

RawImageData GetImageData(BinReaderRef rd, bool isNormalMap) {
  stbi_io_callbacks cbs;
  cbs.read = [](void *user, char *data, int size) {
    static_cast<BinReaderRef *>(user)->ReadBuffer(data, size);
    return size; // todo eof?
  };
  cbs.skip = [](void *user, int n) {
    static_cast<BinReaderRef *>(user)->Skip(n);
  };
  cbs.eof = [](void *user) -> int {
    return static_cast<BinReaderRef *>(user)->IsEOF();
  };

  int x, y, channels;
  auto data =
      stbi_load_from_callbacks(&cbs, &rd, &x, &y, &channels, STBI_default);

  if (!data) {
    throw std::runtime_error("Couldn't convert image: " +
                             std::string(stbi_failure_reason()));
  }

  auto IsPowerOfTwo = [](size_t x) { return (x & (x - 1)) == 0; };
  auto RoundUpPowTwo = [](size_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
  };

  if (!(IsPowerOfTwo(x) && IsPowerOfTwo(y))) {
    printwarning("Image is not power of two, resizing.");
    int nx = RoundUpPowTwo(x);
    int ny = RoundUpPowTwo(y);

    uint8 *rData = static_cast<uint8 *>(malloc(nx * ny * channels));

    stbir_resize_uint8_generic(
        data, x, y, x * channels, rData, nx, ny, nx * channels, channels, 0, 0,
        stbir_edge::STBIR_EDGE_CLAMP, stbir_filter::STBIR_FILTER_DEFAULT,
        stbir_colorspace::STBIR_COLORSPACE_LINEAR, nullptr);
    free(data);
    data = rData;
    x = nx;
    y = ny;
  }

  auto ClampChannels = [&](int desiredChannels) {
    char *nData = static_cast<char *>(malloc(x * y * desiredChannels));
    memset(nData, 0, x * y * desiredChannels);

    for (int p = 0; p < x * y; p++) {
      char *destData = nData + (p * desiredChannels);
      memcpy(destData, data + (p * channels), desiredChannels);
      if (isNormalMap) {
        reinterpret_cast<uint8 &>(destData[1]) =
            0xff - reinterpret_cast<uint8 &>(destData[1]);
      }
    }

    free(data);
    data = reinterpret_cast<uint8 *>(nData);
    channels = desiredChannels;
  };

  RawImageData retVal;
  retVal.origChannels = channels;
  retVal.height = y;
  retVal.width = x;

  if (isNormalMap) {
    // todo monochrome, height > normal
    switch (settings.normalType) {
    case NormalType::RG:
    case NormalType::RGS:
    case NormalType::BC5:
    case NormalType::BC5S:
      if (channels > 2) {
        ClampChannels(2);
      }
      break;

    case NormalType::BC3:
      if (channels < 4) {
        ClampChannels(4);
      }
      break;

    case NormalType::BC7:
      if (channels < 4) {
        ClampChannels(4);
      }
      break;

    default:
      break;
    }
  } else {
    switch (channels) {
    case STBI_rgb: {
      switch (settings.rgbType) {
      case RGBType::BC1:
      case RGBType::RGBX:
      case RGBType::BC7:
        ClampChannels(4);
        break;

      default:
        break;
      }
    }
    case STBI_grey_alpha: {
      switch (settings.rgType) {
      case RGType::BC7:
        ClampChannels(4);
        break;

      default:
        break;
      }
    }
    case STBI_grey: {
      switch (settings.monochromeType) {
      case MonochromeType::BC7:
        ClampChannels(4);
        break;

      default:
        break;
      }
    }
    }
  }
  retVal.data = data;
  retVal.numChannels = channels;
  retVal.rawSize = x * y * channels;
  retVal.bcSize = (x / 4) * (y / 4);

  return retVal;
}

template <typename fc>
void ForEachMipmap(RawImageData &ctx, uint32 numMips, fc &&cb) {
  cb(0);

  for (uint32 m = 1; m < numMips; m++) {
    uint32 oldWidth = ctx.width;
    uint32 oldHeight = ctx.height;
    ctx.MipMap();
    uint8 *rData = static_cast<uint8 *>(malloc(ctx.rawSize));

    stbir_resize_uint8_generic(                                 //
        static_cast<uint8 *>(ctx.data), oldWidth, oldHeight, 0, //
        rData, ctx.width, ctx.height, 0, ctx.numChannels,       //
        STBIR_ALPHA_CHANNEL_NONE, 0, stbir_edge::STBIR_EDGE_CLAMP,
        stbir_filter::STBIR_FILTER_DEFAULT,
        stbir_colorspace::STBIR_COLORSPACE_LINEAR, nullptr);
    free(ctx.data);
    ctx.data = rData;
    cb(m);
  }
};

void AppProcessFile(std::istream &stream, AppContext *ctx) {
  AFileInfo fleInfo(ctx->outFile);
  const bool isNormalMap =
      settings.normalExts.IsFiltered(fleInfo.GetFilename());
  auto rawData = GetImageData(stream, isNormalMap);

  auto CountBits = [](size_t x) {
    size_t numBits = 0;

    while (x >>= 1) {
      numBits++;
    }

    return numBits;
  };

  using namespace prime::graphics;

  auto outFile = fleInfo.GetFullPathNoExt().to_string();
  char indexExt[] = ".gtbx";
  size_t currentStream = NUM_STREAMS;
  size_t maxUsedStream = 0;
  BinWritter wr;

  auto UpdateStream = [&] {
    const size_t minPixel = std::min(rawData.width, rawData.height);

    for (size_t i = 0; i < NUM_STREAMS; i++) {
      if (minPixel < settings.streamLimit[i]) {
        if (currentStream != i) {
          currentStream = i;
          maxUsedStream = std::max(maxUsedStream, i);
          indexExt[sizeof(indexExt) - 2] = '0' + i;
          BinWritter newWr(outFile + indexExt);
          std::swap(newWr, wr);
        }

        break;
      }
    }
  };

  std::vector<TextureEntry> entries;
  uint16 currentTarget = GL_TEXTURE_2D;
  // -1 because we want minimum 4x4 mip
  int16 numMips = CountBits(std::min(rawData.width, rawData.height)) - 1;

  Texture hdr{};
  hdr.height = rawData.height;
  hdr.width = rawData.width;
  hdr.numDims = 2;
  hdr.target = currentTarget;
  hdr.maxLevel = std::max(numMips - 1, 0);

  auto WriteTile = [&](uint32 level) {
    UpdateStream();
    rgba_surface surf;
    surf.height = rawData.height;
    surf.width = rawData.width;
    surf.stride = rawData.width * 4;
    surf.ptr = static_cast<uint8_t *>(rawData.data);

    TextureEntry entry{};
    entry.target = currentTarget;
    entry.level = level;
    entry.streamIndex = currentStream;
    entry.bufferOffset = wr.Tell();

    if (rawData.origChannels == STBI_rgb_alpha) {
      hdr.flags += TextureFlag::AlphaMasked;

      if (settings.rgbaType == RGBAType::RGBA) {
        hdr.format = GL_RGBA;
        hdr.internalFormat = GL_RGBA;
        hdr.type = GL_UNSIGNED_BYTE;
        entry.bufferSize = rawData.rawSize;
        wr.WriteBuffer(reinterpret_cast<char *>(rawData.data), rawData.rawSize);
      } else if (settings.rgbaType == RGBAType::BC3) {
        hdr.flags += TextureFlag::Compressed;
        hdr.internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        entry.bufferSize = rawData.bcSize * 16;
        std::string buffer;
        buffer.resize(entry.bufferSize);
        CompressBlocksBC3(&surf, reinterpret_cast<uint8_t *>(buffer.data()));
        wr.WriteContainer(buffer);
      } else if (settings.rgbaType == RGBAType::BC7) {
        hdr.flags += TextureFlag::Compressed;
        hdr.internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM;
        entry.bufferSize = rawData.bcSize * 16;
        std::string buffer;
        buffer.resize(entry.bufferSize);
        bc7_enc_settings prof;
        GetProfile_alpha_basic(&prof);
        CompressBlocksBC7(&surf, reinterpret_cast<uint8_t *>(buffer.data()),
                          &prof);
        wr.WriteContainer(buffer);
      }
    } else if (rawData.origChannels == STBI_rgb) {
      if (settings.rgbType == RGBType::RGBX) {
        hdr.format = GL_RGBA;
        hdr.internalFormat = GL_RGBA;
        hdr.type = GL_UNSIGNED_BYTE;
        entry.bufferSize = rawData.rawSize;
        wr.WriteBuffer(reinterpret_cast<char *>(rawData.data), rawData.rawSize);
      } else if (settings.rgbType == RGBType::BC1) {
        hdr.flags += TextureFlag::Compressed;
        hdr.internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        entry.bufferSize = rawData.bcSize * 8;
        std::string buffer;
        buffer.resize(entry.bufferSize);
        CompressBlocksBC1(&surf, reinterpret_cast<uint8_t *>(buffer.data()));
        wr.WriteContainer(buffer);
      } else if (settings.rgbType == RGBType::BC7) {
        hdr.flags += TextureFlag::Compressed;
        hdr.internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM;
        entry.bufferSize = rawData.bcSize * 16;
        std::string buffer;
        buffer.resize(entry.bufferSize);
        bc7_enc_settings prof;
        GetProfile_basic(&prof);
        CompressBlocksBC7(&surf, reinterpret_cast<uint8_t *>(buffer.data()),
                          &prof);
        wr.WriteContainer(buffer);
      }
    } else if (rawData.origChannels == STBI_grey_alpha) {
      if (settings.rgType == RGType::RG) {
        hdr.format = GL_RG;
        hdr.internalFormat = GL_RG;
        hdr.type = GL_UNSIGNED_BYTE;
        entry.bufferSize = rawData.rawSize;
        wr.WriteBuffer(reinterpret_cast<char *>(rawData.data), rawData.rawSize);
      } else if (settings.rgType == RGType::BC5) {
        hdr.flags += TextureFlag::Compressed;
        hdr.internalFormat = GL_COMPRESSED_RG_RGTC2;
        entry.bufferSize = rawData.bcSize * 16;
        std::string buffer;
        buffer.resize(entry.bufferSize);
        CompressBlocksBC5(&surf, reinterpret_cast<uint8_t *>(buffer.data()));
        wr.WriteContainer(buffer);
      } else if (settings.rgType == RGType::BC7) {
        hdr.flags += TextureFlag::Compressed;
        hdr.internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM;
        entry.bufferSize = rawData.bcSize * 16;
        std::string buffer;
        buffer.resize(entry.bufferSize);
        bc7_enc_settings prof;
        GetProfile_basic(&prof);
        CompressBlocksBC7(&surf, reinterpret_cast<uint8_t *>(buffer.data()),
                          &prof);
        wr.WriteContainer(buffer);
      }
    } else if (rawData.origChannels == STBI_grey) {
      if (settings.monochromeType == MonochromeType::Monochrome) {
        hdr.format = GL_RED;
        hdr.internalFormat = GL_RED;
        hdr.type = GL_UNSIGNED_BYTE;
        entry.bufferSize = rawData.rawSize;
        wr.WriteBuffer(reinterpret_cast<char *>(rawData.data), rawData.rawSize);
      } else if (settings.monochromeType == MonochromeType::BC4) {
        hdr.flags += TextureFlag::Compressed;
        hdr.internalFormat = GL_COMPRESSED_RED_RGTC1;
        entry.bufferSize = rawData.bcSize * 8;
        std::string buffer;
        buffer.resize(entry.bufferSize);
        CompressBlocksBC5(&surf, reinterpret_cast<uint8_t *>(buffer.data()));
        wr.WriteContainer(buffer);
      } else if (settings.monochromeType == MonochromeType::BC7) {
        hdr.flags += TextureFlag::Compressed;
        hdr.internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM;
        entry.bufferSize = rawData.bcSize * 16;
        std::string buffer;
        buffer.resize(entry.bufferSize);
        bc7_enc_settings prof;
        GetProfile_basic(&prof);
        CompressBlocksBC7(&surf, reinterpret_cast<uint8_t *>(buffer.data()),
                          &prof);
        wr.WriteContainer(buffer);
      }
    }

    entries.push_back(entry);
  };

  auto WriteTileNormal = [&](uint32 level) {
    UpdateStream();
    rgba_surface surf;
    surf.height = rawData.height;
    surf.width = rawData.width;
    surf.stride = rawData.width * rawData.numChannels;
    surf.ptr = static_cast<uint8_t *>(rawData.data);
    hdr.flags += TextureFlag::NormalDeriveZAxis;
    hdr.flags += TextureFlag::NormalMap;

    TextureEntry entry{};
    entry.target = currentTarget;
    entry.level = level;
    entry.streamIndex = currentStream;
    entry.bufferOffset = wr.Tell();

    if (settings.normalType == NormalType::BC3) {
      hdr.flags += TextureFlag::Compressed;
      hdr.flags += TextureFlag::Swizzle;
      uint32 swMask[]{GL_RED, GL_ALPHA, GL_ONE, GL_ONE};
      memcpy(hdr.swizzleMask, swMask, sizeof(swMask));
      hdr.internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
      entry.bufferSize = rawData.bcSize * 16;
      std::string buffer;
      buffer.resize(entry.bufferSize);

      // Reswizzle
      const size_t stride = 4;
      const size_t numLoops = rawData.rawSize / stride;
      const size_t restBytes = rawData.rawSize % stride;
      char *data = static_cast<char *>(rawData.data);
      uint8 *rData = static_cast<uint8 *>(malloc(rawData.rawSize));
      surf.ptr = rData;

      auto process = [](const char *data) {
        UCVector4 in;
        memcpy(&in, data, 4);
        in.W = in.Y;
        in.Y = in.X;
        in.z = in.X;
        return in;
      };

      for (size_t i = 0; i < numLoops; i++) {
        const size_t index = i * stride;
        const UCVector4 value = process(data + index);
        memcpy(rData + index, &value, stride);
      }

      if (restBytes) {
        const size_t index = numLoops * stride;
        const UCVector4 value = process(data + index);
        memcpy(rData + index, &value, restBytes);
      }

      CompressBlocksBC3(&surf, reinterpret_cast<uint8_t *>(buffer.data()));
      free(rData);
      wr.WriteContainer(buffer);
    } else if (settings.normalType == NormalType::BC7) {
      hdr.flags += TextureFlag::Compressed;
      hdr.flags -= TextureFlag::NormalDeriveZAxis;
      hdr.internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM;
      entry.bufferSize = rawData.bcSize * 16;
      std::string buffer;
      buffer.resize(entry.bufferSize);
      bc7_enc_settings prof;
      GetProfile_basic(&prof);
      CompressBlocksBC7(&surf, reinterpret_cast<uint8_t *>(buffer.data()),
                        &prof);
      wr.WriteContainer(buffer);
    } else if (settings.normalType == NormalType::BC5) {
      hdr.flags += TextureFlag::Compressed;
      hdr.internalFormat = GL_COMPRESSED_RG_RGTC2;
      entry.bufferSize = rawData.bcSize * 16;
      std::string buffer;
      buffer.resize(entry.bufferSize);
      CompressBlocksBC5(&surf, reinterpret_cast<uint8_t *>(buffer.data()));
      wr.WriteContainer(buffer);
    } else if (settings.normalType == NormalType::BC5S) {
      hdr.flags += TextureFlag::Compressed;
      hdr.internalFormat = GL_COMPRESSED_SIGNED_RG_RGTC2;
      hdr.flags += TextureFlag::SignedNormal;
      entry.bufferSize = rawData.bcSize * 16;
      std::string buffer;
      buffer.resize(entry.bufferSize);
      CompressBlocksBC5S(&surf, reinterpret_cast<uint8_t *>(buffer.data()));
      wr.WriteContainer(buffer);
    } else {
      hdr.format = GL_RG;
      entry.bufferSize = rawData.rawSize;
      if (settings.normalType == NormalType::RG) {
        hdr.internalFormat = GL_RG;
        hdr.type = GL_UNSIGNED_BYTE;
        wr.WriteBuffer(reinterpret_cast<char *>(rawData.data), rawData.rawSize);
      } else if (settings.normalType == NormalType::RGS) {
        hdr.type = GL_BYTE;
        hdr.internalFormat = GL_RG8_SNORM;
        hdr.flags += TextureFlag::SignedNormal;

        // Convert to signed space
        const size_t stride = 4;
        const size_t numLoops = rawData.rawSize / stride;
        const size_t restBytes = rawData.rawSize % stride;
        char *data = static_cast<char *>(rawData.data);
        char *rData = static_cast<char *>(malloc(rawData.rawSize));

        auto process = [](const char *data) {
          UCVector4 in;
          memcpy(&in, data, 4);
          IVector4A16 casted(in.Convert<int32>());
          casted -= 0x80;
          CVector4 retVal = casted.Convert<int8>();
          return retVal;
        };

        for (size_t i = 0; i < numLoops; i++) {
          const size_t index = i * stride;
          const CVector4 value = process(data + index);
          memcpy(rData + index, &value, stride);
        }

        if (restBytes) {
          const size_t index = numLoops * stride;
          const CVector4 value = process(data + index);
          memcpy(rData + index, &value, restBytes);
        }
        wr.WriteBuffer(rData, rawData.rawSize);
        free(rData);
      }
    }

    entries.push_back(entry);
  };

  if (isNormalMap) {
    ForEachMipmap(rawData, numMips, WriteTileNormal);
  } else {
    ForEachMipmap(rawData, numMips, WriteTile);
  }

  if (rawData.data) {
    free(rawData.data);
  }

  std::sort(entries.begin(), entries.end(),
            [](const prime::graphics::TextureEntry &i1,
               const prime::graphics::TextureEntry &i2) {
              if (i1.streamIndex == i2.streamIndex) {
                return i1.level > i2.level;
              }

              return i1.streamIndex < i2.streamIndex;
            });

  hdr.entries.pointer = sizeof(Texture);
  hdr.entries.numItems = entries.size();
  hdr.numStreams = maxUsedStream + 1;
  outFile = fleInfo.GetFullPathNoExt().to_string() + ".gth";
  BinWritter wrh(outFile);
  wrh.Write(hdr);
  wrh.WriteContainer(entries);
}
