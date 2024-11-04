#pragma once
#include "spike/util/detail/sc_architecture.hpp"
#include <tuple>

namespace prime::reflect {
enum class Type : uint8 {
  None,
  Bool,
  Int,
  Uint,
  Float,
  Vec2,
  Vec3,
  Vec4,
  Class,
  Subtype,
  Flags,
  Quat,
  DualQuat,
  Enum,
  ExternalResource,
  HString,
};

enum class Container : uint8 {
  None,
  InlineArray,
  Array,
  LocalPointer,
  Pointer,
};

struct DataTypeBase {
  Type type;
  Container container;
  uint8 size;
  uint8 alignment;
  uint32 count;
};

struct DataType : DataTypeBase {
  uint32 hash;
};

struct Member {
  const char *name;
  const DataType *types;
  uint32 nTypes;
  uint32 offset;
};

struct Class {
  const char *className;
  const Member *members;
  uint32 nMembers;

  template <class ClassType, class... C>
  Class(const ClassType *, const char *className_, C... members_)
      : className(className_), nMembers(sizeof...(C)) {
    if constexpr (sizeof...(C) > 0) {
      static const Member members__[]{members_...};
      members = members__;
    }
  }
};

template <class C> const Class *GetReflectedClass();
const Class *GetReflectedClass(uint32 hash);

template <class C> struct EnumMember {
  using value_type = C;
  const char *name;
  C value;
};

struct Enum {
  const char *name;
  const char **names;
  const void *values;
  uint32 nMembers;
  uint16 size;
  bool isSigned = false;

  template <class EnumType, class... C>
  Enum(const EnumType *, const char *enumName, C... members_)
      : name(enumName), nMembers(sizeof...(C)) {
    if constexpr (sizeof...(C) > 0) {
      static const char *names__[]{members_.name...};
      names = names__;

      using value_type =
          typename std::tuple_element_t<0, std::tuple<C...>>::value_type;
      size = sizeof(value_type);
      isSigned = std::is_signed_v<value_type>;

      static const value_type values_[]{members_.value...};
      values = values_;
    }
  }
};

template <class C> const Enum *GetReflectedEnum();
const Enum *GetReflectedEnum(uint32 hash);
} // namespace prime::reflect
