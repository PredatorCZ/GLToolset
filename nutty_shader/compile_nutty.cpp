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

std::string CompileScript(AppContext *ctx) {
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

  sq_pushregistrytable(v);
  sq_pushstring(v, "CTX", 3);
  sq_pushuserpointer(v, ctx);
  sq_newslot(v, -3, SQFalse);
  sq_poptop(v);

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
          &ctx->GetStream(), ctx->workingFile.GetFullPath().data(), SQTrue))) {

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
};

struct StackValue {
  StackType type;
  union {
    bool asBool;
    int64 asInt;
    float asFloat;
    SQChar *asCString;
    prime::script::FuncProto *asClosure;
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

  StackValue &operator=(prime::script::FuncProto *in) {
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
};

void EntryPoint(SQClosure *c, HSQUIRRELVM v) {
  prime::script::FuncProto *proto = c->_function;

  if (proto->parameters.numItems > 1) {
    throw std::runtime_error("Entrypoint function cannot have parameters");
  }

  std::vector<StackType> stack(proto->stackSize);
}

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
  uint32 count = 0;
};

void AppProcessFile(AppContext *ctx) {
  std::string protoBuf = CompileScript(ctx);
  // todo check
  prime::script::FuncProto *proto =
      reinterpret_cast<prime::script::FuncProto *>(protoBuf.data());

  std::vector<StackValue> stack(proto->stackSize);

  std::map<std::string, StackValue> globals;

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
      stack.at(i._arg0) = &proto->functions[i._arg1];
      break;
    }

    case _OP_CALL: {
      //std::string_view funcName = stack.at(i._arg1).asCString;
      //uint32 stackBase = i._arg2;
      //uint32 numArgs = i._arg3;
      break;
    }

    case _OP_NEWSLOT: {
      StackValue &key = stack.at(i._arg2);
      StackValue &value = stack.at(i._arg3);

      globals[key.asCString] = value;
      break;
    }
    }
  }

  int t = 0;
}
