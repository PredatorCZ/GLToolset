#pragma once
#include "core.hpp"

namespace prime::common {
template <class C> struct LocalPointerBase {
  typedef C value_type;

  int32 pointer;

  operator bool() const { return pointer != 0; }

  C *Get() {
    return pointer ? reinterpret_cast<C *>(reinterpret_cast<char *>(&pointer) +
                                           pointer)
                   : nullptr;
  }

  const C *Get() const {
    return pointer ? reinterpret_cast<const C *>(
                         reinterpret_cast<const char *>(&pointer) + pointer)
                   : nullptr;
  }

  operator C *() { return Get(); }
  C &operator*() { return *static_cast<C *>(*this); }
  C *operator->() { return *this; }

  operator const C *() const { return Get(); }
  const C &operator*() const { return *static_cast<const C *>(*this); }
  const C *operator->() const { return *this; }

  LocalPointerBase &operator=(C *newDest) {
    uintptr_t _rawDest = reinterpret_cast<uintptr_t>(newDest);
    pointer =
        static_cast<int32>(_rawDest - reinterpret_cast<uintptr_t>(&pointer));

    return *this;
  }

  void Reset(int32 value = 0) { pointer = value; }
};

template <class C> struct LocalPointer : LocalPointerBase<C> {
  uint32 classHash = GetClassHash<C>();
  static_assert(GetClassHash<C>(), "Unregistered prime class.");
};

template <class... C>
struct LocalVariantPointer : private LocalPointerBase<char> {
  uint32 classHash = 0;
  static_assert((GetClassHash<C>() && ...), "Unregistered prime class.");

  template <class D> void operator=(const LocalPointerBase<D> &other) {
    static_assert(((GetClassHash<C>() == GetClassHash<D>()) || ...),
                  "Variant asign type is invalid.");
    Reset(other.pointer);
  }
  template <class D> void operator=(D *other) {
    static_assert(((GetClassHash<C>() == GetClassHash<D>()) || ...),
                  "Variant asign type is invalid.");
    *reinterpret_cast<LocalPointerBase<D> *>(this) = other;
    classHash = GetClassHash<D>();
  }

  template <class D> const D *Get() const {
    static_assert(((GetClassHash<C>() == GetClassHash<D>()) || ...),
                  "Variant get type is invalid.");

    if (classHash != GetClassHash<D>()) {
      throw std::bad_cast();
    }

    return *reinterpret_cast<const LocalPointerBase<D> *>(this);
  }

  template <class D> operator const D *() const { return Get<D>(); }

  using LocalPointerBase<char>::operator bool;
};

} // namespace prime::common
