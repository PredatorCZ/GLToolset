#include "utils/debug.hpp"
#include <algorithm>

namespace prime::utils {
ResourceDebugPlayground::ResourceDebugPlayground()
    : main(AddClass<ResourceDebug>()) {}

common::ResourceHash ResourceDebugPlayground::AddRef(std::string_view path_,
                                                     uint32 clHash) {
  auto resHash = common::MakeHash<char>(path_);
  resHash.type = clHash;

  auto found = std::find_if(
      main->dependencies.begin(), main->dependencies.end(),
      [&](const ResourceDebugDependency &item) {
        return path_ == std::string_view{item.path.begin(), item.path.end()};
      });

  if (found == main->dependencies.end()) {
    Pointer<ResourceDebugDependency> newDep(ArrayEmplace(main->dependencies));
    NewString(newDep->path, path_);
    ArrayEmplace(newDep->types, clHash);
  } else {
    ArrayEmplace(found->types, clHash);
  }

  return resHash;
}

std::string_view ResourceDebugPlayground::AddString(std::string_view str) {
  auto found =
      std::find_if(main->strings.begin(), main->strings.end(),
                   [&](const common::LocalArray32<char> &item) {
                     return str == std::string_view{item.begin(), item.end()};
                   });

  if (found == main->strings.end()) {
    Pointer<common::LocalArray32<char>> newDep(ArrayEmplace(main->strings));
    NewString(*newDep, str);
  }
  return str;
}

PlayGround::Pointer<ResourceDebugClass>
ResourceDebugPlayground::AddDebugClass(const reflect::Class *cls) {
  Pointer<ResourceDebugClass> newClass = ArrayEmplace(main->classes);
  NewString(newClass->className, std::string_view(cls->className));

  for (uint32_t i = 0; i < cls->nMembers; i++) {
    Pointer<ResourceDebugClassMember> newMember =
        ArrayEmplace(newClass->members);
    const reflect::Member &member = cls->members[i];
    newMember->offset = member.offset;
    NewString(newMember->name, std::string_view(member.name));

    for (uint32 t = 0; t < member.nTypes; t++) {
      Pointer<ResourceDebugDataType> newType = ArrayEmplace(newMember->types);
      *newType = member.types[t];
      if (member.types[t].type == reflect::Type::Class) {
        const reflect::Class *subCls =
            reflect::GetReflectedClass(member.types[t].hash);

        auto found = std::find_if(
            main->classes.begin(), main->classes.end(),
            [subCls](const ResourceDebugClass &cls) {
              return std::string_view(cls.className.begin(),
                                      cls.className.end()) == subCls->className;
            });

        if (found == main->classes.end()) {
          Link(newType->definition, AddDebugClass(subCls).operator->());
        } else {
          Link(newType->definition, found);
        }
      }
    }
  }

  return newClass;
}

std::string ResourceDebugPlayground::Build(PlayGround &base,
                                           const reflect::Class *refClass) {
  std::string retVal = base.Build();
  ResourceDebugFooter footer{.dataSize = uint32(retVal.size())};
  AddDebugClass(refClass);
  const uint32 pad = GetPadding(retVal.size(), alignof(ResourceDebug));
  footer.pad = pad;
  retVal.append(pad, 0);
  retVal.append(PlayGround::Build());
  retVal.append(GetPadding(retVal.size(), alignof(footer)), 0);
  retVal.append(reinterpret_cast<const char *>(&footer), sizeof(footer));

  return retVal;
}
} // namespace prime::utils
