#include "common/resource.hpp"
#include "spike/io/stat.hpp"
#include "utils/reflect.hpp"
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <vector>

namespace pr = prime::reflect;
namespace pc = prime::common;

namespace prime::reflect {
std::map<uint32, const Class *> &GetClasses();
std::map<uint32, const Enum *> &GetEnums();
} // namespace prime::reflect

struct Linked {
  std::vector<const void *> referencedBy;
  std::vector<const void *> references;
  std::stringstream definition;
  bool isEnum = false;
  bool isGenerated = false;
};

std::string autogenFolder;
std::map<const void *, Linked> LINKED_CLASSES;

std::pair<std::string_view, std::string_view>
SplitClassName(std::string_view str) {
  std::string_view part1;
  if (str.starts_with("prime::")) {
    str.remove_prefix(7);
    part1 = str.substr(0, str.find(':'));
    str.remove_prefix(str.find(':') + 2);
  }

  return std::make_pair(part1, str);
}

std::string BuildFileName(const void *item, Linked &lnk) {
  std::string_view name;

  if (lnk.isEnum) {
    name = static_cast<const pr::Enum *>(item)->name;
  } else {
    name = static_cast<const pr::Class *>(item)->className;
  }

  auto pair = SplitClassName(name);
  std::string retVal;

  if (pair.first.size()) {
    retVal.append(pair.first);
    retVal.push_back('.');
  }

  return retVal + std::string(pair.second);
}

bool IsEmpty(const void *item, Linked &lnk) {
  if (lnk.isEnum) {
    return static_cast<const pr::Enum *>(item)->nMembers == 0;
  }
  return static_cast<const pr::Class *>(item)->nMembers == 0;
}

void WalkClasses(std::set<const void *> &items, Linked &lnk,
                 std::ostream &str) {
  for (auto &ref : lnk.references) {
    Linked &clink = LINKED_CLASSES.at(ref);

    if (IsEmpty(ref, clink)) {
      continue;
    }

    if (clink.referencedBy.size() == 1) {
      WalkClasses(items, clink, str);
    } else {
      if (items.count(ref) == 0) {
        items.emplace(ref);
        str << "dofile(\"" << BuildFileName(ref, clink) << "\")\n";
      }

      if (!clink.isGenerated) {
        clink.isGenerated = true;
        std::ofstream cstr(autogenFolder + BuildFileName(ref, clink) + ".nut");
        std::set<const void *> citems;
        WalkClasses(citems, clink, cstr);
      }
    }
  }

  str << lnk.definition.str();
}

pc::Return<const pr::Class *> RequestClass(uint32 hash,
                                           const pr::Class *parent) {
  return pr::GetReflectedClass(hash).Success([&](const pr::Class *cls) {
    LINKED_CLASSES[cls].referencedBy.emplace_back(parent);
    LINKED_CLASSES[parent].references.emplace_back(cls);
  });
}

pc::Return<const pr::Enum *> RequestEnum(uint32 hash, const pr::Class *parent) {
  return pr::GetReflectedEnum(hash).Success([&](const pr::Enum *cls) {
    LINKED_CLASSES[cls].referencedBy.emplace_back(parent);
    LINKED_CLASSES[parent].references.emplace_back(cls);
    LINKED_CLASSES[cls].isEnum = true;
  });
}

void AppendClassName(std::ostream &str, std::string_view clName) {
  auto parts = SplitClassName(clName);
  if (parts.first.size()) {
    str << parts.first << '.';
  }

  str << parts.second;
};

void AppendEnumName(std::ostream &str, std::string_view clName) {
  auto parts = SplitClassName(clName);
  str << parts.second;
};

