#include "utils/converters/texture_compiler.hpp"
#include "graphics/detail/texture.hpp"
#include "utils/converters.hpp"
#include "utils/debug.hpp"
#include "utils/texture.hpp"

#include "spike/crypto/crc32.hpp"
#include "spike/io/binreader.hpp"
#include "spike/io/binwritter.hpp"
#include "spike/io/fileinfo.hpp"
#include "spike/io/stat.hpp"
#include "spike/type/vectors_simd.hpp"

#include "ispc_texcomp.h"
#include "stb_image.h"
#include "stb_image_resize.h"

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
    uint32 oldWidth = width;
    uint32 oldHeight = height;
    width /= 2;
    height /= 2;
    rawSize = width * height * numChannels;
    bcSize = (width / 4) * (height / 4);

    uint8 *rData = static_cast<uint8 *>(malloc(rawSize));

    stbir_resize_uint8_generic(                             //
        static_cast<uint8 *>(data), oldWidth, oldHeight, 0, //
        rData, width, height, 0, numChannels,               //
        STBIR_ALPHA_CHANNEL_NONE, 0, stbir_edge::STBIR_EDGE_CLAMP,
        stbir_filter::STBIR_FILTER_DEFAULT,
        stbir_colorspace::STBIR_COLORSPACE_LINEAR, nullptr);
    free(data);
    data = rData;
  }
};

uint32 GetCrc(std::istream &str);

