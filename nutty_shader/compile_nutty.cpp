/*  CompileNutty
    Copyright(C) 2025 Lukas Cone

    This program is free software : you can redistribute it and / or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.If not, see <https://www.gnu.org/licenses/>.
*/

#include "common/resource.hpp"
#include "project.h"
#include "script/detail/funcproto.hpp"
#include "spike/app_context.hpp"
#include "spike/reflect/reflector.hpp"
#include "sqclosure.h"
#include "sqvm.h"
#include "utils/converters.hpp"
#include <cstdarg>
#include <istream>
#include <map>
#include <sstream>

static std::string_view filters[]{".nut$"};

static AppInfo_s appInfo{
    .filteredLoad = true,
    .header = CompileNutty_DESC " v" CompileNutty_VERSION
                                ", " CompileNutty_COPYRIGHT "Lukas Cone",
    .filters = filters,
};

AppInfo_s *AppInitModule() { return &appInfo; }

uint32 prime::common::detail::RegisterClass(prime::common::ExtString,
                                            uint32 obj) {
  return obj;
}

bool prime::common::AddResourceHandle(uint32, prime::common::ResourceHandle) {
  return true;
}

static void printfn(HSQUIRRELVM SQ_UNUSED_ARG(v), const SQChar *s, ...) {
  va_list vl;
  va_start(vl, s);
  vprintf(s, vl);
  va_end(vl);
}

static void printfnerr(HSQUIRRELVM SQ_UNUSED_ARG(v), const SQChar *s, ...) {
  va_list vl;
  va_start(vl, s);
  vprintf(s, vl);
  va_end(vl);
}

void CompilerError(HSQUIRRELVM v, const SQChar *sErr, const SQChar *sSource,
                   SQInteger line, SQInteger column);

SQInteger RuntimeError(HSQUIRRELVM v);

static SQInteger dofile(HSQUIRRELVM v) {
  sq_pushregistrytable(v);
  sq_pushstring(v, "CTX", 3);
  sq_get(v, -2);
  SQUserPointer up;
  sq_getuserpointer(v, -1, &up);
  sq_pop(v, 2);
  AppContext *ctx = static_cast<AppContext *>(up);

  const SQChar *spath;
  sq_getstring(v, 2, &spath);
  std::string path(spath);
  path.append(".nut");
  AppContextStream str = ctx->RequestFile(path);

  if (SQ_FAILED(sq_compile(
          v,
          [](void *up) -> SQInteger {
            char data;
            auto str = static_cast<std::istream *>(up);
            if (str->read(&data, 1); str->eof()) {
              return 0;
            }
            return data;
          },
          str.Get(), path.c_str(), SQTrue))) {
    return 0;
  }
  sq_pushroottable(v);
  if (SQ_FAILED(sq_call(v, 1, SQFalse, SQTrue))) {
    throw std::runtime_error("Failed to execute main");
  }

  sq_poptop(v);

  return 0;
}

static SQChar SQVEC3_TAG[] = "vec3";
static SQChar SQVEC4_TAG[] = "vec4";
static SQChar SQFLOAT_TAG[] = "float";
static SQChar SQINT_TAG[] = "int";
static SQChar SQUINT_TAG[] = "uint";
static SQChar SQMAT4_TAG[] = "mat4";

namespace vec3 {
SQInteger constructor(HSQUIRRELVM v) {
  SQUserPointer self = nullptr;
  if (SQ_FAILED(sq_getinstanceup(v, 1, &self, SQVEC3_TAG, SQFalse))) {
    return sq_throwerror(v, _SC("invalid type tag"));
  }
  if (self != nullptr) {
    return sq_throwerror(v, _SC("invalid vec3 object"));
  }

  return SQ_OK;
}

void Register(HSQUIRRELVM v) {
  /* +1 */ sq_pushstring(v, _SC(SQVEC3_TAG), -1);
  /* +1 */ sq_newclass(v, SQFalse);
  /* +0 */ sq_settypetag(v, -1, SQVEC3_TAG);

  /* +1 */ // sq_pushstring(v, "constructor", 11);
  /* +1 */ // sq_newclosure(v, constructor, 0);
  /* +0 */ // sq_setparamscheck(v, 0, "xnnn");
  /* +0 */ // sq_setnativeclosurename(v, -1, "constructor");
  /* -2 */ // sq_newslot(v, -3, SQFalse);

  /* -2 */ sq_newslot(v, -3, SQFalse);
}
} // namespace vec3

