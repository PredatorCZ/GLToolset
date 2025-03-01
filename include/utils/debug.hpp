#include "common/local_pointer.hpp"
#include "common/resource.hpp"
#include "common/string.hpp"
#include "reflect.hpp"
#include "spike/util/supercore.hpp"
#include "utils/playground.hpp"

namespace prime::utils {
struct ResourceDebugDependency;
struct ResourceDebug;
struct ResourceDebugClassMember;
struct ResourceDebugClass;
struct ResourceDebugDataType;
} // namespace prime::utils

CLASS_EXT(prime::utils::ResourceDebug);
HASH_CLASS(prime::utils::ResourceDebugDependency);
HASH_CLASS(prime::utils::ResourceDebugClassMember);
HASH_CLASS(prime::utils::ResourceDebugClass);
HASH_CLASS(prime::utils::ResourceDebugDataType);

namespace prime::utils {
struct ResourceDebugDependency {
  common::String path;
  common::LocalArray32<uint32> types;
};

struct ResourceDebugDataType : reflect::DataTypeBase {
  using reflect::DataTypeBase::operator=;
  common::LocalVariantPointer<ResourceDebugClass> definition;
};

struct ResourceDebugClassMember {
  common::LocalArray32<ResourceDebugDataType> types;
  common::String name;
  uint32 offset;
};

struct ResourceDebugClass {
  common::LocalArray32<ResourceDebugClassMember> members;
  common::String className;
};

struct ResourceDebugFooter {
  static const uint32 ID = common::GetClassExtension<ResourceDebug>().raw;
  uint32 pad : 8 = 0;
  uint32 dataSize : 24;
  uint32 id = ID;
};

struct ResourceDebug {
  common::LocalArray32<ResourceDebugDependency> dependencies;
  common::LocalArray32<common::String> strings;
  common::LocalArray32<ResourceDebugClass> classes;
  common::LocalPointer<common::ResourceBase> converter;
  uint32 inputCrc = 0;
};

inline ResourceDebug *LocateDebug(char *data, size_t dataSize) {
  ResourceDebugFooter *footer = reinterpret_cast<ResourceDebugFooter *>(
      data + dataSize - sizeof(ResourceDebugFooter));

  if (footer->id != ResourceDebugFooter::ID) {
    return nullptr;
  }

  return reinterpret_cast<ResourceDebug *>(data + footer->dataSize +
                                           footer->pad);
}

struct ResourceDebugPlayground : PlayGround {
  Pointer<ResourceDebug> main;

  ResourceDebugPlayground();
  common::ResourceHash AddRef(std::string_view path_, uint32 clHash);
  std::string_view AddString(std::string_view str);

  template <class C> std::string Build(PlayGround &base) {
    return Build(base, reflect::GetReflectedClass<C>());
  }

  std::string Build(PlayGround &base, const reflect::Class *refClass);

private:
  Pointer<ResourceDebugClass> AddDebugClass(const reflect::Class *cls);
};
} // namespace prime::utils
