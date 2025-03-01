#include "utils/converters/texture_compiler.hpp"
#include "graphics/detail/texture.hpp"
#include "ispc_texcomp.h"
#include "spike/io/binreader.hpp"
#include "spike/io/binwritter.hpp"
#include "spike/io/fileinfo.hpp"
#include "spike/type/vectors_simd.hpp"
#include "stb_image.h"
#include "stb_image_resize.h"
#include "utils/debug.hpp"
#include "utils/texture.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

#include <optional>
#include <vector>

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

namespace prime::utils {

RawImageData GetImageData(BinReaderRef rd, TextureCompiler &compiler) {
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
    // printwarning("Image is not power of two, resizing.");
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
      if (compiler.isNormalMap) {
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

  if (compiler.isNormalMap) {
    // todo monochrome, height > normal
    switch (compiler.normalType) {
    case TextureCompilerNormalType::RG:
    case TextureCompilerNormalType::RGS:
    case TextureCompilerNormalType::BC5:
    case TextureCompilerNormalType::BC5S:
      if (channels > 2) {
        ClampChannels(2);
      }
      break;

    case TextureCompilerNormalType::BC3:
      if (channels < 4) {
        ClampChannels(4);
      }
      break;

    case TextureCompilerNormalType::BC7:
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
      switch (compiler.rgbType) {
      case TextureCompilerRGBType::BC1:
      case TextureCompilerRGBType::RGBX:
      case TextureCompilerRGBType::BC7:
        ClampChannels(4);
        break;

      default:
        break;
      }
      break;
    }
    case STBI_grey_alpha: {
      switch (compiler.rgType) {
      case TextureCompilerRGType::BC7:
        ClampChannels(4);
        break;

      default:
        break;
      }
      break;
    }
    case STBI_grey: {
      switch (compiler.monochromeType) {
      case TextureCompilerMonochromeType::BC7:
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
}

void SetupGenericFromBest(RawImageData &rawData, graphics::Texture &meta,
                          TextureCompiler &compiler) {
  graphics::TextureFlags &metaFlags = meta.flags;

  if (rawData.origChannels == STBI_rgb_alpha) {
    metaFlags += graphics::TextureFlag::AlphaMasked;
    if (compiler.rgbaType == TextureCompilerRGBAType::RGBA) {
      meta.format = GL_RGBA;
      meta.internalFormat = GL_RGBA;
      meta.type = GL_UNSIGNED_BYTE;
    } else if (compiler.rgbaType == TextureCompilerRGBAType::BC3) {
      metaFlags += graphics::TextureFlag::Compressed;
      meta.internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    } else if (compiler.rgbaType == TextureCompilerRGBAType::BC7) {
      metaFlags += graphics::TextureFlag::Compressed;
      meta.internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM;
    }
  } else if (rawData.origChannels == STBI_rgb) {
    if (compiler.rgbType == TextureCompilerRGBType::RGBX) {
      meta.format = GL_RGBA;
      meta.internalFormat = GL_RGBA;
      meta.type = GL_UNSIGNED_BYTE;
    } else if (compiler.rgbType == TextureCompilerRGBType::BC1) {
      metaFlags += graphics::TextureFlag::Compressed;
      meta.internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
    } else if (compiler.rgbType == TextureCompilerRGBType::BC7) {
      metaFlags += graphics::TextureFlag::Compressed;
      meta.internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM;
    }
  } else if (rawData.origChannels == STBI_grey_alpha) {
    if (compiler.rgType == TextureCompilerRGType::RG) {
      meta.format = GL_RG;
      meta.internalFormat = GL_RG;
      meta.type = GL_UNSIGNED_BYTE;
    } else if (compiler.rgType == TextureCompilerRGType::BC5) {
      metaFlags += graphics::TextureFlag::Compressed;
      meta.internalFormat = GL_COMPRESSED_RG_RGTC2;
    } else if (compiler.rgType == TextureCompilerRGType::BC7) {
      metaFlags += graphics::TextureFlag::Compressed;
      meta.internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM;
    }
  } else if (rawData.origChannels == STBI_grey) {
    if (compiler.monochromeType == TextureCompilerMonochromeType::Monochrome) {
      meta.format = GL_RED;
      meta.internalFormat = GL_RED;
      meta.type = GL_UNSIGNED_BYTE;
    } else if (compiler.monochromeType == TextureCompilerMonochromeType::BC4) {
      metaFlags += graphics::TextureFlag::Compressed;
      meta.internalFormat = GL_COMPRESSED_RED_RGTC1;
    } else if (compiler.monochromeType == TextureCompilerMonochromeType::BC7) {
      metaFlags += graphics::TextureFlag::Compressed;
      meta.internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM;
    }
  }
}

uint32 ConvertGenericSlice(BinWritterRef wr, TextureCompiler &compiler,
                           RawImageData &rawData) {
  rgba_surface surf;
  surf.height = rawData.height;
  surf.width = rawData.width;
  surf.stride = rawData.width * 4;
  surf.ptr = static_cast<uint8_t *>(rawData.data);

  if (rawData.origChannels == STBI_rgb_alpha) {
    if (compiler.rgbaType == TextureCompilerRGBAType::RGBA) {
      wr.WriteBuffer(reinterpret_cast<char *>(rawData.data), rawData.rawSize);
      return rawData.rawSize;
    } else if (compiler.rgbaType == TextureCompilerRGBAType::BC3) {
      uint32 bufferSize = rawData.bcSize * 16;
      std::string buffer;
      buffer.resize(bufferSize);
      CompressBlocksBC3(&surf, reinterpret_cast<uint8_t *>(buffer.data()));
      wr.WriteContainer(buffer);
      return bufferSize;
    } else if (compiler.rgbaType == TextureCompilerRGBAType::BC7) {
      uint32 bufferSize = rawData.bcSize * 16;
      std::string buffer;
      buffer.resize(bufferSize);
      bc7_enc_settings prof;
      GetProfile_alpha_basic(&prof);
      CompressBlocksBC7(&surf, reinterpret_cast<uint8_t *>(buffer.data()),
                        &prof);
      wr.WriteContainer(buffer);
      return bufferSize;
    }
  } else if (rawData.origChannels == STBI_rgb) {
    if (compiler.rgbType == TextureCompilerRGBType::RGBX) {
      wr.WriteBuffer(reinterpret_cast<char *>(rawData.data), rawData.rawSize);
      return rawData.rawSize;
    } else if (compiler.rgbType == TextureCompilerRGBType::BC1) {
      uint32 bufferSize = rawData.bcSize * 8;
      std::string buffer;
      buffer.resize(bufferSize);
      CompressBlocksBC1(&surf, reinterpret_cast<uint8_t *>(buffer.data()));
      wr.WriteContainer(buffer);
      return bufferSize;
    } else if (compiler.rgbType == TextureCompilerRGBType::BC7) {
      uint32 bufferSize = rawData.bcSize * 16;
      std::string buffer;
      buffer.resize(bufferSize);
      bc7_enc_settings prof;
      GetProfile_basic(&prof);
      CompressBlocksBC7(&surf, reinterpret_cast<uint8_t *>(buffer.data()),
                        &prof);
      wr.WriteContainer(buffer);
      return bufferSize;
    }
  } else if (rawData.origChannels == STBI_grey_alpha) {
    if (compiler.rgType == TextureCompilerRGType::RG) {
      wr.WriteBuffer(reinterpret_cast<char *>(rawData.data), rawData.rawSize);
      return rawData.rawSize;
    } else if (compiler.rgType == TextureCompilerRGType::BC5) {
      uint32 bufferSize = rawData.bcSize * 16;
      std::string buffer;
      buffer.resize(bufferSize);
      CompressBlocksBC5(&surf, reinterpret_cast<uint8_t *>(buffer.data()));
      wr.WriteContainer(buffer);
      return bufferSize;
    } else if (compiler.rgType == TextureCompilerRGType::BC7) {
      uint32 bufferSize = rawData.bcSize * 16;
      std::string buffer;
      buffer.resize(bufferSize);
      bc7_enc_settings prof;
      GetProfile_basic(&prof);
      CompressBlocksBC7(&surf, reinterpret_cast<uint8_t *>(buffer.data()),
                        &prof);
      wr.WriteContainer(buffer);
      return bufferSize;
    }
  } else if (rawData.origChannels == STBI_grey) {
    if (compiler.monochromeType == TextureCompilerMonochromeType::Monochrome) {
      wr.WriteBuffer(reinterpret_cast<char *>(rawData.data), rawData.rawSize);
      return rawData.rawSize;
    } else if (compiler.monochromeType == TextureCompilerMonochromeType::BC4) {
      uint32 bufferSize = rawData.bcSize * 8;
      std::string buffer;
      buffer.resize(bufferSize);
      CompressBlocksBC5(&surf, reinterpret_cast<uint8_t *>(buffer.data()));
      wr.WriteContainer(buffer);
      return bufferSize;
    } else if (compiler.monochromeType == TextureCompilerMonochromeType::BC7) {
      uint32 bufferSize = rawData.bcSize * 16;
      std::string buffer;
      buffer.resize(bufferSize);
      bc7_enc_settings prof;
      GetProfile_basic(&prof);
      CompressBlocksBC7(&surf, reinterpret_cast<uint8_t *>(buffer.data()),
                        &prof);
      wr.WriteContainer(buffer);
      return bufferSize;
    }
  }
}

void SetupNormalFromBest(RawImageData &rawData,
                         PlayGround::Pointer<graphics::Texture> meta,
                         TextureCompiler &compiler) {
  graphics::TextureFlags &metaFlags = meta->flags;

  metaFlags += graphics::TextureFlag::NormalDeriveZAxis;
  metaFlags += graphics::TextureFlag::NormalMap;

  if (compiler.normalType == TextureCompilerNormalType::BC3) {
    metaFlags += graphics::TextureFlag::Compressed;
    meta.owner->ArrayEmplace(meta->swizzle, GL_RED);
    meta.owner->ArrayEmplace(meta->swizzle, GL_ALPHA);
    meta.owner->ArrayEmplace(meta->swizzle, GL_ONE);
    meta.owner->ArrayEmplace(meta->swizzle, GL_ONE);
    meta->internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
  } else if (compiler.normalType == TextureCompilerNormalType::BC7) {
    metaFlags += graphics::TextureFlag::Compressed;
    metaFlags -= graphics::TextureFlag::NormalDeriveZAxis;
    meta->internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM;
  } else if (compiler.normalType == TextureCompilerNormalType::BC5) {
    metaFlags += graphics::TextureFlag::Compressed;
    meta->internalFormat = GL_COMPRESSED_RG_RGTC2;
  } else if (compiler.normalType == TextureCompilerNormalType::BC5S) {
    metaFlags += graphics::TextureFlag::Compressed;
    meta->internalFormat = GL_COMPRESSED_SIGNED_RG_RGTC2;
    metaFlags += graphics::TextureFlag::SignedNormal;
  } else {
    meta->format = GL_RG;

    if (compiler.normalType == TextureCompilerNormalType::RG) {
      meta->internalFormat = GL_RG;
      meta->type = GL_UNSIGNED_BYTE;
    } else if (compiler.normalType == TextureCompilerNormalType::RGS) {
      meta->type = GL_BYTE;
      meta->internalFormat = GL_RG8_SNORM;
      metaFlags += graphics::TextureFlag::SignedNormal;
    }
  }
}

uint32 ConvertNormalSlice(BinWritterRef wr, TextureCompiler &compiler,
                          RawImageData &rawData) {
  rgba_surface surf;
  surf.height = rawData.height;
  surf.width = rawData.width;
  surf.stride = rawData.width * rawData.numChannels;
  surf.ptr = static_cast<uint8_t *>(rawData.data);

  if (compiler.normalType == TextureCompilerNormalType::BC3) {
    uint32 bufferSize = rawData.bcSize * 16;
    std::string buffer;
    buffer.resize(bufferSize);

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
    return bufferSize;
  } else if (compiler.normalType == TextureCompilerNormalType::BC7) {
    uint32 bufferSize = rawData.bcSize * 16;
    std::string buffer;
    buffer.resize(bufferSize);
    bc7_enc_settings prof;
    GetProfile_basic(&prof);
    CompressBlocksBC7(&surf, reinterpret_cast<uint8_t *>(buffer.data()), &prof);
    wr.WriteContainer(buffer);
    return bufferSize;
  } else if (compiler.normalType == TextureCompilerNormalType::BC5) {
    uint32 bufferSize = rawData.bcSize * 16;
    std::string buffer;
    buffer.resize(bufferSize);
    CompressBlocksBC5(&surf, reinterpret_cast<uint8_t *>(buffer.data()));
    wr.WriteContainer(buffer);
    return bufferSize;
  } else if (compiler.normalType == TextureCompilerNormalType::BC5S) {
    uint32 bufferSize = rawData.bcSize * 16;
    std::string buffer;
    buffer.resize(bufferSize);
    CompressBlocksBC5S(&surf, reinterpret_cast<uint8_t *>(buffer.data()));
    wr.WriteContainer(buffer);
    return bufferSize;
  } else {

    if (compiler.normalType == TextureCompilerNormalType::RG) {
      wr.WriteBuffer(reinterpret_cast<char *>(rawData.data), rawData.rawSize);
    } else if (compiler.normalType == TextureCompilerNormalType::RGS) {

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

    return rawData.rawSize;
  }
};

void Compile(std::string buffer, std::string_view output) {
  TextureCompiler &compiler =
      reinterpret_cast<TextureCompiler &>(*buffer.data());
  // check non zero entries
  const uint32 numSlices = compiler.entries[0].paths.numItems;
  const bool isGeneric =
      compiler.entries[0].type == TextureCompilerEntryType::Mipmap;
  TextureCompilerEntry *bestEntry = nullptr;

  struct EntryMatrix {
    std::vector<bool> slices;
    uint32 numChannels;
    uint32 width;
    uint32 height;
  };

  EntryMatrix entries[16]{};

  for (TextureCompilerEntry &e : compiler.entries) {
    if (e.paths.numItems != numSlices) {
      // todo error
    }

    if (e.type != TextureCompilerEntryType::Mipmap && isGeneric) {
      // todo error
    }

    if (!bestEntry && e.level == 0) {
      bestEntry = &e;
    }

    entries[e.level].slices.resize(e.paths.numItems);
  }

  BinReader rd(std::string(bestEntry->paths[0]));
  RawImageData rawData = GetImageData(rd.BaseStream(), compiler);

  auto CountBits = [](size_t x) {
    size_t numBits = 0;

    while (x >>= 1) {
      numBits++;
    }

    return numBits;
  };

  // -1 because we want minimum 4x4 mip
  int16 numMips = CountBits(std::min(rawData.width, rawData.height)) - 1;

  PlayGround texturePg;
  PlayGround::Pointer<graphics::Texture> metap =
      texturePg.AddClass<graphics::Texture>();
  {
    graphics::Texture &meta = *metap;
    meta.height = rawData.height;
    meta.width = rawData.width;
    meta.depth = numSlices;
    meta.numDims = 2 + (compiler.isVolumetric && numSlices > 1);
    meta.maxLevel = std::max(numMips - 1, 0);

    if (meta.numDims > 2) {
      meta.flags += graphics::TextureFlag::Volume;
      meta.target = GL_TEXTURE_3D;
    } else if (numSlices > 1) {
      meta.flags += graphics::TextureFlag::Array;

      if (isGeneric) {
        meta.target = GL_TEXTURE_2D_ARRAY;
      } else {
        meta.target = GL_TEXTURE_CUBE_MAP_ARRAY;
      }
    } else if (isGeneric) {
      meta.target = GL_TEXTURE_2D;
    } else {
      meta.target = GL_TEXTURE_CUBE_MAP;
    }
  }

  if (compiler.isNormalMap) {
    SetupNormalFromBest(rawData, metap, compiler);
  } else {
    SetupGenericFromBest(rawData, *metap, compiler);
  }

  std::optional<BinWritter> streams[compiler.NUM_STREAMS];
  std::string outFile; // todo
  std::string outFolder;

  utils::ResourceDebugPlayground debugPg;

  auto GetStream = [&](RawImageData &rawData) -> uint32 {
    const int32 minPixel = std::min(rawData.width, rawData.height);

    for (uint32 i = 0; i < compiler.NUM_STREAMS; i++) {
      if (minPixel < compiler.streamLimit[i]) {
        auto &s = streams[i];

        if (!s) {
          outFile.back() = '0' + i;
          s = BinWritter(outFolder + outFile);

          AFileInfo fInf(outFile);
          debugPg.AddRef(fInf.GetFullPathNoExt(),
                         utils::RedirectTexture({}, i).type);
        }

        return i;
      }
    }

    // should not reach
  };

  auto TargetFromType = [&metap](TextureCompilerEntryType type) -> uint16 {
    switch (type) {
    case TextureCompilerEntryType::Mipmap:
      return metap->target;
    case TextureCompilerEntryType::FrontFace:
      return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
    case TextureCompilerEntryType::BackFace:
      return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
    case TextureCompilerEntryType::RightFace:
      return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
    case TextureCompilerEntryType::LeftFace:
      return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
    case TextureCompilerEntryType::TopFace:
      return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
    case TextureCompilerEntryType::BottomFace:
      return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
    }
  };

  if (compiler.isNormalMap) {
    const uint32 strIndex = GetStream(rawData);
    BinWritterRef wr(*streams[strIndex]);

    graphics::TextureEntry entry{
        .level = bestEntry->level,
        .streamIndex = uint8(strIndex),
        .target = TargetFromType(bestEntry->type),
        .bufferSize = 0,
        .bufferOffset = wr.Tell(),
    };

    entry.bufferSize = ConvertNormalSlice(wr, compiler, rawData);

    for (uint32 i = 1; i < bestEntry->paths.numItems; i++) {
      free(rawData.data);
      BinReader rd(std::string(bestEntry->paths[i]));
      rawData = GetImageData(rd.BaseStream(), compiler);
      entry.bufferSize += ConvertNormalSlice(wr, compiler, rawData);
    }
  } else {
  }
}
} // namespace prime::utils
