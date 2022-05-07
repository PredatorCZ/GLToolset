#include "common/resource.hpp"
#include "datas/binreader.hpp"
#include "datas/jenkinshash3.hpp"

namespace prime::common {
static std::string workingDirectory;

ResourceData LoadResource(const std::string &path) {
  BinReader rd(workingDirectory + path);
  ResourceData data{};
  data.hash = JenkinsHash3_(path);
  rd.ReadContainer(data.buffer, rd.GetSize());
  return data;
}

static std::map<uint32, std::pair<std::string, ResourceData>> resources;

uint32 AddSimpleResource(const std::string &path, uint32 seed) {
  const uint32 hash = JenkinsHash3_(path, seed);
  resources.insert({hash, {path, {hash, {}}}});
  return hash;
}

void AddSimpleResource(ResourceData &&resource) {
  resources.insert({resource.hash, {{}, std::move(resource)}});
}

ResourceData &LoadResource(uint32 hash) {
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
