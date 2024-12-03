#include "common/resource.hpp"
#include "resource_classes.hpp"
#include "script/funcproto.hpp"
#include "spike/crypto/crc32.hpp"
#include "spike/io/stat.hpp"
#include "squirrel.h"
#include "utils/debug.hpp"
#include "utils/reflect.hpp"
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <glm/gtx/dual_quaternion.hpp>

#include "sqbase/sqclosure.h"
#include "sqbase/sqfuncproto.h"
#include "sqbase/sqfuncstate.h"
#include "sqbase/sqtable.h"
#include "sqbase/sqvm.h"

void printfunc(HSQUIRRELVM SQ_UNUSED_ARG(v), const SQChar *s, ...);

void CompilerError(HSQUIRRELVM v, const SQChar *sErr, const SQChar *sSource,
                   SQInteger line, SQInteger column);

SQInteger RuntimeError(HSQUIRRELVM v);

static std::string autogenFolder;
static std::string cacheFolder;
static std::string autogenCacheFolder;

template <class C> SQInteger sq_cast(HSQUIRRELVM v, SQUserPointer tag, C &out) {
  SQUserPointer self = nullptr;
  if (SQ_FAILED(sq_getinstanceup(v, -1, &self, tag, SQFalse))) {
    return sq_throwerror(v, _SC("invalid type tag"));
  }
  if (self == nullptr) {
    return sq_throwerror(v, _SC("invalid object"));
  }

  out = *static_cast<C *>(self);

  return SQ_OK;
}

#include <variant>

using Addr = prime::utils::PlayGround::Pointer<char>;

void SetClass(const prime::reflect::Class *cls, Addr addr, HSQUIRRELVM v,
              prime::utils::PlayGround &pg,
              prime::utils::ResourceDebugPlayground &dbg);

const char *GetObjectType(SQObjectType type) {
  switch (type) {
  case OT_NULL:
    return "NULL";
  case OT_INTEGER:
    return "INTEGER";
  case OT_FLOAT:
    return "FLOAT";
  case OT_BOOL:
    return "BOOL";
  case OT_STRING:
    return "STRING";
  case OT_TABLE:
    return "TABLE";
  case OT_ARRAY:
    return "ARRAY";
  case OT_USERDATA:
    return "USERDATA";
  case OT_CLOSURE:
    return "CLOSURE";
  case OT_NATIVECLOSURE:
    return "NATIVECLOSURE";
  case OT_GENERATOR:
    return "GENERATOR";
  case OT_USERPOINTER:
    return "USERPOINTER";
  case OT_THREAD:
    return "THREAD";
  case OT_FUNCPROTO:
    return "FUNCPROTO";
  case OT_CLASS:
    return "CLASS";
  case OT_INSTANCE:
    return "INSTANCE";
  case OT_WEAKREF:
    return "WEAKREF";
  case OT_OUTER:
    return "OUTER";
  }

  return "unknown";
}

void CheckType(HSQUIRRELVM v, SQObjectType type) {
  if (SQObjectType iType = sq_gettype(v, -1); iType != type) {
    throw std::runtime_error("Unexpected type " +
                             std::string(GetObjectType(iType)) + ", expected " +
                             std::string(GetObjectType(type)));
  }
}

std::pair<std::string_view, std::string_view>
SplitClassName(std::string_view str);

struct StackGuard {
  StackGuard(HSQUIRRELVM v_) : v(v_), top(sq_gettop(v)) {}
  ~StackGuard() {
    if (top != sq_gettop(v)) {
      throw std::runtime_error("Unexpected stack top");
    }
  }

  HSQUIRRELVM v;
  SQInteger top;
};

