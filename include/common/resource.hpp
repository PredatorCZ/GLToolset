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

struct ResourcePath {
  ResourceHash hash;
  std::string localPath;
  std::string_view workingDir;

  std::string AbsPath() const { return std::string(workingDir) + localPath; }
};

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

// Resource data
ResourceData &LoadResource(ResourceHash hash, bool reload = false);
ResourceData &FindResource(const void *address);
const ResourcePath &FindResource(ResourceHash hash);
void FreeResource(ResourceData &resource);
void ReplaceResource(ResourceData &oldResource, ResourceData &newResource);
template<class C>
const ResourcePath &FindResource(JenHash3 name) {
  return FindResource(MakeHash<C>(name));
}

// Project data
void ProjectDataFolder(std::string_view path);
const std::string &ProjectDataFolder();
const std::string &CacheDataFolder();

// Working folders
void AddWorkingFolder(std::string path);

// Resource registry
void RegisterResource(std::string path);
void AddSimpleResource(ResourceData &&resource);
ResourceHash AddSimpleResource(std::string path, uint32 classHash);
template <class C> ResourceHash AddSimpleResource(const std::string &path) {
  return AddSimpleResource(path, GetClassHash<C>());
}

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

  if (resource.resourceHash.name == 0) {
    return nullptr;
  }

  auto &resData = LoadResource(resource.resourceHash);
  resource.resourcePtr = static_cast<C *>(GetResourceHandle(resData));
  resData.numRefs++;

  return resource;
}

void UnlinkResource(ResourceBase *ptr);

void PollUpdates();

} // namespace prime::common