std::string CompileScript(std::string path, std::istream &str) {
  HSQUIRRELVM v = sq_open(1024);
  sq_setprintfunc(v, printfn, printfnerr);

  sq_pushroottable(v);

  sq_pushstring(v, _SC(SQVEC3_TAG), -1);
  sq_newclass(v, SQFalse);
  sq_settypetag(v, -1, SQVEC3_TAG);
  sq_newslot(v, -3, SQFalse);

  sq_pushstring(v, _SC(SQVEC4_TAG), -1);
  sq_newclass(v, SQFalse);
  sq_settypetag(v, -1, SQVEC4_TAG);
  sq_newslot(v, -3, SQFalse);

  sq_pushstring(v, _SC(SQFLOAT_TAG), -1);
  sq_newclass(v, SQFalse);
  sq_settypetag(v, -1, SQFLOAT_TAG);
  sq_newslot(v, -3, SQFalse);

  sq_pushstring(v, _SC(SQINT_TAG), -1);
  sq_newclass(v, SQFalse);
  sq_settypetag(v, -1, SQINT_TAG);
  sq_newslot(v, -3, SQFalse);

  sq_pushstring(v, _SC(SQUINT_TAG), -1);
  sq_newclass(v, SQFalse);
  sq_settypetag(v, -1, SQUINT_TAG);
  sq_newslot(v, -3, SQFalse);

  sq_pushstring(v, _SC(SQMAT4_TAG), -1);
  sq_newclass(v, SQFalse);
  sq_settypetag(v, -1, SQMAT4_TAG);
  sq_newslot(v, -3, SQFalse);

  sq_pushstring(v, "dofile", 6);
  sq_newclosure(v, dofile, 0);
  sq_setparamscheck(v, 2, "ts");
  sq_setnativeclosurename(v, -1, "dofile");
  sq_newslot(v, -3, SQTrue);

  sq_setcompilererrorhandler(v, CompilerError);
  sq_newclosure(v, RuntimeError, 0);
  sq_seterrorhandler(v);

  /*sq_pushregistrytable(v);
  sq_pushstring(v, "CTX", 3);
  sq_pushuserpointer(v, ctx);
  sq_newslot(v, -3, SQFalse);
  sq_poptop(v);*/

  if (SQ_SUCCEEDED(sq_compile(
          v,
          [](void *up) -> SQInteger {
            char data;
            auto str = static_cast<std::istream *>(up);
            if (str->read(&data, 1); str->eof()) {
              return 0;
            }
            return data;
          },
          &str, path.c_str(), SQTrue))) {

    // sq_pushroottable(v);
    // if (SQ_FAILED(sq_call(v, 1, SQFalse, SQTrue))) {
    //   int t = 0;
    // }

    auto wrf = [](SQUserPointer up, SQUserPointer data,
                  SQInteger size) -> SQInteger {
      static_cast<std::stringstream *>(up)->write(static_cast<char *>(data),
                                                  size);
      return 0;
    };

    std::stringstream wr;
    sq_writeclosure(v, wrf, &wr, 0);
    sq_close(v);

    return std::move(wr).str();
  }

  sq_close(v);
  return {};
}

enum StackType {
  None,
  Bool,
  Int,
  UInt,
  Float,
  CString,
  Closure,
  Class,
  Typename,
  Array,
};

struct ClassDef;

enum class GLType {
  None,
  vec2,
  vec3,
  vec4,
  mat2,
  mat3,
  mat4,
  Float,
  Int,
  UInt,
};

struct GlobalType {
  GLType type;
  uint32 count;
};

struct FuncArg {
  std::string_view name;
  GlobalType type;
};

struct FuncDef {
  prime::script::FuncProto *proto;
  std::vector<FuncArg> args;
};

struct StackValue {
  StackType type;
  union {
    bool asBool;
    int64 asInt;
    float asFloat;
    SQChar *asCString;
    FuncDef *asClosure;
    ClassDef *asClass;
    GlobalType asType;
    std::vector<StackValue> *asArray;
  };

  StackValue &operator=(SQChar *in) {
    asCString = in;
    type = StackType::CString;

    return *this;
  }

  StackValue &operator=(int64 in) {
    asInt = in;
    type = StackType::Int;

    return *this;
  }

  StackValue &operator=(float in) {
    asFloat = in;
    type = StackType::Float;

    return *this;
  }

  StackValue &operator=(bool in) {
    asBool = in;
    type = StackType::Bool;

    return *this;
  }

  StackValue &operator=(FuncDef *in) {
    asClosure = in;
    type = StackType::Closure;

    return *this;
  }

  StackValue &operator=(const StackValue &) = default;

