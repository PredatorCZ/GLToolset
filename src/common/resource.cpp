#include "common/resource.hpp"
#include "spike/io/binreader.hpp"
#include "spike/io/fileinfo.hpp"
#include <map>

#include "spike/master_printer.hpp"
#include <dirent.h>
#include <sys/inotify.h>
#include <thread>

namespace prime::common {
static std::vector<std::string> workingDirs;
static std::map<uint32, std::string> watches;

std::string ResourcePath(std::string path) {
  for (auto &w : workingDirs) {
    try {
      std::string absPath = path.front() == '/' ? path : w + path;
      BinReader rd(absPath);
      return absPath;
    } catch (const es::FileNotFoundError &) {
    }
  }

  throw es::FileNotFoundError(path);
}

std::string ResourceWorkingFolder(std::string path) {
  for (auto &w : workingDirs) {
    try {
      std::string absPath = path.front() == '/' ? path : w + path;
      BinReader rd(absPath);
      return w;
    } catch (const es::FileNotFoundError &) {
    }
  }

  throw es::FileNotFoundError(path);
}

ResourceData LoadResource(const std::string &path) {
  AFileInfo fileInfo(path);
  ResourceData data{};
  data.hash.name = JenkinsHash3_(fileInfo.GetFullPathNoExt());
  try {
    auto ext = fileInfo.GetExtension();
    ext.remove_prefix(1);
    data.hash.type = GetClassFromExtension({ext.data(), ext.size()});
  } catch (...) {
  }

  auto Load = [&](const std::string &base) {
    BinReader rd(path.front() == '/' ? path : base + path);
    rd.ReadContainer(data.buffer, rd.GetSize());
  };

  bool found = false;

  for (auto &w : workingDirs) {
    try {
      Load(w);
      found = true;
      break;
    } catch (const es::FileNotFoundError &) {
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
  auto [item, _] = resources.insert_or_assign(
      resource.hash,
      std::pair<std::string, ResourceData>{{}, std::move(resource)});
  resourceFromPtr.emplace(item->second.second.buffer.data(),
                          &item->second.second);
}

std::string ResourceWorkingFolder(ResourceHash object) {
  auto found = resources.find(object);

  if (found == resources.end()) {
    throw es::FileNotFoundError("[object]");
  }

  return ResourceWorkingFolder(found->second.first);
}

void FreeResource(ResourceData &resource) {
  resourceFromPtr.erase(resource.buffer.data());
  resources.erase(resource.hash);
}

ResourceData &FindResource(const void *address) {
  return *resourceFromPtr.at(address);
}

auto &Registry() {
  static std::map<uint32, ResourceHandle> registry;
  return registry;
}

bool AddResourceHandle(uint32 hash, ResourceHandle handle) {
  return Registry().emplace(hash, handle).second;
}

void *GetResourceHandle(ResourceData &data) {
  if (Registry().contains(data.hash.type)) {
    return Registry().at(data.hash.type).Handle(data);
  }

  return data.buffer.data();
}

const ResourceHandle &GetClassHandle(uint32 classHash) {
  Registry().at(classHash);
}

ResourceData &LoadResource(ResourceHash hash, bool reload) {
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

void UnlinkResource(Resource *ptr) {
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
          uint32 classHash = 0;

          try {
            GetClassFromExtension(watchPath.substr(foundDot + 1));
          } catch (const std::out_of_range &) {
          }

          if (reg.contains(classHash)) {
            const ResourceHandle &hdl = reg.at(classHash);
            if (hdl.Update) {
              hdl.Update(ResourceHash(
                  JenkinsHash3_(watchPath.substr(0, foundDot)), classHash));
            }
          } else if (classHash > 0) {
            for (auto &[c, hdl] : reg) {
              if (hdl.Update) {
                hdl.Update(ResourceHash(
                    JenkinsHash3_(watchPath.substr(0, foundDot)), classHash));
              }
            }
          } else {
            for (auto &[c, hdl] : reg) {
              if (hdl.Update) {
                hdl.Update(ResourceHash(JenkinsHash3_(watchPath), 0));
              }
            }
          }
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

void WatchTree(const std::string &path) {
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
    if (!strcmp(cFile->d_name, ".") || !strcmp(cFile->d_name, "..")) {
      continue;
    }

    if (cFile->d_type == DT_DIR) {
      WatchTree(path + '/' + cFile->d_name);
    }
  }

  closedir(cDir);
}

void AddWorkingFolder(std::string path) {
  workingDirs.emplace_back(path);
  if (path.back() == '/') {
    path.pop_back();
  }
  WatchTree(path);
}

} // namespace prime::common