BinWritter NewFile(const std::string &path) {
  std::string absPath = prime::common::CacheDataFolder() + path;
  BinWritter ostr;

  try {
    ostr.Open(absPath);
  } catch (const es::FileInvalidAccessError &) {
    AFileInfo finf(absPath);
    mkdirs(std::string(finf.GetFolder()));
    ostr.Open(absPath);
  }

  prime::common::RegisterResource(path).Unused();

  return ostr;
}

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

    if (desiredChannels == 4 && channels == 1) { // monochrome to rgba
      for (int p = 0; p < x * y; p++) {
        char *destData = nData + (p * desiredChannels);
        char source = data[p];
        destData[0] = source;
        destData[1] = source;
        destData[2] = source;
        destData[3] = -1;
      }
    } else {
      memset(nData, 0, x * y * desiredChannels);
      for (int p = 0; p < x * y; p++) {
        char *destData = nData + (p * desiredChannels);
        memcpy(destData, data + (p * channels), desiredChannels);
        if (compiler.isNormalMap) {
          reinterpret_cast<uint8 &>(destData[1]) =
              0xff - reinterpret_cast<uint8 &>(destData[1]);
        }
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

void SetupGenericFromBest(RawImageData &rawData, graphics::Texture &meta,
                          TextureCompiler &compiler) {
  graphics::TextureFlags &metaFlags = meta.flags;

  if (rawData.origChannels == STBI_rgb_alpha) {
    metaFlags += graphics::TextureFlag::AlphaMasked;
    if (compiler.rgbaType == TextureCompilerRGBAType::RGBA) {
      meta.format = GL_RGBA;
      meta.internalFormat = GL_RGBA;
      meta.type = GL_UNSIGNED_BYTE;
    } else if (compiler.rgbaType == TextureCompilerRGBAType::BC1) {
      metaFlags += graphics::TextureFlag::Compressed;
      meta.internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
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
    } else if (compiler.rgbaType == TextureCompilerRGBAType::BC1) {
      uint32 bufferSize = rawData.bcSize * 8;
      std::string buffer;
      buffer.resize(bufferSize);
      CompressBlocksBC1(&surf, reinterpret_cast<uint8_t *>(buffer.data()));
      wr.WriteContainer(buffer);
      return bufferSize;
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
      surf.stride = rawData.width;
      uint32 bufferSize = rawData.bcSize * 8;
      std::string buffer;
      buffer.resize(bufferSize);
      CompressBlocksBC4(&surf, reinterpret_cast<uint8_t *>(buffer.data()));
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

  __builtin_unreachable();
}

void SetupNormalFromBest(PlayGround::Pointer<graphics::Texture> meta,
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

common::Return<void> Compile(std::string buffer, std::string_view output) {
  ResourceDebugPlayground debugPg;
  PlayGround::Pointer<TextureCompiler> compiler(
      debugPg.NewBytes<TextureCompiler>(buffer.data(), buffer.size()));
  debugPg.Link(debugPg.main->converter,
               static_cast<common::ResourceBase *>(compiler.operator->()));
  ResourceDebug *compDebug = LocateDebug(buffer.data(), buffer.size());
  debugPg.main->inputCrc = compDebug->inputCrc;

  // check non zero entries
  const uint32 numSlices = compiler->entries[0].paths.numItems;
  if (numSlices == 0) {
    return {RUNTIME_ERROR("All entries for TextureCompiler must have non zero "
                          "number of slices.")};
  }
  const bool isGeneric =
      compiler->entries[0].type == TextureCompilerEntryType::Mipmap;

  for (uint32 i = 0; i < compiler->entries.numItems; i++) {
    debugPg.ArrayEmplaceBytes(reinterpret_cast<common::LocalArray32<char> &>(
                                  compiler->entries[i].crcs),
                              4 * numSlices, 4);
  }

  TextureCompilerEntry *slotEntries[16][7]{};

  for (TextureCompilerEntry &e : compiler->entries) {
    if (e.paths.numItems != numSlices) {
      return {RUNTIME_ERROR("All entries for TextureCompiler must have same "
                            "number of slices across all levels.")};
    }

    if (e.type != TextureCompilerEntryType::Mipmap && isGeneric) {
      return {RUNTIME_ERROR("Entry types for TextureCompiler cannot mix "
                            "cubemap faces with generic.")};
    }

    slotEntries[e.level][int(e.type)] = &e;
  }

  auto CountBits = [](size_t x) {
    size_t numBits = 0;

    while (x >>= 1) {
      numBits++;
    }

    return numBits;
  };

  PlayGround texturePg;
  PlayGround::Pointer<graphics::Texture> metap =
      texturePg.AddClass<graphics::Texture>();

  auto SetupTexture = [&](RawImageData &rawData) {
    if (metap->width > 0) {
      return;
    }

    // -1 because we want minimum 4x4 mip
    int16 numMips = CountBits(std::min(rawData.width, rawData.height)) - 1;

    {
      graphics::Texture &meta = *metap;
      meta.height = rawData.height;
      meta.width = rawData.width;
      meta.depth = numSlices;
      meta.numDims = 2 + (compiler->isVolumetric && numSlices > 1);
      meta.maxLevel = std::max((numMips - 1) * compiler->generateMipmaps, 0);

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

    if (compiler->isNormalMap) {
      SetupNormalFromBest(metap, *compiler);
    } else {
      SetupGenericFromBest(rawData, *metap, *compiler);
    }
  };

  std::optional<BinWritter> streams[compiler->NUM_STREAMS];
  std::string outFile(output);
  outFile.append(common::GetClassExtension<graphics::TextureStream<0>>());
  uint32 maxUsedStream = 0;

  auto GetStream = [&](RawImageData &rawData) -> uint32 {
    const uint32 minPixel = std::min(rawData.width, rawData.height);

    for (uint32 i = 0; i < compiler->NUM_STREAMS; i++) {
      if (minPixel < uint32(compiler->streamLimit[i])) {
        auto &s = streams[i];

        if (!s) {
          maxUsedStream = std::max(maxUsedStream, i);
          outFile.back() = '0' + i;
          s = NewFile(outFile);

          AFileInfo fInf(outFile);
          debugPg.AddRef(fInf.GetFullPathNoExt(),
                         utils::RedirectTexture({}, i).type);
        }

        return i;
      }
    }

    __builtin_unreachable();
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

    __builtin_unreachable();
    return GL_NONE;
  };

  auto ConvertSlice = [&](BinWritterRef wr, RawImageData &r) {
    if (compiler->isNormalMap) {
      return ConvertNormalSlice(wr, *compiler, r);
    }

    return ConvertGenericSlice(wr, *compiler, r);
  };

  std::vector<RawImageData> rawData[7]{};

  for (auto &r : rawData) {
    r.resize(numSlices);
  }

  for (uint32 lid = 0; auto &l : slotEntries) {
    for (uint32 fid = 0; auto &f : l) {
      // next level is not provided, but first is, resize internally
      if (compiler->generateMipmaps && lid <= metap->maxLevel && lid > 0 && !f && slotEntries[0][fid]) {
        graphics::TextureEntry entry{
            .level = uint8(lid),
            .streamIndex = 0,
            .target = TargetFromType(slotEntries[0][fid]->type),
            .bufferSize = 0,
            .bufferOffset = 0,
        };

        for (auto &r : rawData[fid]) {
          r.MipMap();
          const uint32 strIndex = GetStream(r);
          BinWritterRef wr(*streams[strIndex]);

          if (!entry.bufferSize) {
            entry.bufferOffset = wr.Tell();
            entry.streamIndex = strIndex;
          }

          entry.bufferSize += ConvertSlice(wr, r);
        }

        texturePg.ArrayEmplace(metap->entries, entry);
      }

      if (f) {
        graphics::TextureEntry entry{
            .level = uint8(lid),
            .streamIndex = 0,
            .target = 0,
            .bufferSize = 0,
            .bufferOffset = 0,
        };

        for (uint32 p = 0; p < numSlices; p++) {
          AFileInfo iFile(f->paths[p]);
          if (iFile.GetFullPath().size() > 0) {
            common::ResourceHash hs(JenkinsHash3_(iFile.GetFullPathNoExt()), 0);
            common::ResourcePath foundPath(common::FindResource(hs));
            BinReader rd(std::string(foundPath.workingDir) +
                         foundPath.localPath);
            f->crcs[p] = GetCrc(rd.BaseStream());
            RawImageData &rData = rawData[fid][p];

            if (rData.data) {
              free(rData.data);
            }

            rawData[fid][p] = GetImageData(rd.BaseStream(), *compiler);
          } else {
            rawData[fid][p].MipMap();
          }

          RawImageData &rData = rawData[fid][p];
          const uint32 strIndex = GetStream(rData);
          BinWritterRef wr(*streams[strIndex]);
          SetupTexture(rData);

          if (!entry.bufferSize) {
            entry.target = TargetFromType(f->type);
            entry.bufferOffset = wr.Tell();
            entry.streamIndex = strIndex;
          }

          entry.bufferSize += ConvertSlice(wr, rData);
        }

        texturePg.ArrayEmplace(metap->entries, entry);
      }

      fid++;
    }

    lid++;
  }

  std::sort(metap->entries.begin(), metap->entries.end(),
            [](const prime::graphics::TextureEntry &i1,
               const prime::graphics::TextureEntry &i2) {
              if (i1.streamIndex == i2.streamIndex) {
                return i1.level > i2.level;
              }

              return i1.streamIndex < i2.streamIndex;
            });

  metap->numStreams = maxUsedStream + 1;

  std::string built =
      debugPg.Build(texturePg, reflect::GetReflectedClass<graphics::Texture>());
  outFile = output;
  outFile.append(common::GetClassExtension<graphics::Texture>());
  NewFile(outFile).WriteContainer(built);

  return {NO_ERROR};
}
} // namespace prime::utils

REGISTER_CLASS(prime::utils::TextureCompiler);
REGISTER_COMPILER(prime::utils::TextureCompiler, prime::utils::Compile);