bool SetVariantClass(HSQUIRRELVM v, Addr addr_, uint32 hash,
                     prime::utils::PlayGround &pg,
                     prime::utils::ResourceDebugPlayground &dbg) {
  StackGuard g(v);
  const SQChar *className;
  SQUserPointer typeTag;
  sq_getclass(v, -1);
  sq_getclassname(v, &className);
  sq_gettypetag(v, -1, &typeTag);
  sq_poptop(v);
  void *addr = addr_.operator->();

  if (typeTag == SQCOLOR_TAG) {
    SQUserPointer up;
    sq_getinstanceup(v, -1, &up, SQCOLOR_TAG, SQFalse);
    memcpy(addr, up, 4);
    return true;
  }
  if (typeTag == SQTRANSFORM_TAG) {
    SQUserPointer up;
    sq_getinstanceup(v, -1, &up, SQTRANSFORM_TAG, SQFalse);
    memcpy(addr, up, 32);
    return true;
  }
  if (typeTag == SQROTATION_TAG) {
    SQUserPointer up;
    sq_getinstanceup(v, -1, &up, SQROTATION_TAG, SQFalse);
    memcpy(addr, up, 16);
    return true;
  }
  if (typeTag == SQVEC2_TAG) {
    SQUserPointer up;
    sq_getinstanceup(v, -1, &up, SQVEC2_TAG, SQFalse);
    memcpy(addr, up, 8);
    return true;
  }
  if (typeTag == SQVEC3_TAG) {
    SQUserPointer up;
    sq_getinstanceup(v, -1, &up, SQVEC3_TAG, SQFalse);
    memcpy(addr, up, 12);
    return true;
  }
  if (typeTag == SQVEC4_TAG) {
    SQUserPointer up;
    sq_getinstanceup(v, -1, &up, SQVEC4_TAG, SQFalse);
    memcpy(addr, up, 16);
    return true;
  }

  const prime::reflect::Class *cls = prime::reflect::GetReflectedClass(hash);
  auto splicClassName = SplitClassName(cls->className);
  /*std::string tClassName(splicClassName.first);
  tClassName.push_back('.');
  tClassName.append(splicClassName.second);*/

  if (splicClassName.second == className) {
    cls->construct(addr);
    SetClass(cls, addr_, v, pg, dbg);
    return true;
  }

  return false;
}

void SetVariant(HSQUIRRELVM v, const prime::reflect::DataType *type,
                const prime::reflect::DataType *types, Addr addr_,
                prime::utils::PlayGround &pg,
                prime::utils::ResourceDebugPlayground &dbg) {
  StackGuard g(v);
  namespace pr = prime::reflect;
  SQObjectType vType = sq_gettype(v, -1);
  const uint32 nTypes = type->count;
  void *addr = addr_.operator->();

  switch (vType) {
  case OT_BOOL: {
    for (uint32 i = 0; i < nTypes; i++) {
      if (types[i].type == pr::Type::Bool) {
        SQBool asBool;
        sq_getbool(v, -1, &asBool);
        *static_cast<bool *>(addr) = asBool;
        *(static_cast<char *>(addr) + type->size - type->alignment) = i;
        return;
      }
    }
    break;
  }

  case OT_INTEGER: {
    for (uint32 i = 0; i < nTypes; i++) {
      switch (types[i].type) {
      case pr::Type::Int:
      case pr::Type::Uint:
      case pr::Type::Enum: {
        SQInteger asInt;
        sq_getinteger(v, -1, &asInt);
        memcpy(addr, &asInt, types[i].size);
        *(static_cast<char *>(addr) + type->size - type->alignment) = i;
        return;
      }

      default:
        break;
      }
    }
    break;
  }

  case OT_FLOAT: {
    for (uint32 i = 0; i < nTypes; i++) {
      if (types[i].type == pr::Type::Float) {
        SQFloat asFloat;
        sq_getfloat(v, -1, &asFloat);
        if (types[i].size == 4) {
          *static_cast<float *>(addr) = asFloat;
        } else {
          *static_cast<double *>(addr) = asFloat;
        }
        *(static_cast<char *>(addr) + type->size - type->alignment) = i;
        return;
      }
    }
    break;
  }

  case OT_INSTANCE: {
    for (uint32 i = 0; i < nTypes; i++) {
      if (types[i].type == pr::Type::Class &&
          SetVariantClass(v, addr_, types[i].hash, pg, dbg)) {
        *(static_cast<char *>(addr) + type->size - type->alignment) = i;
        return;
      }
    }
    break;
  }

  default:
    break;
  }

  throw std::runtime_error("Type " + std::string(GetObjectType(vType)) +
                           " is not part of variant.");
}

