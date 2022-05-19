#pragma once
#include "datas/jenkinshash.hpp"
#include <string_view>

#define HASH_CLASS(...)                                                        \
  template <> constexpr uint32 prime::common::GetClassHash<__VA_ARGS__>() {    \
    return JenkinsHash_(#__VA_ARGS__);                                         \
  }

namespace prime::common {
template <class C> constexpr uint32 GetClassHash() { return 0; }
template <> constexpr uint32 GetClassHash<char>() {
  return JenkinsHash_("prime::common::String");
}

template <class C> constexpr std::string_view GetClassExtension() { return ""; }

uint32 GetClassFromExtension(std::string_view ext);
std::string_view GetExtentionFromHash(uint32 hash);
} // namespace prime::common
