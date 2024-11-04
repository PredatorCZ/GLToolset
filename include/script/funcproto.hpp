#pragma once
#include "common/array.hpp"
#include "common/pointer.hpp"

// clang-format off
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqstring.h"
#include "sqarray.h"
#include "sqtable.h"
#include "squserdata.h"
#include "sqopcodes.h"
#include "sqclass.h"
// clang-format on

namespace prime::script {
struct OuterVar;
struct LocalVar;
struct LineInfo;
struct Object;

enum ObjectType {
  None,
  Bool,
  Int,
  Float,
  String,
};
}

HASH_CLASS(prime::script::OuterVar);
HASH_CLASS(prime::script::LocalVar);
HASH_CLASS(prime::script::LineInfo);
HASH_CLASS(prime::script::ObjectType);
HASH_CLASS(prime::script::Object);
HASH_CLASS(SQInstruction);
HASH_CLASS(SQInteger);
CLASS_RESOURCE(1, prime::script::FuncProto);

namespace prime::script {
enum class OuterType : uint8 {
  Local,
  Outer,
};

struct OuterVar {
  OuterType type;
  common::LocalArray32<char> name;
  int32 src;
};

struct LocalVar {
  common::LocalArray32<char> name;
  uint32 startOp;
  uint32 endOp;
  uint32 pos;
};

struct LineInfo {
  uint32 op : 8;
  uint32 line : 24;
};

struct Object {
  ObjectType type;
  union {
    SQFloat asFloat;
    SQInteger asInt;
    SQBool asBool;
    common::Pointer<SQChar> asString;
  };
};

struct FuncProto : common::Resource<FuncProto> {
  common::LocalArray32<char> sourceName;
  common::LocalArray32<char> name;
  bool isGenerator = false;
  uint32 stackSize = 0;
  uint32 varParams;
  common::LocalArray32<LocalVar> localVars;
  common::LocalArray32<LineInfo> lineInfos;
  common::LocalArray32<OuterVar> outerVars;
  common::LocalArray32<SQInteger> defaultParams;
  common::LocalArray32<SQInstruction> instructions;
  common::LocalArray32<Object> literals;
  common::LocalArray32<Object> parameters;
  common::LocalArray32<FuncProto> functions;

  const SQChar *GetLocal(SQVM *vm, SQUnsignedInteger stackbase,
                         SQUnsignedInteger nseq, SQUnsignedInteger nop);
  SQInteger GetLine(SQInstruction *curr);
};

} // namespace prime::script