static prime::common::ResourcePath GetResourcePath(std::string_view path,
                                                   uint32 hash) {
  const size_t found = path.find(':');

  if (found == path.npos) {
    if (hash == 0) {
      throw std::runtime_error("Expected <class>:<path> but got: " +
                               std::string(path));
    }

    prime::common::ResourcePath retVal{
        .hash = prime::common::ResourceHash(JenkinsHash3_(path), hash),
        .localPath = std::string(path),
    };

    return retVal;
  }

  std::string className("prime::");
  for (auto sub = path.substr(0, found); char c : sub) {
    if (c == '.') {
      className.append("::");
    } else {
      className.push_back(c);
    }
  }

  const uint32 classHash = JenkinsHash_(className);

  try {
    prime::common::GetClassHandle(classHash);
  } catch (...) {
    throw std::runtime_error("Invalid class name: " + std::string(className));
  }

  if (hash) {
    if (hash != classHash) {
      const prime::reflect::Class *cls =
          prime::reflect::GetReflectedClass(hash);
      if (cls) {
        throw std::runtime_error(
            "Expected class: " + std::string(cls->className) +
            " but got: " + className);
      }
      throw std::runtime_error("Class name: " + std::string(className) +
                               " is not expected");
    }
  } else {
    if (!prime::reflect::GetReflectedClass(hash)) {
      throw std::runtime_error("Class name: " + std::string(className) +
                               " is not expected");
    }
  }

  std::string_view pathPart(path.substr(found + 1));

  prime::common::ResourcePath retVal{
      .hash = prime::common::ResourceHash(JenkinsHash3_(pathPart), classHash),
      .localPath = std::string(pathPart),
  };

  return retVal;
}

void SetPrimitive(const prime::reflect::DataType *types, uint32 nTypes,
                  uint32 cType, Addr addr_, HSQUIRRELVM v,
                  prime::utils::PlayGround &pg,
                  prime::utils::ResourceDebugPlayground &dbg) {
  namespace pr = prime::reflect;
  StackGuard g(v);
  void *addr = addr_.operator->();
  switch (types[cType].type) {
  case pr::Type::Bool: {
    SQBool sBool;
    sq_getbool(v, -1, &sBool);
    CheckType(v, OT_BOOL);
    // sq_poptop(v);
    *static_cast<bool *>(addr) = sBool;
    break;
  }
  case pr::Type::Int:
  case pr::Type::Uint:
  case pr::Type::Enum: {
    SQInteger sInt;
    sq_getinteger(v, -1, &sInt);
    CheckType(v, OT_INTEGER);
    // sq_poptop(v);

    // todo check limits
    memcpy(addr, &sInt, types[cType].size);
    break;
  }
  case pr::Type::Float: {
    SQFloat sFloat;
    sq_getfloat(v, -1, &sFloat);
    CheckType(v, OT_FLOAT);
    // sq_poptop(v);
    if (types[cType].size == 4) {
      *static_cast<float *>(addr) = sFloat;
    } else {
      *static_cast<double *>(addr) = sFloat;
    }
    break;
  }

  case pr::Type::Color: {
    sq_cast(v, SQCOLOR_TAG, *static_cast<Color *>(addr));
    break;
  }
  case pr::Type::DualQuat: {
    sq_cast(v, SQTRANSFORM_TAG, *static_cast<glm::dualquat *>(addr));
    break;
  }
  case pr::Type::Quat: {
    sq_cast(v, SQROTATION_TAG, *static_cast<glm::quat *>(addr));
    break;
  }
  case pr::Type::Vec2: {
    sq_cast(v, SQVEC2_TAG, *static_cast<glm::vec2 *>(addr));
    break;
  }
  case pr::Type::Vec3: {
    sq_cast(v, SQVEC3_TAG, *static_cast<glm::vec3 *>(addr));
    break;
  }
  case pr::Type::Vec4: {
    sq_cast(v, SQVEC4_TAG, *static_cast<glm::vec4 *>(addr));
    break;
  }
  case pr::Type::ExternalResource: {
    const SQChar *path;
    sq_getstring(v, -1, &path);
    CheckType(v, OT_STRING);
    const uint32 hash = types[cType].hash;
    std::string_view swPath(path);

    if (types[cType].size == sizeof(JenHash3)) {
      *static_cast<JenHash3 *>(addr) =
          dbg.AddRef(swPath, types[cType].hash).name;
      break;
    }

    const prime::common::ResourcePath rPath = GetResourcePath(swPath, hash);

    if (hash) {
      *static_cast<JenHash3 *>(addr) = dbg.AddRef(rPath.localPath, hash).name;
    } else {
      prime::common::ResourceHash hValue(
          dbg.AddRef(rPath.localPath, rPath.hash.type));
      *static_cast<prime::common::ResourceHash *>(addr) = hValue;
    }

    break;
  }

  case pr::Type::HString: {
    const SQChar *str;
    sq_getstring(v, -1, &str);
    CheckType(v, OT_STRING);
    *static_cast<JenHash *>(addr) = dbg.AddString(str);
    break;
  }

  case pr::Type::String: {
    const SQChar *str;
    sq_getstring(v, -1, &str);
    CheckType(v, OT_STRING);
    pg.NewString(*static_cast<prime::common::String *>(addr), str);
    break;
  }

  case pr::Type::Flags: {
    const pr::Enum *enm = pr::GetReflectedEnum(types[cType].hash);

    if (!enm) {
      throw std::logic_error("Forgot to reflect an enum");
    }

    sq_get(v, -1);
    CheckType(v, OT_TABLE);
    sq_pushnull(v);
    while (SQ_SUCCEEDED(sq_next(v, -2))) {
      const SQChar *key;
      SQBool value;
      sq_getstring(v, -2, &key);
      sq_getbool(v, -1, &value);
      CheckType(v, OT_BOOL);
      sq_pop(v, 2);

      std::string_view sw(key);
      bool found = false;

      for (uint32 i = 0; i < enm->nMembers; i++) {
        if (sw == enm->names[i]) {
          found = true;
          uint64 value = 0;
          const void *valuePtr =
              static_cast<const char *>(enm->values) + i * enm->size;
          memcpy(&value, valuePtr, enm->size);
          *static_cast<uint64 *>(addr) |= uint64(1) << value;
          break;
        }
      }

      if (!found) {
        throw std::runtime_error(
            "Enum value: " + std::string(sw) +
            " not found for enum: " + std::string(enm->name));
      }
    }
    sq_pop(v, 2);
    break;
  }

  case pr::Type::Class: {
    const SQChar *className;
    CheckType(v, OT_INSTANCE);
    sq_getclass(v, -1);
    sq_getclassname(v, &className);
    sq_poptop(v);
    const pr::Class *cls = pr::GetReflectedClass(types[cType].hash);
    if (!cls) {
      throw std::runtime_error("Class name: " + std::string(className) +
                               " is not expected");
    }

    cls->construct(addr);
    SetClass(cls, addr_, v, pg, dbg);
    break;
  }

  case pr::Type::Variant: {
    SetVariant(v, types, types + 1, addr_, pg, dbg);
    break;
  }
  }
}

