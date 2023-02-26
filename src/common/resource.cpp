#include "common/resource.hpp"
#include "datas/binreader.hpp"
#include "datas/fileinfo.hpp"
#include <map>

namespace prime::utils {
const std::string &ShadersSourceDir();
}

namespace prime::common {
static std::string workingDirectory;

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

  try {
    Load(workingDirectory);
  } catch (const es::FileNotFoundError &) {
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
  resourceFromPtr.emplace(item->second.second.buffer.data(), &item->second.second);
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

ResourceData &LoadResource(ResourceHash hash) {
  auto &res = resources.at(hash);
  auto &[fileName, resource] = res;

  if (resource.buffer.empty()) {
    res.second = LoadResource(fileName);
    resourceFromPtr.emplace(resource.buffer.data(), &res.second);
  }

  if (Registry().contains(hash.type)) {
    Resource *item = reinterpret_cast<Resource *>(res.second.buffer.data());
    if (res.second.numRefs == 0) {
      Registry().at(hash.type).Process(res.second);
    }
  }

  return res.second;
}

void SetWorkingFolder(const std::string &path) { workingDirectory = path; }
const std::string &WorkingFolder() { return workingDirectory; }

} // namespace prime::common