void MakeType(std::ostream &str, const pr::DataType *types, uint32 idx,
              const pr::Class *owner) {
  const pr::DataType &type = types[idx];

  if (type.type == pr::Type::Subtype) {
    str << '[';
    MakeType(str, types, idx + 1, owner);
    str << ']';
    return;
  }

  if (type.type == pr::Type::Variant) {
    for (uint32 i = 0; i < type.count; i++) {
      MakeType(str, types, i + 1, owner);
      str << ", ";
    }

    return;
  }

  switch (type.type) {
  case pr::Type::Bool:
    str << "bool";
    break;
  case pr::Type::Int:
    str << "int";
    break;
  case pr::Type::Uint:
    str << "uint";
    break;
  case pr::Type::Float:
    str << "float";
    break;
  case pr::Type::DualQuat:
    str << "dualquat";
    break;
  case pr::Type::Color:
    str << "color";
    break;
  case pr::Type::Quat:
    str << "quat";
    break;
  case pr::Type::Vec2:
    str << "vec2";
    break;
  case pr::Type::Vec3:
    str << "vec3";
    break;
  case pr::Type::Vec4:
    str << "vec4";
    break;
  case pr::Type::ExternalResource:
    str << "ExternalResource";
    if (type.hash) {
      str << '<';
      RequestClass(type.hash, owner)
          .Either(
              [&](const pr::Class *cls) {
                AppendClassName(str, cls->className);
              },
              [&] { AppendClassName(str, "__UNREGISTERED__"); })
          .Unused();
      str << '>';
    }
    break;
  case pr::Type::Class:
    RequestClass(type.hash, owner)
        .Either(
            [&](const pr::Class *cls) { AppendClassName(str, cls->className); },
            [&] { AppendClassName(str, "__UNREGISTERED__"); })
        .Unused();
    break;
  case pr::Type::Enum:
    RequestEnum(type.hash, owner)
        .Either([&](const pr::Enum *cls) { AppendEnumName(str, cls->name); },
                [&] { AppendEnumName(str, "__UNREGISTERED__"); })
        .Unused();
    break;
  case pr::Type::Flags:
    pr::GetReflectedEnum(type.hash)
        .Either([&](const pr::Enum *cls) { AppendClassName(str, cls->name); },
                [&] { AppendClassName(str, "__UNREGISTERED__"); })
        .Unused();
    str << "{}";
    break;
  case pr::Type::HString:
  case pr::Type::String:
    str << "string";
    break;
  case pr::Type::None:
    break;
  }

  switch (type.container) {
  case pr::Container::Array:
    str << "[]";
    break;
  case pr::Container::InlineArray:
    str << '[' << type.count << ']';
    break;
  default:
    break;
  }
}

void DumpClass(const pr::Class *cls) {
  std::ostream &str = LINKED_CLASSES[cls].definition;
  std::string_view className(cls->className);
  str << "class ";

  char instanceBuffer[0xffff]{};
  std::string instanceHeap;
  char *instanceData = nullptr;

  if (cls->construct) {
    instanceData = instanceBuffer;
    if (cls->classSize > sizeof(instanceBuffer)) {
      instanceHeap.resize(cls->classSize);
      instanceData = instanceHeap.data();
    }
    cls->construct(instanceData);
  }

  AppendClassName(str, className);

  str << " {\n";

  if (cls->extension.raw) {
    str << R"(  constructor(fileName, params) {
    foreach (k,v in params) {
      this[k] = v
    }
    ::AddResource(fileName, this, 0x)"
        << std::hex << JenkinsHash_(className) << std::dec << R"()
  }
)";
  }

  for (uint32 i = 0; i < cls->nMembers; i++) {
    const pr::Member &member = cls->members[i];

    str << "  " << member.name << " = ";

    const pr::DataType &type = member.types[0];

    auto GetValue = [&](const pr::DataType &type, std::ostream &str) {
      int64 retVal = 0;

      if (instanceData) {
        memcpy(&retVal, instanceData + member.offset, type.size);
      }

      switch (type.type) {
      case pr::Type::Bool:
        str << (retVal ? "true" : "false");
        break;

      case pr::Type::Float:
        str << reinterpret_cast<const float &>(retVal);
        break;

      case pr::Type::Uint:
        str << uint64(retVal);
        break;

      case pr::Type::Int:
        retVal <<= (8 - type.size) * 8;
        retVal >>= (8 - type.size) * 8;
        str << retVal;
        break;

      case pr::Type::Enum:
        RequestEnum(type.hash, cls)
            .Either(
                [&](const pr::Enum *enums) {
                  AppendEnumName(str, enums->name);
                  bool found = false;

                  for (uint32 v = 0; v < enums->nMembers; v++) {
                    if (!memcmp(&retVal,
                                static_cast<const char *>(enums->values) +
                                    v * enums->size,
                                enums->size)) {
                      str << '.' << enums->names[v];
                      found = true;
                      break;
                    }
                  }

                  if (!found) {
                    str << '.' << enums->names[0];
                  }
                },
                [&] { AppendEnumName(str, "__UNREGISTERED__"); })
            .Unused();
        break;

      default:
        break;
      }
    };

    switch (type.container) {
    case pr::Container::Array:
    case pr::Container::InlineArray:
      str << "[]  // ";
      MakeType(str, member.types, 0, cls);
      break;

    case pr::Container::Pointer:
      str << "\"";
      AppendClassName(str, pr::GetReflectedClass(type.hash).retVal->className);
      str << ":\"";
      break;

    default:
      switch (type.type) {
      case pr::Type::Bool:
      case pr::Type::Int:
      case pr::Type::Uint:
      case pr::Type::Float:
      case pr::Type::Enum:
        GetValue(type, str);
        break;
      case pr::Type::ExternalResource:
        str << '"';
        if (type.hash) {
          AppendClassName(str,
                          pr::GetReflectedClass(type.hash).retVal->className);
        } else {
          str << "none";
        }
        str << ":\"";
        break;
      case pr::Type::Quat:
        str << "DEFAULT_ROTATION";
        break;
      case pr::Type::Vec2:
        str << "DEFAULT_VECTOR2";
        break;
      case pr::Type::Vec3:
        str << "DEFAULT_VECTOR3";
        break;
      case pr::Type::Vec4:
        str << "DEFAULT_VECTOR4";
        break;
      case pr::Type::DualQuat:
        str << "DEFAULT_TRANSFORM";
        break;
      case pr::Type::Color:
        str << "COLOR_WHITE";
        break;
      case pr::Type::Variant:
        str << "null // variant<";
        MakeType(str, member.types, 0, cls);
        str.seekp(size_t(str.tellp()) - 2);
        str << '>';
        break;
      case pr::Type::Flags: {
        str << "{\n";
        pr::GetReflectedEnum(type.hash)
            .Either(
                [&](const pr::Enum *enums) {
                  for (uint32 i = 0; i < enums->nMembers; i++) {
                    str << "    " << enums->names[i] << " = false\n";
                  }
                },
                [&] { str << "    __UNREGISTERED__\n"; })
            .Unused();
        str << "  }";
        break;
      }
      case pr::Type::Class:
        RequestClass(type.hash, cls)
            .Either(
                [&](const pr::Class *cls) {
                  AppendClassName(str, cls->className);
                },
                [&] { AppendClassName(str, "__UNREGISTERED__"); })
            .Unused();
        str << "()";
        break;
      case pr::Type::HString:
      case pr::Type::String: {
        str << "\"\"";
        break;
      }
      default:
        break;
      }

      break;
    }

    str << '\n';
  }

  str << "}\n";
}

