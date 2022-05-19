#include "common/resource.hpp"
#include "datas/binreader.hpp"
#include "datas/fileinfo.hpp"
#include <map>

namespace prime::common {
static std::string workingDirectory;

ResourceData LoadResource(const std::string &path) {
  AFileInfo fileInfo(path);
  BinReader rd(path.front() == '/' ? path : workingDirectory + path);
  ResourceData data{};
  data.hash.name = JenkinsHash3_(fileInfo.GetFullPathNoExt());
  try {
    auto ext = fileInfo.GetExtension();
    ext.remove_prefix(1);
    data.hash.type = GetClassFromExtension({ext.data(), ext.size()});
  } catch (...) {
  }
  rd.ReadContainer(data.buffer, rd.GetSize());
  return data;
}

static std::map<ResourceHash, std::pair<std::string, ResourceData>> resources;

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
  resources.insert({resource.hash, {{}, std::move(resource)}});
}

ResourceData &LoadResource(ResourceHash hash) {
  auto &res = resources.at(hash);
  auto &[fileName, resource] = res;

  if (resource.buffer.empty()) {
    res.second = LoadResource(fileName);
  }

  return res.second;
}

void SetWorkingFolder(const std::string &path) { workingDirectory = path; }
const std::string &WorkingFolder() { return workingDirectory; }

} // namespace prime::common
