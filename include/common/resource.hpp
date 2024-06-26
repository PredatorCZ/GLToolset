#pragma once
#include "core.hpp"
#include "spike/crypto/jenkinshash3.hpp"
#include <string>

namespace prime::common {
union ResourceHash {
  uint64 hash = 0;
  struct {
    uint32 name;
    uint32 type;
  };

  ResourceHash() = default;
  ResourceHash(const ResourceHash &) = default;
  explicit ResourceHash(uint64 hash_) : hash(hash_) {}
  explicit ResourceHash(uint32 name_) : name(name_) {}
  explicit ResourceHash(uint32 name_, uint32 type_)
      : name(name_), type(type_) {}
  bool operator<(const ResourceHash &other) const { return hash < other.hash; }
};

template <class C> union ResourcePtr {
  ResourceHash resourceHash{};
  C *resourcePtr;

  operator C *() { return resourcePtr; }
  C *operator->() { return resourcePtr; }
};

template <typename, typename = void> constexpr bool is_type_complete_v = false;

template <typename T>
constexpr bool is_type_complete_v<T, std::void_t<decltype(sizeof(T))>> = true;

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
  int32 numRefs = 0;

  template <class C> C *As() {
    C *item = reinterpret_cast<C *>(buffer.data());

    if constexpr (is_type_complete_v<C>) {
      if constexpr (std::is_base_of_v<Resource, C>) {
        ValidateClass(*item);
      }
    }

    return item;
  }
};

// Load resource data, don't add it to global registry
ResourceData LoadResource(const std::string &path);
ResourceData &FindResource(const void *address);

void AddWorkingFolder(std::string path);
void FreeResource(ResourceData &resource);
void ReplaceResource(ResourceData &oldResource, ResourceData &newResource);
std::string ResourcePath(std::string path);
std::string ResourceWorkingFolder(std::string path);
std::string ResourceWorkingFolder(ResourceHash object);

// Add resource to global registry for deferred loading
void AddSimpleResource(ResourceData &&resource);
ResourceHash AddSimpleResource(std::string path, uint32 classHash);
template <class C> ResourceHash AddSimpleResource(const std::string &path) {
  return AddSimpleResource(path, GetClassHash<C>());
}

// Load resource data, that are registered via AddSimpleResource
ResourceData &LoadResource(ResourceHash hash, bool reload = false);

template <class C> struct InvokeGuard;

struct ResourceHandle {
  void (*Process)(ResourceData &res);
  void (*Delete)(ResourceData &res);
  void *(*Handle)(ResourceData &res);
  void (*Update)(ResourceHash object) = nullptr;
};

bool AddResourceHandle(uint32 hash, ResourceHandle handle);

template <class C> bool AddResourceHandle(ResourceHandle handle) {
  return AddResourceHandle(GetClassHash<C>(), handle);
}

void *GetResourceHandle(ResourceData &data);
const ResourceHandle &GetClassHandle(uint32 classHash);

template <class C> C *LinkResource(ResourceHash &resourceHash) {
  auto &resData = LoadResource(resourceHash);
  ResourcePtr<C> mut;
  mut.resourcePtr = static_cast<C *>(GetResourceHandle(resData));
  resourceHash = mut.resourceHash;
  resData.numRefs++;

  return mut.resourcePtr;
}

void UnlinkResource(Resource *ptr);

void PollUpdates();

} // namespace prime::common
