#pragma once
#include "common/core.hpp"
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
  Variant,
  Color,
  String,
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
  common::ExtString extension;
  void (*construct)(void *);
  uint32 nMembers;
  uint32 classSize;

  template <class ClassType, class... C>
  Class(const ClassType *, const char *className_, C... members_)
      : className(className_),
        extension(common::GetClassExtension<ClassType>()),
        construct([](void *item) {
          if constexpr (prime::common::is_type_complete_v<ClassType>) {
            if constexpr (std::is_default_constructible_v<ClassType>) {
              std::construct_at(static_cast<ClassType *>(item));
            }
          }
        }),
        nMembers(sizeof...(C)), classSize([] {
          if constexpr (prime::common::is_type_complete_v<ClassType>) {
            return sizeof(ClassType);
          }

          return size_t(0);
        }()) {
    if constexpr (sizeof...(C) > 0) {
      static const Member members__[]{members_...};
      members = members__;
    }
  }
};

template <class C> const Class *GetReflectedClass();
common::Return<const Class *> GetReflectedClass(uint32 hash);

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
common::Return<const Enum *> GetReflectedEnum(uint32 hash);
} // namespace prime::reflect
