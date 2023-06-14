/*  PrimeCache
    Copyright(C) 2023 Lukas Cone

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

#include "common/array.hpp"
#include "common/core.hpp"
#include "common/resource.hpp"
#include "datas/app_context.hpp"
#include "datas/binreader.hpp"
#include "datas/binwritter.hpp"
#include "datas/crc32.hpp"
#include "datas/stat.hpp"
#include "project.h"
#include <map>
#include <mutex>
#include <set>
#include <zstd.h>

static AppInfo_s appInfo{
    .header = PrimeCache_DESC " v" PrimeCache_VERSION ", " PrimeCache_COPYRIGHT
                              "Lukas Cone",
};

AppInfo_s *AppInitModule() { return &appInfo; }

extern std::map<uint32, std::string (*)(BinReaderRef)> STRIPPERS;

struct SubStream {
  size_t offset;
  uint32 size;
};

struct Stream {
  std::string streamPath;
  BinWritter_t<BinCoreOpenMode::NoBuffer> streamStore;
  size_t blobalOffset = 0;
  std::mutex mtx;

  Stream(std::string &&path)
      : streamPath(std::move(path)), streamStore(streamPath) {}

  SubStream SendStream(std::istream &str, uint32 clHash ) {
    if (STRIPPERS.contains(clHash)) {
      auto stripped = STRIPPERS.at(clHash)(str);
      std::lock_guard lg(mtx);
      SubStream retVal;
      retVal.offset = streamStore.Tell();
      retVal.size = stripped.size();
      streamStore.WriteContainer(stripped);
      return retVal;
    }

    std::lock_guard lg(mtx);
    const size_t inputSize = BinReaderRef(str).GetSize();
    char buffer[0x10000];
    const size_t numChunks = inputSize / sizeof(buffer);
    const size_t restBytes = inputSize % sizeof(buffer);
    SubStream retVal;
    retVal.offset = streamStore.Tell();

    for (size_t i = 0; i < numChunks; i++) {
      str.read(buffer, sizeof(buffer));
      streamStore.WriteBuffer(buffer, sizeof(buffer));
    }

    if (restBytes) {
      str.read(buffer, restBytes);
      streamStore.WriteBuffer(buffer, restBytes);
    }

    retVal.size = streamStore.Tell() - retVal.offset;

    return retVal;
  }
};

struct IFile {
  prime::common::ResourceHash info;
  SubStream location;

  bool operator<(const IFile &o) const { return info < o.info; }
};

namespace prime::common {
struct CacheFile;
struct CacheBlock;
struct Cache;
} // namespace prime::common

HASH_CLASS(prime::common::CacheFile);
HASH_CLASS(prime::common::CacheBlock);
CLASS_EXT(prime::common::Cache);

namespace prime::common {
struct CacheFile {
  prime::common::ResourceHash info;
  uint64 blockOffset : 20; // 255GiB
  uint64 microOffset : 18; // BLOCK_SIZE
  uint64 tailSize : 18;    // BLOCK_SIZE
  uint64 numBlocks : 8;    // 64MiB
};

static_assert(sizeof(CacheFile) == 16);

struct CacheBlock {
  uint64 offset;
  uint32 size;
  uint32 crc;
};

struct Cache : Resource {
  Cache() : CLASS_VERSION(1) {}
  LocalArray32<CacheFile> files;
  LocalArray32<CacheBlock> blocks;
};
} // namespace prime::common

struct MakeContext : AppPackContext {
  std::string baseFile;
  std::map<uint32, Stream> streams;
  std::mutex mtx;
  std::vector<IFile> files;
  std::set<std::string> fileNames;

  MakeContext(std::string baseFile_, const AppPackStats &)
      : baseFile(std::move(baseFile_)) {}

  void SendFile(std::string_view path, std::istream &stream) override {
    auto dot = path.find_last_of('.');

    if (dot == path.npos) {
      throw std::runtime_error("File not supported.");
    }

    uint32 clHash = 0;
    auto ext = path.substr(dot + 1);

    if (ext == "refs") {
      return;
    }

    try {
      clHash = prime::common::GetClassFromExtension(ext);
    } catch (...) {
      throw std::runtime_error("File not supported.");
    }

    if (!streams.contains(clHash)) {
      std::lock_guard lg(mtx);
      streams.emplace(clHash, baseFile + std::string(ext));
    }

    IFile curFile;
    curFile.info.type = clHash;
    curFile.info.name = JenkinsHash3_(path.substr(0, dot));
    curFile.location = streams.at(clHash).SendStream(stream, clHash);

    {
      std::lock_guard lg(mtx);
      files.emplace_back(curFile);
      fileNames.emplace(path.substr(0, dot));
    }
  }

  void Finish() override {
    namespace pc = prime::common;
    {
      BinWritter refs(baseFile + ".refs");
      for (auto &f : fileNames) {
        refs.WriteContainer(f);
        refs.Write('\n');
      }
    }

    BinWritter outIdx(baseFile + "." +
                      std::string(pc::GetClassExtension<pc::Cache>()));
    BinWritter_t<BinCoreOpenMode::NoBuffer> outData(baseFile + ".dat");
    std::sort(files.begin(), files.end());

    pc::Cache outCache;
    outCache.files.numItems = files.size();
    outIdx.Write(outCache);
    outIdx.ApplyPadding(alignof(pc::CacheBlock));
    outCache.blocks.pointer =
        outIdx.Tell() - offsetof(pc::Cache, blocks.pointer);

    constexpr size_t BLOCK_SIZE = 0x40000;
    char block[BLOCK_SIZE]{};
    size_t blockAvail = BLOCK_SIZE;

    ZSTD_CCtx *zctx = ZSTD_createCCtx();
    std::string zBuffer;
    zBuffer.resize(ZSTD_compressBound(BLOCK_SIZE));

    for (auto &[clHash, stream] : streams) {
      es::Dispose(stream.streamStore);
      stream.blobalOffset = outData.Tell();
      BinReader rd(stream.streamPath);
      size_t streamSize = rd.GetSize();

      while (streamSize) {
        size_t blockFill = std::min(blockAvail, streamSize);
        streamSize -= blockFill;
        rd.ReadBuffer(block + (BLOCK_SIZE - blockAvail), blockFill);
        blockAvail -= blockFill;

        if (blockAvail == 0) {
          outCache.blocks.numItems++;
          const uint32 cSize =
              ZSTD_compressCCtx(zctx, zBuffer.data(), zBuffer.size(), block,
                                BLOCK_SIZE, ZSTD_btultra);

          pc::CacheBlock nBlock{
              .offset = outData.Tell(),
              .size = cSize,
              .crc = crc32b(0, zBuffer.data(), cSize),
          };

          outIdx.Write(nBlock);
          outData.WriteBuffer(zBuffer.data(), cSize);
          blockAvail = BLOCK_SIZE;
        }
      }

      es::Dispose(rd);
      es::RemoveFile(stream.streamPath);
    }

    if (blockAvail != BLOCK_SIZE) {

      outCache.blocks.numItems++;
      const uint32 cSize =
          ZSTD_compressCCtx(zctx, zBuffer.data(), zBuffer.size(), block,
                            BLOCK_SIZE - blockAvail, ZSTD_btultra);
      pc::CacheBlock nBlock{
          .offset = outData.Tell(),
          .size = cSize,
          .crc = crc32b(0, zBuffer.data(), cSize),
      };

      outIdx.Write(nBlock);
      outData.WriteBuffer(zBuffer.data(), cSize);
    }

    outIdx.ApplyPadding(alignof(pc::CacheFile));

    outCache.files.pointer = outIdx.Tell() - offsetof(pc::Cache, files.pointer);

    for (auto &f : files) {
      const size_t globOffset = streams.at(f.info.type).blobalOffset;
      pc::CacheFile file{
          .info = f.info,
          .blockOffset = uint32((globOffset + f.location.offset) / BLOCK_SIZE),
          .microOffset = uint32((globOffset + f.location.offset) % BLOCK_SIZE),
          .tailSize = f.location.size % BLOCK_SIZE,
          .numBlocks = f.location.size / BLOCK_SIZE,
      };

      outIdx.Write(file);
    }

    outIdx.Seek(0);
    outIdx.Write(outCache);
  }
};

AppPackContext *AppNewArchive(const std::string &folder,
                              const AppPackStats &stats) {
  auto file = folder;
  while (file.back() == '/') {
    file.pop_back();
  }

  return new MakeContext(std::move(file), stats);
}
