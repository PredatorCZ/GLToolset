#pragma once
#include "pointer.hpp"
#include "spike/crypto/jenkinshash3.hpp"
#include <string>

namespace prime::common {
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
      if constexpr (std::is_base_of_v<ResourceBase, C>) {
        ValidateClass(*item);
      }
    }

    return item;
  }
};

// Load resource data, don't add it to global registry
ResourceData LoadResource(const std::string &path);
ResourceData &FindResource(const void *address);

void ProjectDataFolder(std::string_view path);
std::string ProjectDataFolder();
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
  void (*Update)(ResourceHash object) = nullptr;
};

bool AddResourceHandle(uint32 hash, ResourceHandle handle);

template <class C> bool AddResourceHandle(ResourceHandle handle) {
  return AddResourceHandle(GetClassHash<C>(), handle);
}

void *GetResourceHandle(ResourceData &data);
const ResourceHandle &GetClassHandle(uint32 classHash);

template <class C> C *LinkResource(Pointer<C> &resource) {
  if (resource.isLinked) {
    return resource;
  }

  auto &resData = LoadResource(resource.resourceHash);
  resource.resourcePtr = static_cast<C *>(GetResourceHandle(resData));
  resData.numRefs++;

  return resource;
}

void UnlinkResource(ResourceBase *ptr);

void PollUpdates();

} // namespace prime::common
