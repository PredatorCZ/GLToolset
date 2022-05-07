#pragma once
#include "datas/supercore.hpp"
#include <map>
#include <string>

namespace prime::common {
struct ResourceData {
  uint32 hash;
  std::string buffer;

  template <class C> C *As() { return reinterpret_cast<C *>(buffer.data()); }
};

ResourceData LoadResource(const std::string &path);
uint32 AddSimpleResource(const std::string &path, uint32 seed = 0);
void AddSimpleResource(ResourceData &&resource);
ResourceData &LoadResource(uint32 hash);
void SetWorkingFolder(const std::string &path);
const std::string &WorkingFolder();
} // namespace prime::common
