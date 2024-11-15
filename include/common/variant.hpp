#pragma once
#include "spike/util/detail/sc_architecture.hpp"

namespace prime::common {
template <class... C> struct Variant {
  template <class D> Variant &operator=(D item) {
    static_assert(GetTypeIndex<D>() != 0xff, "Variant asign type is invalid.");
    reinterpret_cast<D &>(*data) = item;
    typeIndex = GetTypeIndex<D>();
    return *this;
  }

  template <class D> D &Get() {
    constexpr uint8 idx = GetTypeIndex<D>();
    if (idx != typeIndex) {
      throw std::runtime_error("Get class is not currently stored.");
    }
    return reinterpret_cast<D &>(*data);
  }

  template <class D> const D &Get() const {
    constexpr uint8 idx = GetTypeIndex<D>();
    if (idx != typeIndex) {
      throw std::runtime_error("Get class is not currently stored.");
    }
    return reinterpret_cast<const D &>(*data);
  }

  template <class D> consteval static uint8 GetTypeIndex() {
    const bool cmpResult[]{std::is_same_v<D, C>...};
    for (size_t i = 0; i < sizeof...(C); i++) {
      if (cmpResult[i]) {
        return i;
      }
    }

    return 0xff;
  }

  uint8 GetTypeIndex() const { return typeIndex; }

  template <class fn> void Visit(fn cb) const {
    if (typeIndex == 0xff) {
      throw std::runtime_error("Variant is unassigned");
    }
    static WrapType *wraps[]{
        Visitor([&](const Variant *self) { cb(self->Get<C>()); })...};
    wraps[typeIndex]->Call(this);
  }

private:
  struct WrapType {
    virtual void Call(const Variant *self) = 0;
  };

  template <class D> struct WrapTypeImpl : WrapType {
    WrapTypeImpl(D fn) : func(fn) {}

    void Call(const Variant *self) override { func(self); }

    D func;
  };

  template <class D> static WrapType *Visitor(D fn) {
    // this might cause sync problems, so maybe make it TLS?
    static WrapTypeImpl VISIT(fn);
    return &VISIT;
  }

  consteval static uint32 GetMaxSize() {
    const uint32 itemSizes[]{sizeof(C)...};
    uint32 maxSize = 0;
    for (size_t i = 0; i < sizeof...(C); i++) {
      if (itemSizes[i] > maxSize) {
        maxSize = itemSizes[i];
      }
    }

    return maxSize;
  }

  consteval static uint32 GetMaxAlign() {
    const uint32 itemSizes[]{alignof(C)...};
    uint32 maxSize = 0;
    for (size_t i = 0; i < sizeof...(C); i++) {
      if (itemSizes[i] > maxSize) {
        maxSize = itemSizes[i];
      }
    }

    return maxSize;
  }

  alignas(GetMaxAlign()) char data[GetMaxSize()];
  uint8 typeIndex = 0xff;
};
} // namespace prime::common
