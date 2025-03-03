#include "utils/converters.hpp"
#include "common/resource.hpp"
#include "graphics/model_single.hpp"
#include "graphics/texture.hpp"
#include "spike/app_context.hpp"
#include "spike/io/binreader.hpp"
#include "spike/io/binwritter.hpp"
#include "spike/io/directory_scanner.hpp"
#include "spike/io/stat.hpp"
#include "spike/reflect/reflector.hpp"
#include <map>
#include <memory>

namespace prime::common {
extern std::vector<std::string> workingDirs;
}

namespace {
AFileInfo basePath;
std::vector<std::string_view> basePathParts;

struct Context : AppContext {
  std::istream *OpenFile(const std::string &path) {
    for (size_t b = 0; b < 32; b++) {
      uint32 bit = 1 << b;
      if (!(usedFiles & bit)) {
        streamedFiles[b].Open(path);
        usedFiles ^= bit;
        return &streamedFiles[b].BaseStream();
      }
    }

    throw std::out_of_range("Maximum opened files reached!");
  }

  AppContextStream RequestFile(const std::string &path) override {
    AFileInfo wFile(workingFile);
    AFileInfo pFile(path);
    auto catchedFile = pFile.CatchBranch(wFile.GetFolder());
    return {OpenFile(catchedFile), this};
  }

  void DisposeFile(std::istream *str) override {
    size_t index = 0;

    for (auto &f : streamedFiles) {
      if (&f.BaseStream() == str) {
        uint32 bit = 1 << index;

        if (!(usedFiles & bit)) {
          throw std::runtime_error("Stream already freed.");
        }

        es::Dispose(f);
        usedFiles ^= bit;
        return;
      }

      index++;
    }

    throw std::runtime_error("Requested stream not found!");
  }

  AppContextFoundStream FindFile(const std::string &rootFolder,
                                 const std::string &pattern) override {
    if (!rootFolder.starts_with(prime::common::CacheDataFolder())) {
      throw std::runtime_error("FindFile rootFolder is not cache folder");
    }

    std::string searchPath(path.workingDir);
    searchPath.append(
        rootFolder.substr(prime::common::CacheDataFolder().size()));

    DirectoryScanner sc;
    sc.AddFilter(pattern);
    sc.Scan(searchPath);

    if (sc.Files().empty()) {
      throw es::FileNotFoundError(pattern);
    } else if (sc.Files().size() > 1) {
      std::string *winner = nullptr;
      size_t minFolder = 0x10000;

      for (auto &f : sc) {
        size_t foundIdx = f.find_last_of('/');

        if (foundIdx == f.npos) {
          throw std::runtime_error("Too many files found.");
        }

        if (foundIdx < minFolder) {
          winner = &f;
          minFolder = foundIdx;
        } else if (foundIdx == minFolder) {
          throw std::runtime_error("Too many files found.");
        }
      }

      if (!winner) {
        throw std::runtime_error("Too many files found.");
      }

      return {OpenFile(*winner), this, AFileInfo(*winner)};
    }

    return {OpenFile(sc.Files().front()), this, AFileInfo(sc.Files().front())};
  }

  const std::vector<std::string> &SupplementalFiles() override {
    throw std::logic_error("Unsupported call");
  }

  std::istream &GetStream() override { return mainFile.BaseStream(); }

  std::string GetBuffer(size_t size = -1, size_t begin = 0) override {
    mainFile.Push();
    mainFile.Seek(begin);
    std::string buffer;
    mainFile.ReadContainer(buffer,
                           size == size_t(-1) ? mainFile.GetSize() : size);
    mainFile.Pop();

    return buffer;
  }

  AppExtractContext *ExtractContext() override {
    throw std::logic_error("Unsupported call");
  }

  AppExtractContext *ExtractContext(std::string_view) override {
    throw std::logic_error("Unsupported call");
  }

  NewFileContext NewFile(const std::string &path) override {
    const std::string &cacheDir = prime::common::CacheDataFolder();
    const std::string filePath = cacheDir + path;
    ::prime::common::RegisterResource(path);
    try {
      outFile = BinWritter(filePath);
    } catch (const es::FileInvalidAccessError &e) {
      // todo: add to watchlist
      mkdirs(filePath);
      outFile = BinWritter(filePath);
    }
    return {outFile.BaseStream(), filePath, cacheDir.size()};
  }

  NewTexelContext *NewImage(NewTexelContextCreate,
                            const std::string *) override {
    throw std::logic_error("Unsupported call");
  }

  Context(const ::prime::common::ResourcePath &input)
      : path(input), mainFile(std::string(input.workingDir) + input.localPath) {
    workingFile.Load(input.localPath);
  }

  ::prime::common::ResourcePath path;

  BinWritter outFile;

  BinReader mainFile;
  BinReader streamedFiles[32];
  uint32 usedFiles = 0;
};

struct Converter {
  void (*Func)(AppContext *);
  Reflector *settings = nullptr;
  PathFilter filter;
};

PathFilter MakeFilter(std::span<std::string_view> items) {
  PathFilter filter;

  for (auto &i : items) {
    filter.AddFilter(i);
  }

  return filter;
}

std::map<uint32, std::vector<Converter>> CONVERTERS{
    {
        prime::common::GetClassHash<prime::graphics::ModelSingle>(),
        {
            Converter{
                .Func = prime::utils::ProcessMD2,
                .filter = MakeFilter(prime::utils::ProcessMD2Filters()),
            },
        },
    },
    {
        prime::common::GetClassHash<prime::graphics::Texture>(),
        {
            Converter{
                .Func = prime::utils::ProcessImage,
                .settings = prime::utils::ProcessImageSettings(),
                .filter = MakeFilter(prime::utils::ProcessImageFilters()),
            },
        },
    },
};

using CompileReg = std::map<uint32, prime::utils::CompileFunc>;

CompileReg &Compilers() {
  static CompileReg COMPILERS;
  return COMPILERS;
}
} // namespace

namespace prime::utils {
void ContextOutputPath(std::string output) {
  if (output.back() != '/') {
    output.push_back('/');
  }
  basePath = AFileInfo(output);
  basePathParts = basePath.Explode();
}

std::unique_ptr<AppContext> MakeContext(const common::ResourcePath &filePath) {
  return std::make_unique<Context>(filePath);
}

bool ConvertResource(const common::ResourcePath &path) {
  auto found = CONVERTERS.find(path.hash.type);

  if (found == CONVERTERS.end()) {
    return false;
  }

  std::vector<Converter> viableConvertors;

  for (auto &c : found->second) {
    if (c.filter.IsFiltered(path.localPath)) {
      viableConvertors.emplace_back(c);
    }
  }

  if (viableConvertors.empty()) {
    return false;
  }

  Context ctx(path);
  viableConvertors.front().Func(&ctx);

  return true;
}

CompileFunc GetCompileFunction(uint32 compilerClassHash) {
  auto found = Compilers().find(compilerClassHash);

  if (found == Compilers().end()) {
    return nullptr;
  }

  return found->second;
}

uint32 detail::RegisterCompiler(uint32 compilerClassHash, CompileFunc func) {
  Compilers().emplace(compilerClassHash, func);
  return 0;
}
} // namespace prime::utils