void SetObject(const prime::reflect::DataType *types, uint32 nTypes,
               uint32 cType, Addr addr, HSQUIRRELVM v,
               prime::utils::PlayGround &pg,
               prime::utils::ResourceDebugPlayground &dbg) {
  namespace pr = prime::reflect;
  StackGuard g(v);
  switch (types[cType].container) {
  case pr::Container::None:
    SetPrimitive(types, nTypes, cType, addr, v, pg, dbg);
    break;
  case pr::Container::Array:
    /*case pr::Container::InlineArray: */ {
      // sq_get(v, -1);
      CheckType(v, OT_ARRAY);
      sq_pushnull(v);

      if (types[cType].count == 2) {
        while (SQ_SUCCEEDED(sq_next(v, -2))) {
          const uint32 nType =
              types[cType].type == pr::Type::Subtype ? cType + 1 : cType;
          auto ptr = pg.ArrayEmplaceBytes(
              *reinterpret_cast<prime::common::LocalArray16<char> *>(
                  addr.operator->()),
              types[nType].size, types[nType].alignment);

          if (types[cType].type == pr::Type::Subtype) {
            SetObject(types, nTypes, nType, ptr, v, pg, dbg);
          } else {
            SetPrimitive(types, nTypes, nType, ptr, v, pg, dbg);
          }
          sq_pop(v, 2);
        }
      } else {
        while (SQ_SUCCEEDED(sq_next(v, -2))) {
          const uint32 nType =
              types[cType].type == pr::Type::Subtype ? cType + 1 : cType;
          auto ptr = pg.ArrayEmplaceBytes(
              *reinterpret_cast<prime::common::LocalArray32<char> *>(
                  addr.operator->()),
              types[nType].size, types[nType].alignment);

          if (types[cType].type == pr::Type::Subtype) {
            SetObject(types, nTypes, nType, ptr, v, pg, dbg);
          } else {
            SetPrimitive(types, nTypes, nType, ptr, v, pg, dbg);
          }
          sq_pop(v, 2);
        }
      }

      sq_poptop(v);
      break;
    }

  case pr::Container::Pointer: {
    if (types[cType].type != pr::Type::ExternalResource) {
      throw std::runtime_error(
          "Only ExternalResource for Container::Pointer is supported.");
    }

    const SQChar *path;
    sq_getstring(v, -1, &path);
    CheckType(v, OT_STRING);
    const uint32 hash = types[cType].hash;
    const prime::common::ResourcePath rPath = GetResourcePath(path, hash);

    if (rPath.localPath.size()) {
      auto &ptr =
          *reinterpret_cast<prime::common::Pointer<char> *>(addr.operator->());
      ptr = dbg.AddRef(rPath.localPath, hash);
    }

    break;
  }
  default:
    break;
  }
}

