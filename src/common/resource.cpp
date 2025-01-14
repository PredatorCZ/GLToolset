#include "common/resource.hpp"
#include "spike/io/binreader.hpp"
#include "spike/io/directory_scanner.hpp"
#include "spike/io/fileinfo.hpp"
#include "spike/io/stat.hpp"
#include "spike/master_printer.hpp"
#include "utils/converters.hpp"
#include <dirent.h>
#include <map>
#include <script/scriptapi.hpp>
#include <sys/inotify.h>
#include <thread>

namespace prime::common {
std::vector<std::string> workingDirs;
static std::map<uint32, std::string> watches;
static std::map<uint32, std::vector<ResourcePath>> workDirFiles;
static std::string projectFolder;
static std::string projectCacheFolder;

static ResourceData LoadResource(const std::string &path) {
  AFileInfo fileInfo(path);
  ResourceData data{};
  data.hash.name = JenkinsHash3_(fileInfo.GetFullPathNoExt());
  std::string_view ext = fileInfo.GetExtension();
  ext.remove_prefix(1);

  GetClassFromExtension(ext)
      .Success([&data](uint32 value) { data.hash.type = value; })
      .Unused();

  auto Load = [&](const std::string &base) {
    BinReader rd(path.front() == '/' ? path : base + path);
    rd.ReadContainer(data.buffer, rd.GetSize());
  };

  bool found = false;

  try {
    Load(projectCacheFolder);
    found = true;
  } catch (const es::FileNotFoundError &) {
    for (auto &w : workingDirs) {
      try {
        Load(w);
        found = true;
        break;
      } catch (const es::FileNotFoundError &) {
      }
    }
  }

  if (!found) {
    throw es::FileNotFoundError(path);
  }

  return data;
}

std::map<ResourceHash, std::pair<std::string, ResourceData>> resources;
static std::map<const void *, ResourceData *> resourceFromPtr;

ResourceHash AddSimpleResource(std::string path, uint32 classHash) {
  ResourceHash hash{};
  hash.name = JenkinsHash3_(path);
  hash.type = classHash;

  if (resources.contains(hash)) {
    return hash;
  }

  if (auto ext = GetExtentionFromHash(classHash); !ext.empty()) {
    path.push_back('.');
    path.append(ext);
  }

  resources.insert({hash, {path, {hash, {}}}});
  return hash;
}

void AddSimpleResource(ResourceData &&resource) {
  ResourceHash key(resource.hash);

  auto [item, _] = resources.insert_or_assign(
      key, std::pair<std::string, ResourceData>{{}, std::move(resource)});
  resourceFromPtr.emplace(item->second.second.buffer.data(),
                          &item->second.second);
}

Return<void> RegisterResource(std::string path) {
  AFileInfo finf(path);
  const uint32 name = JenkinsHash3_(finf.GetFullPathNoExt());

  return GetClassFromExtension(finf.GetExtension().substr(1))
      .Success([&](uint32 value) {
        ResourceHash hash(name, value);
        resources.insert({hash, {path, {hash, {}}}});
      })
      .Void();
}

void FreeResource(ResourceData &resource) {
  resourceFromPtr.erase(resource.buffer.data());
  resources.erase(resource.hash);
}

ResourceData &FindResource(const void *address) {
  return *resourceFromPtr.at(address);
}

const ResourcePath &FindResource(ResourceHash hash) {
  auto &files = workDirFiles.at(hash.name);

  for (auto &f : files) {
    if (f.hash.type == hash.type) {
      return f;
    }
  }

  throw std::out_of_range("Resource not found in working directories.");
}

auto &Registry() {
  static std::map<uint32, ResourceHandle> registry;
  return registry;
}

bool AddResourceHandle(uint32 hash, ResourceHandle handle) {
  return Registry().emplace(hash, handle).second;
}

void *GetResourceHandle(ResourceData &data) { return data.buffer.data(); }

Return<const ResourceHandle *> GetClassHandle(uint32 classHash) {
  return MapGetCPtrOr(Registry(), classHash, [classHash] {
    RUNTIME_ERROR("Cannot find class 0x%X in registry", classHash);
  });
}

ResourceData &LoadResource(ResourceHash hash, bool reload) {
  auto found = resources.find(hash);

  if (found == resources.end()) {
    auto foundWork = workDirFiles.find(hash.name);

    if (foundWork == workDirFiles.end()) {
      throw es::FileNotFoundError();
    }

    bool foundExact = false;
    ResourcePath nutVariant;
    ResourcePath rawVariant;

    for (auto &f : foundWork->second) {
      if (f.hash.type == hash.type) {
        resources.insert({hash, {f.localPath, {hash, {}}}});
        foundExact = true;
        break;
      } else if (f.localPath.ends_with(".nut")) {
        nutVariant = f;
      } else {
        ResourcePath rawVariant = f;
        rawVariant.hash = hash;
        if (utils::ConvertResource(rawVariant)) {
          PrintInfo("Converted: ", f.localPath);
          break;
        }
      }
    }

    if (!foundExact) {
      if (nutVariant.localPath.size() > 0) {
        script::CompileScript(nutVariant);
      }
    }

    int t = 0;
  }

  auto &res = resources.at(hash);
  auto &[fileName, resource] = res;

  if (resource.buffer.empty() || reload) {
    res.second = LoadResource(fileName);
    resourceFromPtr.emplace(resource.buffer.data(), &res.second);
  }

  if (Registry().contains(hash.type)) {
    if (res.second.numRefs == 0) {
      Registry().at(hash.type).Process(res.second);
    }
  }

  return res.second;
}

void UnlinkResource(ResourceBase *ptr) {
  auto &foundRes = FindResource(ptr);
  foundRes.numRefs--;

  if (foundRes.numRefs < 1) {
    if (Registry().contains(foundRes.hash.type)) {
      Registry().at(foundRes.hash.type).Delete(foundRes);
    }
  }
}

static int INOTIFY = [] {
  const int watchFd = inotify_init1(IN_NONBLOCK);
  if (watchFd < 0) {
    PrintWarning("Failed to add watch: ", strerror(errno));
  }

  return watchFd;
}();

static std::vector<ResourceHash> UPDATES;
static char buffer[0x1000] alignas(16);

void PollUpdates() {
  const ssize_t rdLen = read(INOTIFY, buffer, sizeof(buffer));

  for (ssize_t cb = 0; cb < rdLen;) {
    const inotify_event *event =
        reinterpret_cast<const inotify_event *>(buffer + cb);
    cb += event->len + sizeof(inotify_event);

    std::string watchPath = watches.at(event->wd) + '/' + event->name;

    for (auto &w : workingDirs) {
      if (watchPath.starts_with(w)) {
        watchPath.erase(0, w.size());
        const size_t foundDot = watchPath.find_last_of('.');
        static const auto &reg = Registry();

        if (foundDot != watchPath.npos) {
          GetClassFromExtension(watchPath.substr(foundDot + 1))
              .Either(
                  [&](uint32 value) {
                    if (reg.contains(value)) {
                      const ResourceHandle &hdl = reg.at(value);
                      if (hdl.Update) {
                        hdl.Update(ResourceHash(
                            JenkinsHash3_(watchPath.substr(0, foundDot)),
                            value));
                      }
                      return;
                    }

                    for (auto &[c, hdl] : reg) {
                      if (hdl.Update) {
                        hdl.Update(ResourceHash(
                            JenkinsHash3_(watchPath.substr(0, foundDot)),
                            value));
                      }
                    }
                  },
                  [&] {
                    for (auto &[c, hdl] : reg) {
                      if (hdl.Update) {
                        hdl.Update(ResourceHash(JenkinsHash3_(watchPath), 0));
                      }
                    }
                  })
              .Unused();
        } else {
          for (auto &[c, hdl] : reg) {
            if (hdl.Update) {
              hdl.Update(ResourceHash(
                  JenkinsHash3_(watchPath.substr(0, foundDot)), 0));
            }
          }
        }

        break;
      }
    }

    int t = 0;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
}
/*
static std::jthread WATCHER([](std::stop_token stop_token) {
  while (!stop_token.stop_requested()) {
  }
});*/

void WatchTree(const std::string &path, std::string_view workDir) {
  int watchFd = inotify_add_watch(INOTIFY, path.c_str(), IN_CLOSE_WRITE);
  if (watchFd < 0) {
    PrintWarning("Failed to create watch for ", path, " ", strerror(errno));
  } else {
    watches[watchFd] = path;
  }

  DIR *cDir = opendir(path.c_str());

  if (!cDir) {
    return;
  }

  dirent *cFile = nullptr;

  while ((cFile = readdir(cDir)) != nullptr) {
    if (!strcmp(cFile->d_name, ".") || !strcmp(cFile->d_name, "..") ||
        !strcmp(cFile->d_name, ".prime")) {
      continue;
    }

    std::string absPath(path + '/' + cFile->d_name);

    if (cFile->d_type == DT_DIR) {
      WatchTree(absPath, workDir);
    } else {
      std::string localPath = absPath.substr(workDir.size());
      // PrintInfo(localPath);
      AFileInfo finf(localPath);
      const uint32 hash = JenkinsHash3_(finf.GetFullPathNoExt());
      std::string_view ext(finf.GetExtension());
      const uint32 type =
          ext.empty() ? 0 : GetClassFromExtension(ext.substr(1)).retVal;
      ResourcePath resPath{
          .hash = ResourceHash(hash, type),
          .localPath = localPath,
          .workingDir = workDir,
      };

      workDirFiles[hash].emplace_back(resPath);
    }
  }

  closedir(cDir);
}

void AddWorkingFolder(std::string path) {
  workingDirs.emplace_back(path);
  while (path.size() && path.back() == '/') {
    path.pop_back();
  }
  WatchTree(path, workingDirs.back());
}

void ProjectDataFolder(std::string_view path) {
  AddWorkingFolder(std::string(path));

  while (path.back() == '/') {
    path.remove_suffix(1);
  }

  projectFolder.append(path);
  projectFolder.append("/.prime/");
  es::mkdir(projectFolder);
  projectCacheFolder = projectFolder;
  projectCacheFolder.append("cache/");
  es::mkdir(projectCacheFolder);

  DirectoryScanner sc;
  sc.Scan(projectCacheFolder);

  for (std::string f : sc.Files()) {
    f.erase(0, projectCacheFolder.size());
    AFileInfo finf(f);
    std::string_view ext = finf.GetExtension();
    ext.remove_prefix(1);

    GetClassFromExtension(ext)
        .Success([&](uint32 value) {
          ResourceHash rhash(JenkinsHash3_(finf.GetFullPathNoExt()), value);

          resources.insert(
              {rhash, {std::string(finf.GetFullPath()), {rhash, {}}}});
        })
        .Unused();
  }
}

const std::string &ProjectDataFolder() { return projectFolder; }

const std::string &CacheDataFolder() { return projectCacheFolder; }
} // namespace prime::common
