#pragma once
#include "common/array.hpp"
#include "common/local_pointer.hpp"

#include "sqconfig.h"
#include "sqobject.h"

struct SQVM;
struct SQInstruction;
struct SQFuncState;

namespace prime::script {
struct OuterVar;
struct LocalVar;
struct LineInfo;
struct FuncProto;
struct LiteralString;
struct Literal;
} // namespace prime::script

HASH_CLASS(prime::script::OuterVar);
HASH_CLASS(prime::script::LocalVar);
HASH_CLASS(prime::script::LineInfo);
HASH_CLASS(prime::script::LiteralString);
HASH_CLASS(prime::script::Literal);
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

struct LiteralString : SQRefCounted {
  static const SQInteger MIN_SIZE = sizeof(SQHash);
  void Release() override;
  SQSharedState *_sharedstate = nullptr;
  SQString *_next = nullptr; // for SharedStates string table
  SQInteger _len;
  SQHash _hash;
  SQChar _val[MIN_SIZE];
};

struct Literal {
  SQObjectType type;
  union {
    SQRawObjectVal raw;
    SQInteger nInteger;
    SQFloat fFloat;
    common::LocalPointer<LiteralString> pString{};
  };

  operator SQObjectPtr() {
    if (type == OT_STRING) {
      return reinterpret_cast<SQString *>(pString.Get());
    }
    return reinterpret_cast<SQObjectPtr &>(*this);
  }
};

struct FuncProto : common::Resource<FuncProto> {
  common::LocalArray32<char> sourceName;
  common::LocalArray32<char> name;
  bool isGenerator = false;
  uint8 charSize = uint8(sizeof(SQChar));
  uint8 intSize = uint8(sizeof(SQInteger));
  uint8 floatSize = uint8(sizeof(SQFloat));
  uint32 stackSize = 0;
  uint32 varParams;
  common::LocalArray32<LocalVar> localVars;
  common::LocalArray32<LineInfo> lineInfos;
  common::LocalArray32<OuterVar> outerVars;
  common::LocalArray32<SQInteger> defaultParams;
  common::LocalArray32<SQInstruction> instructions;
  common::LocalArray32<Literal> literals;
  common::LocalArray32<Literal> parameters;
  common::LocalArray32<FuncProto> functions;

  const SQChar *GetLocal(SQVM *vm, SQUnsignedInteger stackbase,
                         SQUnsignedInteger nseq, SQUnsignedInteger nop);
  SQInteger GetLine(SQInstruction *curr);
};

void BuildFuncProto(SQFuncState *fs);

} // namespace prime::script
