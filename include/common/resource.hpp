#pragma once
#include "core.hpp"
#include "datas/jenkinshash3.hpp"
#include <string>

namespace prime::common {
union ResourceHash {
  struct {
    uint32 name;
    uint32 type;
  };
  uint64 hash = 0;

  ResourceHash() = default;
  ResourceHash(const ResourceHash &) = default;
  explicit ResourceHash(uint64 hash_) : hash(hash_) {}
  explicit ResourceHash(uint32 name_) : name(name_) {}
  explicit ResourceHash(uint32 name_, uint32 type_)
      : name(name_), type(type_) {}
  bool operator<(const ResourceHash &other) const { return hash < other.hash; }
};

template <class C> ResourceHash MakeHash(uint32 name) {
  return ResourceHash{name, GetClassHash<C>()};
}

template <class C> ResourceHash MakeHash(std::string_view name) {
  return ResourceHash{JenkinsHash3_({name.data(), name.size()}),
                      GetClassHash<C>()};
}

struct ResourceData {
  ResourceHash hash;
  std::string buffer;

  template <class C> C *As() { return reinterpret_cast<C *>(buffer.data()); }
};

// Load resource data, don't add it to global registry
ResourceData LoadResource(const std::string &path);
void AddSimpleResource(ResourceData &&resource);
void SetWorkingFolder(const std::string &path);
const std::string &WorkingFolder();

// Add resource to global registry for deferred loading
ResourceHash AddSimpleResource(std::string path, uint32 classHash);
template <class C> ResourceHash AddSimpleResource(const std::string &path) {
  return AddSimpleResource(path, GetClassHash<C>());
}

// Load resource data, that are registered via AddSimpleResource
ResourceData &LoadResource(ResourceHash hash);

} // namespace prime::common