  StackValue &operator=(prime::script::Literal &lit) {
    switch (lit.type) {
    case OT_STRING:
      return *this = reinterpret_cast<SQString *>(lit.pString.Get())->Val();
    case OT_BOOL:
      return *this = bool(lit.nInteger);
    case OT_INTEGER:
      return *this = int64(lit.nInteger);
    case OT_FLOAT:
      return *this = reinterpret_cast<float &>(lit.fFloat);

    default:
      return *this;
    }
  }

  StackValue &operator=(ClassDef *clsDef) {
    asClass = clsDef;
    type = StackType::Class;

    return *this;
  }

  StackValue &operator=(GlobalType glob) {
    asType = glob;
    type = StackType::Typename;

    return *this;
  }

  StackValue &operator=(std::vector<StackValue> *arr) {
    asArray = arr;
    type = StackType::Array;

    return *this;
  }
};

std::map<std::string_view, GLType> STR_TO_GLTYPE{{
    {"vec2", GLType::vec2},
    {"vec3", GLType::vec3},
    {"vec4", GLType::vec4},
    {"mat2", GLType::mat2},
    {"mat3", GLType::mat3},
    {"mat4", GLType::mat4},
    {"float", GLType::Float},
    {"int", GLType::Int},
    {"uint", GLType::UInt},
}};

GlobalType FromStack(StackValue &v) {
  GlobalType retType{};

  if (v.type == StackType::CString) {
    retType.type = STR_TO_GLTYPE.at(v.asCString);
  } else if (v.type == StackType::Typename) {
    return v.asType;
  }

  return retType;
}

struct ClassDef {
  std::string_view name;
  std::map<std::string_view, GlobalType> members;
};

void EntryPoint(SQClosure *c, HSQUIRRELVM v) {
  prime::script::FuncProto *proto = c->_function;

  if (proto->parameters.numItems > 1) {
    throw std::runtime_error("Entrypoint function cannot have parameters");
  }

  std::vector<StackType> stack(proto->stackSize);
}

struct GlobalState {
  std::map<std::string, StackValue> globals;
  std::vector<std::string> protoBufs;
};

void dofile(AppContext *ctx, std::string_view path, GlobalState &globals);

template <class fn>
StackValue execOperand(StackValue &o0, StackValue &o1, fn &&cb) {
  bool forceFloat = o0.type == StackType::Float || o1.type == StackType::Float;
  StackValue retVal;

  if (forceFloat) {
    float f0 = o0.type == StackType::Float ? o0.asFloat : o0.asInt;
    float f1 = o1.type == StackType::Float ? o1.asFloat : o1.asInt;

    retVal = cb(f0, f1);
  } else {
    retVal = cb(o0.asInt, o1.asInt);
  }

  return retVal;
}

template <class fn> bool compOperand(StackValue &o0, StackValue &o1, fn &&cb) {
  switch (o0.type) {
  case StackType::Bool: {
    switch (o1.type) {
    case StackType::Bool:
      return cb(o0.asBool, o1.asBool);
    case StackType::Float:
      return cb(o0.asBool, o1.asFloat);
    case StackType::Int:
    case StackType::UInt:
      return cb(o0.asBool, o1.asInt);
    default:
      return false;
    }
    break;
  }

  case StackType::Int:
  case StackType::UInt: {
    switch (o1.type) {
    case StackType::Bool:
      return cb(o0.asInt, o1.asBool);
    case StackType::Float:
      return cb(o0.asInt, o1.asFloat);
    case StackType::Int:
    case StackType::UInt:
      return cb(o0.asInt, o1.asInt);
    default:
      return false;
    }
    break;
  }

  case StackType::Float: {
    switch (o1.type) {
    case StackType::Bool:
      return cb(o0.asFloat, o1.asBool);
    case StackType::Float:
      return cb(o0.asFloat, o1.asFloat);
    case StackType::Int:
    case StackType::UInt:
      return cb(o0.asFloat, o1.asInt);
    default:
      return false;
    }
    break;
  }

  case StackType::Array:
    if (o1.type == StackType::Array) {
      return cb(o0.asArray, o1.asArray);
    }

    return false;

  case StackType::Class:
    if (o1.type == StackType::Class) {
      return cb(o0.asClass, o1.asClass);
    }

    return false;

  case StackType::Closure:
    if (o1.type == StackType::Closure) {
      return cb(o0.asClosure, o1.asClosure);
    }

    return false;

    /*case StackType::Typename:
      if (o1.type == StackType::Typename) {
        return cb(o0.asType, o1.asType);
      }

      return false;*/

  case StackType::CString:
    if (o1.type == StackType::CString) {
      return cb(std::string_view(o0.asCString), std::string_view(o1.asCString));
    }

    return false;

  default:
    break;
  }

  return false;
}

