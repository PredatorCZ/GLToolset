

enum TEXFlag : uint16 {
  Cubemap,
  Array,
  Volume,
  Compressed,
  AlphaMasked,
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


struct NewTexture {
  uint32 object;
  TEX hdr;
};

int max_aniso;

NewTexture MakeTexture(std::string fileName) {
  unsigned int texture;
  glGenTextures(1, &texture);

  TEX hdr;
  std::vector<TEXEntry> entries;
  std::string buffer;

  {
    BinReader rd(fileName + ".gth");
    rd.Read(hdr);
    rd.ReadContainer(entries, hdr.numEnries);
  }
  glBindTexture(hdr.target, texture);
  glTexParameteri(hdr.target, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(hdr.target, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(hdr.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
  glTexParameteri(hdr.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(hdr.target, GL_TEXTURE_BASE_LEVEL, 0);
  glTexParameteri(hdr.target, GL_TEXTURE_MAX_LEVEL, hdr.maxLevel);
  glTexParameterf(hdr.target, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);

  if (hdr.flags == TEXFlag::Swizzle) {
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, hdr.swizzleMask);
  }

  {
    BinReader rd(fileName + ".gtb");
    rd.ReadContainer(buffer, hdr.dataSize);
  }

  if (hdr.flags == TEXFlag::Compressed) {
    if (hdr.flags == TEXFlag::Volume) {
      // not implemented
    } else if (hdr.flags == TEXFlag::Array) {
      // not implemented
    } else {
      if (hdr.numDims == 1) {
        for (auto &e : entries) {
          uint32_t width = hdr.width >> e.level;
          glCompressedTexImage1D(e.target, e.level, hdr.internalFormat, width,
                                 0, e.bufferSize,
                                 buffer.data() + e.bufferOffset);
        }
      } else if (hdr.numDims == 2) {
        for (auto &e : entries) {
          uint32_t height = hdr.height >> e.level;
          uint32_t width = hdr.width >> e.level;
          glCompressedTexImage2D(e.target, e.level, hdr.internalFormat, width,
                                 height, 0, e.bufferSize,
                                 buffer.data() + e.bufferOffset);
        }
      }
    }
  } else {
    if (hdr.flags == TEXFlag::Volume) {
      // not implemented
    } else if (hdr.flags == TEXFlag::Array) {
      // not implemented
    } else {
      if (hdr.numDims == 1) {
        for (auto &e : entries) {
          uint32_t width = hdr.width >> e.level;
          glTexImage1D(e.target, e.level, hdr.internalFormat, width, 0,
                       hdr.format, hdr.type, buffer.data() + e.bufferOffset);
        }
      } else if (hdr.numDims == 2) {
        for (auto &e : entries) {
          uint32_t height = hdr.height >> e.level;
          uint32_t width = hdr.width >> e.level;
          glTexImage2D(e.target, e.level, hdr.internalFormat, width, height, 0,
                       hdr.format, hdr.type, buffer.data() + e.bufferOffset);
        }
      }
    }
  }

  return {texture, hdr};
};