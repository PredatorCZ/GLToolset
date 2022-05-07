#pragma once
#include "datas/jenkinshash3.hpp"

#define HASH_CLASS(...)                                                        \
  template <> constexpr uint32 prime::common::GetClassHash<__VA_ARGS__>() {    \
    return JenkinsHash3_(#__VA_ARGS__);                                        \
  }

namespace prime::common {
template <class C> constexpr uint32 GetClassHash() { return 0; }
template <> constexpr uint32 GetClassHash<char>() {
  return JenkinsHash3_("prime::common::String");
}

template <class C> struct Array {
  union {
    int64 pointer;
    C *data;
  };

  uint32 classHash = GetClassHash<C>();
  uint32 numItems = 0;

  static_assert(GetClassHash<C>(), "Unregistered prime class.");

  C *begin() const { return data; }
  C *end() const { return data + numItems; }
  C &operator[](size_t index) { return data[index]; }
};
} // namespace prime::common