void SetClass(const prime::reflect::Class *cls, Addr addr, HSQUIRRELVM v,
              prime::utils::PlayGround &pg,
              prime::utils::ResourceDebugPlayground &dbg) {
  namespace pr = prime::reflect;
  StackGuard g(v);
  for (uint32 i = 0; i < cls->nMembers; i++) {
    const pr::Member &member = cls->members[i];

    sq_pushstring(v, member.name, -1);
    if (SQ_SUCCEEDED(sq_get(v, -2))) {
      auto addr_ = addr;
      addr_.offset += member.offset;
      SetObject(member.types, member.nTypes, 0, addr_, v, pg, dbg);
      sq_poptop(v);
    }
  }
}

static SQInteger AddResource(HSQUIRRELVM v) {
  StackGuard g(v);
  const SQChar *path;
  sq_getstring(v, 2, &path);
  SQInteger classHash;
  sq_getinteger(v, 4, &classHash);
  std::string oPath(path);
  oPath.push_back('.');

  const prime::reflect::Class *cls =
      prime::reflect::GetReflectedClass(classHash);
  oPath.append(cls->extension);
  prime::common::RegisterResource(oPath);

  std::string clsBuffer(cls->classSize, 0);
  cls->construct(clsBuffer.data());
  namespace pu = prime::utils;
  namespace pr = prime::reflect;

  pu::PlayGround pg;
  pu::ResourceDebugPlayground dbg;
  pu::PlayGround::Pointer<prime::common::ResourceBase> baseClass =
      pg.NewBytes<prime::common::ResourceBase>(clsBuffer.data(),
                                               cls->classSize);

  sq_push(v, 3);
  SetClass(cls, baseClass, v, pg, dbg);
  sq_poptop(v);

  std::string built = dbg.Build(pg, cls);
  std::ofstream ostr(cacheFolder + oPath, std::ios::binary | std::ios::out);

  if (ostr.fail()) {
    oPath = cacheFolder;

    AFileInfo finf(path);
    auto exploded = finf.Explode();
    exploded.pop_back();

    for (auto &e : exploded) {
      oPath.append(e);
      oPath.push_back('/');
      es::mkdir(oPath);
    }

    oPath.append(finf.GetFilename());
    oPath.push_back('.');
    oPath.append(cls->extension);
    ostr.open(oPath, std::ios::binary | std::ios::out);
  }
  ostr.write(built.data(), built.size());

  return 0;
}

uint32 GetCrc(std::istream &str) {
  str.seekg(0, std::ios::end);
  const size_t size = str.tellg();
  str.seekg(0);
  char buffer[0x10000];
  const size_t numBlocks = size / sizeof(buffer);
  const size_t restBlock = size % sizeof(buffer);
  uint32 crc = 0;

  for (size_t i = 0; i < numBlocks; i++) {
    str.read(buffer, sizeof(buffer));
    crc = crc32b(crc, buffer, sizeof(buffer));
  }

  if (restBlock) {
    str.read(buffer, restBlock);
    crc = crc32b(crc, buffer, restBlock);
  }

  str.seekg(0);

  return crc;
}

