#include "utils/reflect_impl.hpp"
#include <map>

namespace prime::reflect {
enum SimpleEnum { member };
}
HASH_CLASS(prime::common::LocalArray16<int[1]>);
HASH_CLASS(int[1]);
HASH_CLASS(prime::reflect::SimpleEnum);

namespace prime::reflect {

static_assert(GetType<uint32>().type == Type::Uint);
static_assert(GetType<int32>().type == Type::Int);
static_assert(GetType<float>().type == Type::Float);

consteval bool operator==(const DataTypeBase dt0, const DataTypeBase dt1) {
  return dt0.type == dt1.type && dt0.container == dt1.container &&
         dt0.size == dt1.size && dt0.alignment == dt1.alignment &&
         dt0.count == dt1.count;
}

static_assert(GetType<float[0x1587f]>() ==
              DataTypeBase{.type = Type::Float,
                           .container = Container::InlineArray,
                           .size = 4,
                           .alignment = 4,
                           .count = 0x1587f});

static_assert(GetType<common::LocalArray16<uint32>>() ==
              DataTypeBase{.type = Type::Uint,
                           .container = Container::Array,
                           .size = 4,
                           .alignment = 4,
                           .count = 2});

static_assert(GetType<common::LocalArray32<int16>>() ==
              DataTypeBase{.type = Type::Int,
                           .container = Container::Array,
                           .size = 2,
                           .alignment = 2,
                           .count = 4});

static_assert(GetType<common::LocalArray32<common::LocalArray16<char>>>() ==
              DataTypeBase{.type = Type::Subtype,
                           .container = Container::Array,
                           .size = 12,
                           .alignment = 4,
                           .count = 4});

static_assert(GetType<SimpleEnum>() ==
              DataTypeBase{.type = Type::Enum,
                           .container = Container::None,
                           .size = 4,
                           .alignment = 4,
                           .count = 0});

static_assert(GetTypeDepth<int[1]>() == 0);
static_assert(GetTypeDepth<common::LocalArray16<int[1]>>() == 1);
static_assert(
    GetTypeDepth<common::LocalArray16<common::LocalArray16<int[1]>>>() == 2);

static_assert(BuildDataTypes<float[0x1587f]>::TYPES[0] ==
              DataTypeBase{.type = Type::Float,
                           .container = Container::InlineArray,
                           .size = 4,
                           .alignment = 4,
                           .count = 0x1587f});

static_assert(BuildDataTypes<common::LocalArray16<int[1]>>::TYPES[0] ==
              DataTypeBase{.type = Type::Subtype,
                           .container = Container::Array,
                           .size = 8,
                           .alignment = 4,
                           .count = 2});

static_assert(BuildDataTypes<common::LocalArray16<int[1]>>::TYPES[1] ==
              DataTypeBase{.type = Type::Int,
                           .container = Container::InlineArray,
                           .size = 4,
                           .alignment = 4,
                           .count = 1});

static_assert(
    BuildDataTypes<
        common::LocalArray32<common::LocalArray16<int[1]>>>::TYPES[0] ==
    DataTypeBase{.type = Type::Subtype,
                 .container = Container::Array,
                 .size = 12,
                 .alignment = 4,
                 .count = 4});

static_assert(
    BuildDataTypes<
        common::LocalArray32<common::LocalArray16<int[1]>>>::TYPES[1] ==
    DataTypeBase{.type = Type::Subtype,
                 .container = Container::Array,
                 .size = 8,
                 .alignment = 4,
                 .count = 2});

static_assert(
    BuildDataTypes<
        common::LocalArray32<common::LocalArray16<int[1]>>>::TYPES[2] ==
    DataTypeBase{.type = Type::Int,
                 .container = Container::InlineArray,
                 .size = 4,
                 .alignment = 4,
                 .count = 1});

std::map<uint32, const Class *> &GetClasses() {
  static std::map<uint32, const Class *> CLASSES;
  return CLASSES;
}

int AddClass(const prime::reflect::Class *cls) {
  return GetClasses().emplace(JenkinsHash_(cls->className), cls).second;
}

common::Return<const Class *> GetReflectedClass(uint32 hash) {
  return common::MapGetOr(GetClasses(), hash, [hash]{
    common::RuntimeError("Class 0x%X is not registered!", hash);
    return nullptr;
  });
}

std::map<uint32, const Enum *> &GetEnums() {
  static std::map<uint32, const Enum *> ENUMS;
  return ENUMS;
}

int AddEnum(const prime::reflect::Enum *enm) {
  return GetEnums().emplace(JenkinsHash_(enm->name), enm).second;
}

common::Return<const Enum *> GetReflectedEnum(uint32 hash) {
  return common::MapGetOr(GetEnums(), hash, []{
    common::RuntimeError("Enum is not registered!");
    return nullptr;
  });
}

} // namespace prime::reflect