void DumpEnum(const pr::Enum *enums) {
  std::ostream &str = LINKED_CLASSES[enums].definition;
  LINKED_CLASSES[enums].isEnum = true;
  // str << "enum ";
  AppendEnumName(str, enums->name);
  str << " <- {\n";

  auto Dump = [enums, &str](auto type, const char *fmt) {
    const decltype(type) *values =
        static_cast<const decltype(type) *>(enums->values);

    for (uint32 i = 0; i < enums->nMembers; i++) {
      char buffer[0x10];
      snprintf(buffer, sizeof(buffer), fmt, values[i]);
      str << "  " << enums->names[i] << " = " << buffer << '\n';
    }
  };

  if (enums->isSigned) {
    switch (enums->size) {
    case 1:
      Dump(int8(), "%" PRIi8);
      break;
    case 2:
      Dump(int16(), "%" PRIi16);
      break;
    case 4:
      Dump(int32(), "%" PRIi32);
      break;
    case 8:
      Dump(int64(), "%" PRIi64);
      break;

    default:
      break;
    }
  } else {
    switch (enums->size) {
    case 1:
      Dump(uint8(), "0x%" PRIX8);
      break;
    case 2:
      Dump(uint16(), "0x%" PRIX16);
      break;
    case 4:
      Dump(uint32(), "0x%" PRIX32);
      break;
    case 8:
      Dump(uint64(), "0x%" PRIX64);
      break;

    default:
      break;
    }
  }

  str << "}\n";
}

namespace prime::script {
void AutogenerateScriptClasses() {
  autogenFolder = common::ProjectDataFolder() + "autogenerated/";
  es::mkdir(autogenFolder);
  auto &enums = pr::GetEnums();

  for (auto &c : enums) {
    DumpEnum(c.second);
  }

  auto &classes = pr::GetClasses();

  for (auto &c : classes) {
    DumpClass(c.second);
  }

  for (auto &[ptr, link] : LINKED_CLASSES) {
    if (IsEmpty(ptr, link)) {
      continue;
    }
    if (link.referencedBy.empty() && !link.isEnum) {
      std::ofstream str(autogenFolder + BuildFileName(ptr, link) + ".nut");
      std::set<const void *> items;
      WalkClasses(items, link, str);
    }
  }

  LINKED_CLASSES.clear();
}
} // namespace prime::script