static SQInteger dofile(HSQUIRRELVM v) {
  const SQChar *path;
  sq_getstring(v, 2, &path);
  std::string sPath(autogenFolder);
  sPath.append(path);
  sPath.append(".nut");
  std::string oPath = autogenCacheFolder;
  oPath.append(path);
  oPath.push_back('.');
  const auto oExt(prime::common::GetClassExtension<prime::script::FuncProto>());
  oPath.append(oExt);

  std::ifstream str(sPath);

  if (str.fail()) {
    sq_throwerror(v, "Couldn't open file");
    return 0;
  }

  const uint32 inputCrc = GetCrc(str);

  {
    std::ifstream wr(oPath, std::ios::binary | std::ios::in);
    if (wr.good()) {
      wr.seekg(0, std::ios::end);
      const size_t size = wr.tellg();
      wr.seekg(0);
      std::string buffer;
      buffer.resize(size);
      wr.read(buffer.data(), buffer.size());
      prime::utils::ResourceDebug *dbg =
          prime::utils::LocateDebug(buffer.data(), buffer.size());

      if (dbg && dbg->inputCrc == inputCrc) {
        SQFunctionProto *proto = SQFunctionProto::Create(_ss(v));
        proto->playground.NewBytes<prime::script::FuncProto>(buffer.data(),
                                                             buffer.size());
        SQClosure *closure = SQClosure::Create(
            _ss(v), proto, _table(v->_roottable)->GetWeakRef(OT_TABLE));
        v->Push(closure);

        sq_pushroottable(v);
        if (SQ_FAILED(sq_call(v, 1, SQFalse, SQTrue))) {
          int t = 0;
        }

        sq_poptop(v);

        return 0;
      }
    }
  }

  if (SQ_FAILED(sq_compile(
          v,
          [](void *up) -> SQInteger {
            char data;
            auto str = static_cast<std::ifstream *>(up);
            if (str->read(&data, 1); str->eof()) {
              return 0;
            }
            return data;
          },
          &str, path, SQTrue))) {
    return 0;
  }

  auto wrf = [](SQUserPointer up, SQUserPointer data,
                SQInteger size) -> SQInteger {
    static_cast<std::ofstream *>(up)->write(static_cast<char *>(data), size);
    return 0;
  };

  std::ofstream wr(oPath, std::ios::binary | std::ios::out);
  sq_writeclosure(v, wrf, &wr, inputCrc);

  sq_pushroottable(v);
  if (SQ_FAILED(sq_call(v, 1, SQFalse, SQTrue))) {
    int t = 0;
  }

  sq_poptop(v);

  return 0;
}

namespace prime::script {
void CompileScript(const common::ResourcePath &path) {
  autogenFolder = prime::common::ProjectDataFolder() + "autogenerated/";
  cacheFolder = prime::common::ProjectDataFolder() + "cache/";
  es::mkdir(cacheFolder);
  autogenCacheFolder = cacheFolder + "autogenerated/";
  es::mkdir(autogenCacheFolder);

  HSQUIRRELVM v = sq_open(1024);
  sq_setprintfunc(v, printfunc, printfunc);

  sq_pushroottable(v);

  sq_pushstring(v, "dofile", 6);
  sq_newclosure(v, dofile, 0);
  sq_setparamscheck(v, 2, "ts");
  sq_setnativeclosurename(v, -1, "dofile");
  sq_newslot(v, -3, SQTrue);

  sq_pushstring(v, "AddResource", 11);
  sq_newclosure(v, AddResource, 0);
  sq_setparamscheck(v, 4, "tsxi");
  sq_setnativeclosurename(v, -1, "AddResource");
  sq_newslot(v, -3, SQTrue);

  sq_pushstring(v, "graphics", 8);
  sq_newtable(v);
  sq_newslot(v, -3, SQFalse);

  sq_pushstring(v, "common", 6);
  sq_newtable(v);
  sq_newslot(v, -3, SQFalse);

  RegisterResourceClasses(v);

  sq_setcompilererrorhandler(v, CompilerError);
  sq_newclosure(v, RuntimeError, 0);
  sq_seterrorhandler(v);

  std::ifstream str(path.AbsPath());

  if (SQ_SUCCEEDED(sq_compile(
          v,
          [](void *up) -> SQInteger {
            char data;
            auto str = static_cast<std::ifstream *>(up);
            if (str->read(&data, 1); str->eof()) {
              return 0;
            }
            return data;
          },
          &str, path.localPath.c_str(), SQTrue))) {
    sq_pushroottable(v);
    if (SQ_FAILED(sq_call(v, 1, SQFalse, SQTrue))) {
      int t = 0;
    }
  }

  sq_close(v);
}
} // namespace prime::script
