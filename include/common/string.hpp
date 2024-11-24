#pragma once
#include "local_pointer.hpp"

namespace prime::common {
union String {
  operator std::string_view() const {
    if (asTiny.length & 1) {
      return {asTiny.data, size_t(asTiny.length >> 1)};
    }

    return {asBig.data.Get(), asBig.length >> 1};
  }

  const char *raw() const {
    if (asTiny.length & 1) {
      return asTiny.data;
    }

    return asBig.data.Get();
  }

  struct {
    uint8 length;
    char data[7];
  } asTiny;

  struct {
    uint32 length;
    LocalPointerBase<char> data;
  } asBig;
};
} // namespace prime::common

HASH_CLASS(prime::common::String);
