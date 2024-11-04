#pragma once
#include "core.hpp"

namespace prime::common {
union ResourceHash {
  uint64 hash;
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

template <class C> struct ResourceToHandle : JenHash3 {
  ResourceToHandle() = default;
  ResourceToHandle(const ResourceHash &o) : JenHash3(o) {}
  ResourceToHandle(JenHash3 &o) : JenHash3(o) {}

  template <class D>
    requires std::is_integral_v<D>
  ResourceToHandle(D o) : JenHash3(o) {}

  operator ResourceHash() const {
    return {operator uint32(), GetClassHash<C>()};
  };
  operator bool() const { return operator uint32() & 0xf0000000; }
};
} // namespace prime::common
