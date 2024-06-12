#include "common/resource.hpp"
#include "spike/app_context.hpp"
#include "spike/io/binreader.hpp"
#include "spike/io/binwritter.hpp"
#include "spike/io/directory_scanner.hpp"
#include "spike/io/stat.hpp"
#include <memory>

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
    DirectoryScanner sc;
    sc.AddFilter(pattern);
    sc.Scan(rootFolder);

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
    std::string filePath;
    size_t delimeter = 0;

    if (basePathParts.empty()) {
      filePath = std::string(basePath.GetFullPath()) + path;
      delimeter = basePath.GetFullPath().size();
    } else {
      AFileInfo pathInfo(path);
      auto exploded = pathInfo.Explode();
      const size_t numItems = std::min(exploded.size(), basePathParts.size());

      if (basePath.GetFullPath().at(0) == '/') {
        filePath.push_back('/');
      }

      size_t i = 0;

      for (; i < numItems; i++) {
        if (basePathParts[i] == exploded[i]) {
          filePath.append(exploded[i]);
          filePath.push_back('/');
          continue;
        }

        break;
      }

      for (size_t j = i; j < basePathParts.size(); j++) {
        filePath.append(basePathParts[j]);
        filePath.push_back('/');
      }

      delimeter = filePath.size();

      for (; i < exploded.size(); i++) {
        filePath.append(exploded[i]);
        filePath.push_back('/');
      }

      filePath.pop_back();
    }

    try {
      outFile = BinWritter(filePath);
    } catch (const es::FileInvalidAccessError &e) {
      mkdirs(filePath);
      outFile = BinWritter(filePath);
    }
    return {outFile.BaseStream(), filePath, delimeter};
  }

  NewTexelContext *NewImage(NewTexelContextCreate,
                            const std::string *) override {
    throw std::logic_error("Unsupported call");
  }

  Context(const std::string &input) {
    mainFile.Open(input);
    workingFile.Load(input);
  }

  BinWritter outFile;

  BinReader mainFile;
  BinReader streamedFiles[32];
  uint32 usedFiles = 0;
};
} // namespace

namespace prime::utils {
void ContextOutputPath(std::string output) {
  if (output.back() != '/') {
    output.push_back('/');
  }
  basePath = AFileInfo(output);
  basePathParts = basePath.Explode();
}

std::unique_ptr<AppContext> MakeContext(const std::string &filePath) {
  return std::make_unique<Context>(filePath);
}
} // namespace prime::utils
