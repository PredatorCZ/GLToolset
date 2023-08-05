#include "common/resource.hpp"
#include "spike/io/binreader.hpp"
#include "spike/io/fileinfo.hpp"
#include <map>

namespace prime::utils {
const std::string &ShadersSourceDir();
}

namespace prime::common {
static std::vector<std::string> workingDirs;

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
    Load(prime::utils::ShadersSourceDir());
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

ResourceData &LoadResource(ResourceHash hash) {
  auto &res = resources.at(hash);
  auto &[fileName, resource] = res;

  if (resource.buffer.empty()) {
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

void AddWorkingFolder(std::string path) {
  workingDirs.emplace_back(std::move(path));
}

} // namespace prime::common
