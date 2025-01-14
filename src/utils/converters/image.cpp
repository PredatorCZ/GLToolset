#include "ispc_texcomp.h"
#include "spike/app_context.hpp"
#include "spike/crypto/crc32.hpp"
#include "spike/io/binreader_stream.hpp"
#include "spike/io/binwritter_stream.hpp"
#include "spike/io/directory_scanner.hpp"
#include "spike/master_printer.hpp"
#include "spike/reflect/reflector.hpp"
#include "spike/type/vectors_simd.hpp"
#include "stb_image.h"
#include "stb_image_resize.h"
#include <optional>

#include <GL/gl.h>
#include <GL/glext.h>

#include "graphics/detail/texture.hpp"
#include "utils/debug.hpp"
#include "utils/texture.hpp"

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
  std::string normalMapPatterns = "_normal$";
  std::string normalMapPatternsOld;
  int32 streamLimit[NUM_STREAMS]{128, 2048, 4096, -1};
  PathFilter normalExts;
};

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
namespace {
GLTEX &Settings() {
  static GLTEX settings{};

  if (settings.normalMapPatterns != settings.normalMapPatternsOld) {
    settings.normalMapPatternsOld = settings.normalMapPatterns;
    if (!settings.normalMapPatterns.empty()) {
      std::string_view sv(settings.normalMapPatterns);
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
  }

  return settings;
}

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

float RGBtoHue(Vector4A16 color) {
  Vector4A16 row1(0, 2, 4, 0);
  Vector4A16 row2(color.y, color.z, color.x, 0);
  Vector4A16 row3(color.z, color.x, color.y, 0);
  Vector4A16 comp1 = (row1 + (row2 - row3)) / 6;

  float retval;

  if (color.x == 1.f) {
    retval = comp1.x;
  } else if (color.y == 1.f) {
    retval = comp1.y;
  } else if (color.z == 1.f) {
    retval = comp1.z;
  } else {
    throw std::logic_error("Float compare error");
  }

  if (retval < 0.f) {
    retval += 1.f;
  }

  return retval;
}

struct PixelBlock4x4 {
  UCVector4 data[16];
};

struct Histogram3 {
  UIVector values[0x100];
};

PixelBlock4x4 RGBToHSL(PixelBlock4x4 block) {
  float avgHue = 0;
  float avgCount = 0;
  Vector offsetScaleHue[16];
  size_t curItem = 0;

  for (auto &b : block.data) {
    float offset = std::min(std::min(b.x, b.y), b.z);
    float upperBound = std::max(std::max(b.x, b.y), b.z);
    float scale = upperBound - offset;
    float hue = 0.f;
    float lightness = (offset + upperBound) * 0.5f;
    float saturation = 0.f;

    if (scale > 4.f) {
      if (lightness <= 255.f * 0.5f) {
        saturation = scale / (upperBound + offset);
      } else {
        saturation = scale / (2.f * 255.f - upperBound - offset);
      }

      Vector4A16 unpacked(b.Convert<float>());
      unpacked -= offset;
      unpacked /= scale;

      hue = RGBtoHue(unpacked) * 255.f;
      avgHue += hue;
      avgCount += 1.f;
    } else {
      scale = 0.f;
    }

    offsetScaleHue[curItem++] = {scale, offset, hue};
  }

  curItem = 0;
  avgHue /= avgCount;

  PixelBlock4x4 outBlock;

  for (auto &b : offsetScaleHue) {
    if (b.x < 1.f) {
      b.z = avgHue;
    }

    Vector4A16 unpacked(b, block.data[curItem].w);
    unpacked = Vector4A16(_mm_round_ps(unpacked._data, _MM_ROUND_NEAREST));
    outBlock.data[curItem++] = unpacked.Convert<uint8>();
  }

  return outBlock;
}

void ConvertToHSL(RawImageData &data) {
  for (size_t w = 0; w < data.width; w += 4) {
    for (size_t h = 0; h < data.height; h += 4) {
      PixelBlock4x4 block;
      for (size_t l = 0; l < 4; l++) {
        const size_t addr = (h + l) * data.width + w;
        memcpy(block.data + l * 4, static_cast<char *>(data.data) + addr * 4,
               16);
      }

      block = RGBToHSL(block);

      for (size_t l = 0; l < 4; l++) {
        const size_t addr = (h + l) * data.width + w;
        memcpy(static_cast<char *>(data.data) + addr * 4, block.data + l * 4,
               16);
      }
    }
  }
}

RawImageData GetImageData(BinReaderRef rd, bool isNormalMap) {
  auto &settings = Settings();
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
      break;
    }
    case STBI_grey_alpha: {
      switch (settings.rgType) {
      case RGType::BC7:
        ClampChannels(4);
        break;

      default:
        break;
      }
      break;
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

  /*if (!isNormalMap && settings.rgbType == RGBType::BC1) {
    ConvertToHSL(retVal);
  }*/

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
} // namespace

namespace prime::utils {
void ProcessImage(AppContext *ctx) {
  auto &settings = Settings();
  const bool isNormalMap =
      settings.normalMapPatterns.size() &&
      settings.normalExts.IsFiltered(ctx->workingFile.GetFilename());
  auto rawData = GetImageData(ctx->GetStream(), isNormalMap);
  uint32 inputCrc = 0;
  {
    char buffer[0x4000];
    BinReaderRef str(ctx->GetStream());
    str.BaseStream().clear();
    str.Seek(0);
    const size_t strSize = str.GetSize();
    const size_t numBlocks = strSize / sizeof(buffer);
    const size_t restBlock = strSize % sizeof(buffer);

    for (size_t i = 0; i < numBlocks; i++) {
      str.Read(buffer);
      inputCrc = crc32b(inputCrc, buffer, sizeof(buffer));
    }

    if (restBlock) {
      str.ReadBuffer(buffer, restBlock);
      inputCrc = crc32b(inputCrc, buffer, restBlock);
    }
  }

  auto CountBits = [](size_t x) {
    size_t numBits = 0;

    while (x >>= 1) {
      numBits++;
    }

    return numBits;
  };

  using namespace prime::graphics;

  auto outFile = ctx->workingFile.ChangeExtension2(
      prime::common::GetClassExtension<prime::graphics::TextureStream<0>>());
  size_t currentStream = NUM_STREAMS;
  size_t maxUsedStream = 0;
  BinWritterRef wr;
  utils::ResourceDebugPlayground debugPg;
  debugPg.main->inputCrc = inputCrc;

  auto UpdateStream = [&] {
    const int32 minPixel = std::min(rawData.width, rawData.height);

    for (size_t i = 0; i < NUM_STREAMS; i++) {
      if (minPixel < settings.streamLimit[i]) {
        if (currentStream != i) {
          currentStream = i;
          maxUsedStream = std::max(maxUsedStream, i);
          outFile.back() = '0' + i;
          NewFileContext nCtx = ctx->NewFile(outFile);
          wr = nCtx.str;
          AFileInfo fInf(nCtx.fullPath.data() + nCtx.delimiter);
          debugPg.AddRef(fInf.GetFullPathNoExt(),
                         utils::RedirectTexture({}, i).type);
        }

        break;
      }
    }
  };

  std::vector<TextureEntry> entries;
  uint16 currentTarget = GL_TEXTURE_2D;
  // -1 because we want minimum 4x4 mip
  int16 numMips = CountBits(std::min(rawData.width, rawData.height)) - 1;

  graphics::Texture meta{};
  meta.height = rawData.height;
  meta.width = rawData.width;
  meta.numDims = 2;
  meta.target = currentTarget;
  meta.maxLevel = std::max(numMips - 1, 0);

  TextureFlags metaFlags{};
  struct SwizzleHolder {
    GLenum swizzle[4];
  };
  std::optional<SwizzleHolder> txSwizzle;

  auto WriteTile = [&](uint32 level) {
    UpdateStream();
    rgba_surface surf;
    surf.height = rawData.height;
    surf.width = rawData.width;
    surf.stride = rawData.width * 4;
    surf.ptr = static_cast<uint8_t *>(rawData.data);

    TextureEntry entry{
        .level = uint8(level),
        .streamIndex = uint8(currentStream),
        .target = currentTarget,
        .bufferSize = 0,
        .bufferOffset = wr.Tell(),
    };

    if (rawData.origChannels == STBI_rgb_alpha) {
      metaFlags += TextureFlag::AlphaMasked;

      if (settings.rgbaType == RGBAType::RGBA) {
        meta.format = GL_RGBA;
        meta.internalFormat = GL_RGBA;
        meta.type = GL_UNSIGNED_BYTE;
        entry.bufferSize = rawData.rawSize;
        wr.WriteBuffer(reinterpret_cast<char *>(rawData.data), rawData.rawSize);
      } else if (settings.rgbaType == RGBAType::BC3) {
        metaFlags += TextureFlag::Compressed;
        meta.internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        entry.bufferSize = rawData.bcSize * 16;
        std::string buffer;
        buffer.resize(entry.bufferSize);
        CompressBlocksBC3(&surf, reinterpret_cast<uint8_t *>(buffer.data()));
        wr.WriteContainer(buffer);
      } else if (settings.rgbaType == RGBAType::BC7) {
        metaFlags += TextureFlag::Compressed;
        meta.internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM;
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
        meta.format = GL_RGBA;
        meta.internalFormat = GL_RGBA;
        meta.type = GL_UNSIGNED_BYTE;
        entry.bufferSize = rawData.rawSize;
        wr.WriteBuffer(reinterpret_cast<char *>(rawData.data), rawData.rawSize);
      } else if (settings.rgbType == RGBType::BC1) {
        metaFlags += TextureFlag::Compressed;
        meta.internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        entry.bufferSize = rawData.bcSize * 8;
        std::string buffer;
        buffer.resize(entry.bufferSize);
        CompressBlocksBC1(&surf, reinterpret_cast<uint8_t *>(buffer.data()));
        wr.WriteContainer(buffer);
      } else if (settings.rgbType == RGBType::BC7) {
        metaFlags += TextureFlag::Compressed;
        meta.internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM;
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
        meta.format = GL_RG;
        meta.internalFormat = GL_RG;
        meta.type = GL_UNSIGNED_BYTE;
        entry.bufferSize = rawData.rawSize;
        wr.WriteBuffer(reinterpret_cast<char *>(rawData.data), rawData.rawSize);
      } else if (settings.rgType == RGType::BC5) {
        metaFlags += TextureFlag::Compressed;
        meta.internalFormat = GL_COMPRESSED_RG_RGTC2;
        entry.bufferSize = rawData.bcSize * 16;
        std::string buffer;
        buffer.resize(entry.bufferSize);
        CompressBlocksBC5(&surf, reinterpret_cast<uint8_t *>(buffer.data()));
        wr.WriteContainer(buffer);
      } else if (settings.rgType == RGType::BC7) {
        metaFlags += TextureFlag::Compressed;
        meta.internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM;
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
        meta.format = GL_RED;
        meta.internalFormat = GL_RED;
        meta.type = GL_UNSIGNED_BYTE;
        entry.bufferSize = rawData.rawSize;
        wr.WriteBuffer(reinterpret_cast<char *>(rawData.data), rawData.rawSize);
      } else if (settings.monochromeType == MonochromeType::BC4) {
        metaFlags += TextureFlag::Compressed;
        meta.internalFormat = GL_COMPRESSED_RED_RGTC1;
        entry.bufferSize = rawData.bcSize * 8;
        std::string buffer;
        buffer.resize(entry.bufferSize);
        CompressBlocksBC5(&surf, reinterpret_cast<uint8_t *>(buffer.data()));
        wr.WriteContainer(buffer);
      } else if (settings.monochromeType == MonochromeType::BC7) {
        metaFlags += TextureFlag::Compressed;
        meta.internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM;
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
    metaFlags += TextureFlag::NormalDeriveZAxis;
    metaFlags += TextureFlag::NormalMap;

    TextureEntry entry{};
    entry.target = currentTarget;
    entry.level = level;
    entry.streamIndex = currentStream;
    entry.bufferOffset = wr.Tell();

    if (settings.normalType == NormalType::BC3) {
      metaFlags += TextureFlag::Compressed;
      txSwizzle.emplace(SwizzleHolder{{GL_RED, GL_ALPHA, GL_ONE, GL_ONE}});
      meta.internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
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
      metaFlags += TextureFlag::Compressed;
      metaFlags -= TextureFlag::NormalDeriveZAxis;

      meta.internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM;
      entry.bufferSize = rawData.bcSize * 16;
      std::string buffer;
      buffer.resize(entry.bufferSize);
      bc7_enc_settings prof;
      GetProfile_basic(&prof);
      CompressBlocksBC7(&surf, reinterpret_cast<uint8_t *>(buffer.data()),
                        &prof);
      wr.WriteContainer(buffer);
    } else if (settings.normalType == NormalType::BC5) {
      metaFlags += TextureFlag::Compressed;
      meta.internalFormat = GL_COMPRESSED_RG_RGTC2;
      entry.bufferSize = rawData.bcSize * 16;
      std::string buffer;
      buffer.resize(entry.bufferSize);
      CompressBlocksBC5(&surf, reinterpret_cast<uint8_t *>(buffer.data()));
      wr.WriteContainer(buffer);
    } else if (settings.normalType == NormalType::BC5S) {
      metaFlags += TextureFlag::Compressed;
      meta.internalFormat = GL_COMPRESSED_SIGNED_RG_RGTC2;
      metaFlags += TextureFlag::SignedNormal;
      entry.bufferSize = rawData.bcSize * 16;
      std::string buffer;
      buffer.resize(entry.bufferSize);
      CompressBlocksBC5S(&surf, reinterpret_cast<uint8_t *>(buffer.data()));
      wr.WriteContainer(buffer);
    } else {
      meta.format = GL_RG;
      entry.bufferSize = rawData.rawSize;
      if (settings.normalType == NormalType::RG) {
        meta.internalFormat = GL_RG;
        meta.type = GL_UNSIGNED_BYTE;
        wr.WriteBuffer(reinterpret_cast<char *>(rawData.data), rawData.rawSize);
      } else if (settings.normalType == NormalType::RGS) {
        meta.type = GL_BYTE;
        meta.internalFormat = GL_RG8_SNORM;
        metaFlags += TextureFlag::SignedNormal;

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

  meta.flags = metaFlags;
  meta.numStreams = maxUsedStream + 1;

  utils::PlayGround texturePg;
  utils::PlayGround::Pointer<graphics::Texture> newTexture =
      texturePg.AddClass<graphics::Texture>();
  *newTexture = meta;

  for (auto &entry : entries) {
    texturePg.ArrayEmplace(newTexture->entries, entry);
  }

  if (txSwizzle) {
    for (auto &sw : txSwizzle->swizzle) {
      texturePg.ArrayEmplace(newTexture->swizzle, sw);
    }
  }

  std::string built = debugPg.Build<graphics::Texture>(texturePg);

  outFile = ctx->workingFile.ChangeExtension2(
      prime::common::GetClassExtension<prime::graphics::Texture>());
  BinWritterRef wrh(ctx->NewFile(outFile).str);
  wrh.WriteContainer(built);
}

Reflector *ProcessImageSettings() { return &Settings(); }

std::span<std::string_view> ProcessImageFilters() {
  static std::string_view filters[]{".jpeg$", ".jpg$", ".bmp$", ".psd$",
                                    ".tga$",  ".gif$", ".hdr$", ".pic$",
                                    ".ppm$",  ".pgm$", ".png$"};

  return filters;
}
} // namespace prime::utils
