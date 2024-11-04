#pragma once
#include "core.hpp"
#include "resource_hash.hpp"

namespace prime::common {
template <class C> struct Pointer {
  Pointer() = default;
  Pointer(const Pointer &other)
      : resourcePtr{other.resourcePtr}, isLinked{other.isLinked},
        locator{other.locator} {}
  Pointer(Pointer &&other)
      : resourcePtr{other.resourcePtr}, isLinked{other.isLinked},
        locator{other.locator} {}
  Pointer(C *value) : resourcePtr{value}, isLinked{1} {}
  Pointer(const ResourceHash &value) : resourceHash{value} {}

  Pointer &operator=(const Pointer &other) {
    resourcePtr = other.resourcePtr;
    isLinked = other.isLinked;
    locator = other.locator;
    return *this;
  }

  Pointer &operator=(Pointer &&other) {
    resourcePtr = other.resourcePtr;
    isLinked = other.isLinked;
    locator = other.locator;
    return *this;
  }

  Pointer &operator=(C *other) {
    resourcePtr = other;
    isLinked = 1;
    locator = 0;
    return *this;
  }

  Pointer &operator=(const ResourceHash &other) {
    resourceHash = other;
    isLinked = 0;
    locator = 0;
    return *this;
  }

  static_assert(GetClassHash<C>(), "Unregistered prime class.");

  operator C *() { return resourcePtr; }
  C &operator*() { return *resourcePtr; }
  C *operator->() { return resourcePtr; }

  operator const C *() const { return resourcePtr; }
  const C &operator*() const { return *resourcePtr; }
  const C *operator->() const { return resourcePtr; }

private:
  template <class D> friend D *LinkResource(Pointer<D> &);
  union {
    ResourceHash resourceHash{};
    C *resourcePtr;
  };
  const uint32 classHash = GetClassHash<C>();
  uint32 isLinked : 1 = 0;
  uint32 locator : 31 = 0;
};

static_assert(sizeof(Pointer<char>) == 16);
} // namespace prime::common
