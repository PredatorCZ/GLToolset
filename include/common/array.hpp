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

template <class C, class D> struct LocalArray {
  uint32 classHash = GetClassHash<C>();
  D numItems = 0;
  D pointer;

  static_assert(GetClassHash<C>(), "Unregistered prime class.");

  const C *begin() const {
    return reinterpret_cast<const C *>(
        reinterpret_cast<const char *>(&pointer) + pointer);
  }
  const C *end() const { return begin() + numItems; }
  const C &operator[](size_t index) const { return begin()[index]; }

  C *begin() {
    return reinterpret_cast<C *>(reinterpret_cast<char *>(&pointer) + pointer);
  }
  C *end() { return begin() + numItems; }
  C &operator[](size_t index) { return begin()[index]; }
};

template <class C> using LocalArray32 = LocalArray<C, int32>;
template <class C> using LocalArray16 = LocalArray<C, int16>;

} // namespace prime::common

HASH_CLASS(prime::common::LocalArray16<char>);
HASH_CLASS(prime::common::LocalArray32<char>);
