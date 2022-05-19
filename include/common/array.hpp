#pragma once
#include "core.hpp"

namespace prime::common {
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
