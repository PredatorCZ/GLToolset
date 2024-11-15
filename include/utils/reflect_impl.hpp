#pragma once
#include "reflect.hpp"
#include <string>

#include "common/array.hpp"
#include "common/local_pointer.hpp"
#include "common/pointer.hpp"
#include "common/resource.hpp"
#include "common/variant.hpp"
#include "spike/type/flags.hpp"
#include "spike/type/vectors_simd.hpp"
#include <glm/gtx/dual_quaternion.hpp>

namespace prime::reflect {

template <class C> struct GetValueType {
  static const Container CONTAINER = Container::None;
  static const Type TYPE = Type::None;
};

template <class C, size_t N> struct GetValueType<C[N]> {
  using value_type = C;
  static const Container CONTAINER = Container::InlineArray;
  static const Type TYPE = Type::None;
  static const uint32 SIZE = N;
};

template <class C, class D>
struct GetValueType<prime::common::LocalArray<C, D>> {
  using value_type = C;
  static const Container CONTAINER = Container::Array;
  static const Type TYPE = Type::None;
  static const uint32 SIZE = sizeof(D);
};

template <class C, class D> struct GetValueType<es::Flags<C, D>> {
  using value_type = C;
  static const Container CONTAINER = Container::None;
  static const Type TYPE = Type::Flags;
};

template <class C> struct GetValueType<common::ResourceToHandle<C>> {
  using value_type = C;
  static const Container CONTAINER = Container::None;
  static const Type TYPE = Type::ExternalResource;
};

template <class C> struct GetValueType<prime::common::Pointer<C>> {
  using value_type = C;
  static const Container CONTAINER = Container::Pointer;
  // static const Type TYPE = Type::None;
};

template <class C> struct GetValueType<prime::common::LocalPointer<C>> {
  using value_type = C;
  static const Container CONTAINER = Container::LocalPointer;
  // static const Type TYPE = Type::None;
};

template <class... C> struct GetValueType<prime::common::Variant<C...>> {
  static const Container CONTAINER = Container::None;
  static const Type TYPE = Type::Variant;
};

template <class C> consteval DataType GetType() {
  DataType dt{};
  dt.container = GetValueType<C>::CONTAINER;
  dt.size = uint8(sizeof(C));
  dt.alignment = alignof(C);

  if constexpr (GetValueType<C>::CONTAINER == Container::None) {
    if (std::is_floating_point_v<C>) {
      dt.type = Type::Float;
    } else if (std::is_unsigned_v<C>) {
      dt.type = Type::Uint;
    } else if (std::is_signed_v<C>) {
      dt.type = Type::Int;
    } else if (std::is_same_v<C, bool>) {
      dt.type = Type::Bool;
    } else if constexpr (std::is_same_v<glm::vec2, C> ||
                         std::is_same_v<Vector2, C>) {
      dt.type = Type::Vec2;
    } else if constexpr (std::is_same_v<glm::vec3, C> ||
                         std::is_same_v<Vector, C>) {
      dt.type = Type::Vec3;
    } else if constexpr (std::is_same_v<glm::vec4, C> ||
                         std::is_same_v<Vector4, C> ||
                         std::is_same_v<Vector4A16, C>) {
      dt.type = Type::Vec4;
    } else if constexpr (std::is_same_v<glm::quat, C>) {
      dt.type = Type::Quat;
    } else if constexpr (std::is_same_v<Color, C>) {
      dt.type = Type::Color;
    } else if constexpr (std::is_same_v<glm::dualquat, C>) {
      dt.type = Type::DualQuat;
    } else if constexpr (std::is_same_v<common::ResourceHash, C> ||
                         std::is_same_v<JenHash3, C>) {
      dt.type = Type::ExternalResource;
    } else if constexpr (std::is_same_v<JenHash, C>) {
      dt.type = Type::HString;
    } else if constexpr (GetValueType<C>::TYPE == Type::Variant) {
      dt.type = Type::Variant;
    } else if constexpr (GetValueType<C>::TYPE == Type::Flags) {
      dt.type = Type::Flags;
      dt.hash =
          prime::common::GetClassHash<typename GetValueType<C>::value_type>();
      static_assert(
          prime::common::GetClassHash<typename GetValueType<C>::value_type>(),
          "Enum must be registered!");
    } else if constexpr (GetValueType<C>::TYPE == Type::ExternalResource) {
      dt.type = Type::ExternalResource;
      dt.hash =
          prime::common::GetClassHash<typename GetValueType<C>::value_type>();
      static_assert(
          prime::common::GetClassHash<typename GetValueType<C>::value_type>(),
          "Class must be registered!");
    } else if constexpr (std::is_enum_v<C>) {
      dt.type = Type::Enum;
      static_assert(prime::common::GetClassHash<C>(),
                    "Enum must be registered!");
      dt.hash = prime::common::GetClassHash<C>();
    } else if constexpr (std::is_class_v<C> || std::is_union_v<C>) {
      dt.type = Type::Class;
      static_assert(prime::common::GetClassHash<C>(),
                    "Class must be registered!");
      dt.hash = prime::common::GetClassHash<C>();
    }
  } else if constexpr (GetValueType<C>::CONTAINER == Container::Pointer) {
    dt.type = Type::ExternalResource;
    dt.container = Container::Pointer;
    static_assert(
        prime::common::GetClassHash<typename GetValueType<C>::value_type>(),
        "Class must be registered!");
    dt.hash =
        prime::common::GetClassHash<typename GetValueType<C>::value_type>();
  } else if constexpr (GetValueType<C>::CONTAINER == Container::LocalPointer) {
    dt = GetType<typename GetValueType<C>::value_type>();
    dt.container = Container::LocalPointer;
  } else {
    if (GetValueType<typename GetValueType<C>::value_type>::CONTAINER ==
        Container::None) {
      dt = GetType<typename GetValueType<C>::value_type>();
      dt.container = GetValueType<C>::CONTAINER;
    } else {
      dt.type = Type::Subtype;
    }
    dt.count = GetValueType<C>::SIZE;
  }

  return dt;
}

template <class C, size_t N> struct BuildDataTypesDetail;

template <class C> struct BuildDataTypesDetail<C, 0> {
  using Value0 = C;
  inline static constexpr DataType TYPES[]{GetType<Value0>()};
};

template <class... C>
struct BuildDataTypesDetail<prime::common::Variant<C...>, 0> {
  using Value0 = prime::common::Variant<C...>;
  inline static constexpr DataType TYPES[]{
      DataType{
          DataTypeBase{
              .type = Type::Variant,
              .container = Container::None,
              .size = uint8(sizeof(Value0)),
              .alignment = alignof(Value0),
              .count = sizeof...(C),
          },
          0,
      },
      GetType<C>()...
  };
};

template <class C> struct BuildDataTypesDetail<C, 1> {
  using Value0 = C;
  using Value1 = GetValueType<Value0>::value_type;
  inline static constexpr DataType TYPES[]{GetType<Value0>(),
                                           GetType<Value1>()};
};

template <class C> struct BuildDataTypesDetail<C, 2> {
  using Value0 = C;
  using Value1 = GetValueType<Value0>::value_type;
  using Value2 = GetValueType<Value1>::value_type;
  inline static constexpr DataType TYPES[]{GetType<Value0>(), GetType<Value1>(),
                                           GetType<Value2>()};
};

template <class C> struct BuildDataTypesDetail<C, 3> {
  using Value0 = C;
  using Value1 = GetValueType<Value0>::value_type;
  using Value2 = GetValueType<Value1>::value_type;
  using Value3 = GetValueType<Value2>::value_type;
  inline static constexpr DataType TYPES[]{GetType<Value0>(), GetType<Value1>(),
                                           GetType<Value2>(),
                                           GetType<Value3>()};
};

template <class C> consteval size_t GetTypeDepth() {
  constexpr DataType DT = GetType<C>();

  if constexpr (DT.type == Type::Subtype) {
    return GetTypeDepth<typename GetValueType<C>::value_type>() + 1;
  }

  return 0;
};

template <class C>
using BuildDataTypes = BuildDataTypesDetail<C, GetTypeDepth<C>()>;

template <class C>
consteval Member MakeMember(const char *name, uint32 offset) {
  return Member{
      .name = name,
      .types = BuildDataTypes<C>::TYPES,
      .nTypes = GetTypeDepth<C>() + 1,
      .offset = offset,
  };
}

template <class C> class InvokeGuard;

int AddClass(const Class *cls);
int AddEnum(const Enum *enm);

#define BEGIN_CLASS(...)                                                       \
  template <>                                                                  \
  const prime::reflect::Class *                                                \
  prime::reflect::GetReflectedClass<__VA_ARGS__>() {                           \
    using class_type = __VA_ARGS__;                                            \
    static const prime::reflect::Class def((__VA_ARGS__ *)nullptr, #__VA_ARGS__,

#define BEGIN_EMPTYCLASS(...)                                                  \
  template <>                                                                  \
  const prime::reflect::Class *                                                \
  prime::reflect::GetReflectedClass<__VA_ARGS__>() {                           \
    static const prime::reflect::Class def((__VA_ARGS__ *)nullptr, #__VA_ARGS__

#define INVOKE_CLASS(...)                                                      \
  template <> class prime::reflect::InvokeGuard<__VA_ARGS__> {                 \
    static inline const int data = AddClass(GetReflectedClass<__VA_ARGS__>()); \
  };

#define INVOKE_EMPTYCLASS(...) INVOKE_CLASS(__VA_ARGS__)

#define MEMBER(...)                                                            \
  MakeMember<decltype(class_type::__VA_ARGS__)>(                               \
      #__VA_ARGS__, offsetof(class_type, __VA_ARGS__))

#define MEMBER_CAST(member, ...)                                               \
  MakeMember<__VA_ARGS__>(#member, offsetof(class_type, member))

#define BEGIN_ENUM(...)                                                        \
  template <>                                                                  \
  const prime::reflect::Enum *                                                 \
  prime::reflect::GetReflectedEnum<__VA_ARGS__>() {                            \
    using enum_type = __VA_ARGS__;                                             \
    using value_type = std::underlying_type_t<enum_type>;                      \
    static const prime::reflect::Enum def((__VA_ARGS__ *)nullptr, #__VA_ARGS__,

#define INVOKE_ENUM(...)                                                       \
  template <> class prime::reflect::InvokeGuard<__VA_ARGS__> {                 \
    static inline const int data = AddEnum(GetReflectedEnum<__VA_ARGS__>());   \
  };

#define EMEMBER(...)                                                           \
  EnumMember<value_type> {                                                     \
    #__VA_ARGS__, static_cast<value_type>(enum_type::__VA_ARGS__)              \
  }

#define UEMEMBER(...)                                                          \
  EnumMember<value_type> { #__VA_ARGS__, static_cast<value_type>(__VA_ARGS__) }

#define REFLECT(cls, ...) BEGIN_##cls __VA_ARGS__);                            \
  return &def;                                                                 \
  }                                                                            \
  INVOKE_##cls
} // namespace prime::reflect