void execute(AppContext *ctx, std::string &protoBuf, GlobalState &globals) {
  prime::script::FuncProto *proto =
      reinterpret_cast<prime::script::FuncProto *>(protoBuf.data());

  std::vector<StackValue> stack(proto->stackSize);

  for (SQInstruction i : proto->instructions) {
    switch (i.op) {
    case _OP_PREPCALLK: {
      stack.at(i._arg0) = proto->literals[i._arg1];
      break;
    }

    case _OP_LOAD: {
      stack.at(i._arg0) = proto->literals[i._arg1];
      break;
    }

    case _OP_DLOAD: {
      stack.at(i._arg0) = proto->literals[i._arg1];
      stack.at(i._arg2) = proto->literals[i._arg3];
      break;
    }

    case _OP_LOADINT: {
      stack.at(i._arg0) = int64(i._arg1);
      break;
    }

    case _OP_LOADFLOAT: {
      stack.at(i._arg0) = reinterpret_cast<float &>(i._arg1);
      break;
    }

    case _OP_LOADBOOL: {
      stack.at(i._arg0) = bool(i._arg1);
      break;
    }

    case _OP_CLOSURE: {
      prime::script::FuncProto &funcProto = proto->functions[i._arg1];
      FuncDef *funcDef = new FuncDef();
      funcDef->proto = &funcProto;

      for (uint32 paramId = 0; auto &p : funcProto.defaultParams) {
        StackValue tmp;
        tmp = funcProto.parameters[paramId++];
        FuncArg arg{
            .name = tmp.asCString,
            .type = FromStack(stack.at(p)),
        };

        funcDef->args.emplace_back(arg);
      }

      stack.at(i._arg0) = funcDef;
      break;
    }

    case _OP_CALL: {
      std::string_view funcName = stack.at(i._arg1).asCString;
      uint32 stackBase = i._arg2;
      uint32 numArgs = i._arg3;

      if (funcName == "dofile") {
        if (numArgs != 2) {
          printf("Incorrect number of arguments for dofile function.\n");
        } else {
          if (stack.at(stackBase + 1).type != StackType::CString) {
            printf("dofile argument must be string.\n");
          } else {
            dofile(ctx, stack.at(stackBase + 1).asCString, globals);
          }
        }
      } else if (funcName == "array") {
        GlobalType type = FromStack(stack.at(stackBase + 2));
        type.count = stack.at(stackBase + 1).asInt;
        stack.at(i._arg0) = type;
      } else {
        printf("Function %s is not supported.\n", funcName.data());
      }

      break;
    }

    case _OP_NEWSLOT: {
      StackValue &key = stack.at(i._arg2);
      StackValue &value = stack.at(i._arg3);

      globals.globals[key.asCString] = value;
      break;
    }

    case _OP_NEWOBJ: {
      switch (i._arg3) {
      case NOT_CLASS: {
        stack.at(i._arg0) = new ClassDef();
        stack.at(i._arg0).asClass->name = stack.at(i.asClass._arg5).asCString;

        if (i.asClass._arg4 != 0xff) {
          printf("Classes cannot have base classes, for now....\n");
        }
        if (i._arg3 != 0xff) {
          printf("Classes cannot have attributes, for now....\n");
        }
        break;
      }

      case NOT_ARRAY:
        stack.at(i._arg0) = new std::vector<StackValue>();
        break;

      default:
        break;
      }

      break;
    }

    case _OP_GET: {
      if (i._arg1 == 0) {
        StackValue &key = stack.at(i._arg2);
        if (key.type == StackType::CString) {
          stack.at(i._arg0) = globals.globals.at(key.asCString);
          // stack.at(i._arg0) = key.asCString;
        }
      }
      break;
    }

    case _OP_GETK: {
      if (i._arg2 == 0) {
        stack.at(i._arg0) = proto->literals[i._arg1];
      }

      break;
    }

    case _OP_NEWSLOTA: {
      // flags: i._arg0
      stack.at(i._arg1).asClass->members.emplace(stack.at(i._arg2).asCString,
                                                 FromStack(stack.at(i._arg3)));

      break;
    }

    case _OP_APPENDARRAY: {
      AppendArrayType aat = AppendArrayType(i._arg2);
      StackValue type;

      switch (aat) {
      case AppendArrayType::AAT_BOOL:
        type = bool(i._arg1);
        break;

      case AppendArrayType::AAT_INT:
        type = int64(i._arg1);
        break;

      case AppendArrayType::AAT_FLOAT:
        type = reinterpret_cast<float &>(i._arg1);
        break;

      case AppendArrayType::AAT_LITERAL:
        type = proto->literals[i._arg1];
        break;

      case AppendArrayType::AAT_STACK:
        type = stack.at(i._arg1);
        break;

      default:
        break;
      }

      stack.at(i._arg0).asArray->emplace_back(type);

      break;
    }

    case _OP_DIV:
      stack.at(i._arg0) = execOperand(stack.at(i._arg2), stack.at(i._arg1),
                                      [](auto o0, auto o1) { return o0 / o1; });
      break;

    case _OP_MUL:
      stack.at(i._arg0) = execOperand(stack.at(i._arg2), stack.at(i._arg1),
                                      [](auto o0, auto o1) { return o0 * o1; });
      break;

    case _OP_ADD:
      stack.at(i._arg0) = execOperand(stack.at(i._arg2), stack.at(i._arg1),
                                      [](auto o0, auto o1) { return o0 + o1; });
      break;

    case _OP_SUB:
      stack.at(i._arg0) = execOperand(stack.at(i._arg2), stack.at(i._arg1),
                                      [](auto o0, auto o1) { return o0 - o1; });
      break;

    case _OP_MOD:
      stack.at(i._arg0) = stack.at(i._arg2).asInt % stack.at(i._arg1).asInt;
      break;

    case _OP_NE:
      stack.at(i._arg0) =
          compOperand(stack.at(i._arg2), stack.at(i._arg1),
                      [](auto o0, auto o1) { return o0 != o1; });
      break;

    case _OP_EQ:
      stack.at(i._arg0) =
          compOperand(stack.at(i._arg2), stack.at(i._arg1),
                      [](auto o0, auto o1) { return o0 == o1; });
      break;

    case _OP_CMP:
      switch (CmpOP(i._arg3)) {
      case CmpOP::CMP_G:
        stack.at(i._arg0) =
            compOperand(stack.at(i._arg2), stack.at(i._arg1),
                        [](auto o0, auto o1) { return o0 > o1; });
        break;

      case CmpOP::CMP_GE:
        stack.at(i._arg0) =
            compOperand(stack.at(i._arg2), stack.at(i._arg1),
                        [](auto o0, auto o1) { return o0 >= o1; });
        break;

      case CmpOP::CMP_L:
        stack.at(i._arg0) =
            compOperand(stack.at(i._arg2), stack.at(i._arg1),
                        [](auto o0, auto o1) { return o0 < o1; });
        break;

      case CmpOP::CMP_LE:
        stack.at(i._arg0) =
            compOperand(stack.at(i._arg2), stack.at(i._arg1),
                        [](auto o0, auto o1) { return o0 >= o1; });
        break;

      default:
        break;
      }

      break;

    case _OP_PREPCALL:
    case _OP_RETURN:
      break;

    default:
      printf("Unimplemented operation\n");
      break;
    }
  }
}

