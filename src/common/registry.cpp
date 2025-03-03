#include "common/core.hpp"
#include <cstring>
#include <map>
#include <string_view>

auto &RegistryEH() {
  static std::map<prime::common::ExtString, uint32> REGISTRY_EH;
  return REGISTRY_EH;
};

auto &RegistryHE() {
  static std::map<uint32, prime::common::ExtString> REGISTRY_HE;
  return REGISTRY_HE;
};

prime::common::Return<uint32> prime::common::GetClassFromExtension(std::string_view ext) {
  prime::common::ExtString key;
  memcpy(key.c, ext.data(), std::min(sizeof(key) - 1, ext.size()));

  return MapGetOr(RegistryEH(), key, [&]{
    common::RuntimeError("Cannot find extension %s in class registry.", ext.data());
    return 0;
  });
}

std::string_view prime::common::GetExtentionFromHash(uint32 hash) {
  return RegistryHE().at(hash);
}

uint32 prime::common::detail::RegisterClass(prime::common::ExtString ext,
                                            uint32 obj) {
  RegistryEH().emplace(ext, obj);
  RegistryHE().emplace(obj, ext);
  return obj;
}

REGISTER_CLASS(char);