void dofile(AppContext *ctx, std::string_view path, GlobalState &globals) {
  std::string absPath(ctx->workingFile.GetFolder());
  absPath.append(path);
  absPath.append(".nut");
  auto rfile = ctx->RequestFile(absPath);
  globals.protoBufs.emplace_back(CompileScript(absPath, *rfile.Get()));
  execute(ctx, globals.protoBufs.back(), globals);
}

void AppProcessFile(AppContext *ctx) {
  GlobalState globals;
  globals.globals["vec2"] =  GlobalType{GLType::vec2, 0};
  globals.globals["vec3"] =  GlobalType{GLType::vec3, 0};
  globals.globals["vec4"] =  GlobalType{GLType::vec4, 0};
  globals.globals["mat2"] =  GlobalType{GLType::mat2, 0};
  globals.globals["mat3"] =  GlobalType{GLType::mat3, 0};
  globals.globals["mat4"] =  GlobalType{GLType::mat4, 0};
  globals.globals["float"] =  GlobalType{GLType::Float, 0};
  globals.globals["int"] =  GlobalType{GLType::Int, 0};
  globals.globals["uint"] =  GlobalType{GLType::UInt, 0};

  globals.protoBufs.emplace_back(CompileScript(
      std::string(ctx->workingFile.GetFullPath()), ctx->GetStream()));

  execute(ctx, globals.protoBufs.back(), globals);

  int t = 0;
}
