#include "script/detail/funcproto.hpp"
#include "common/resource.hpp"
#include "sqbase/sqclosure.h"
#include "sqbase/sqfuncproto.h"
#include "sqbase/sqfuncstate.h"
#include "sqbase/sqtable.h"
#include "sqbase/sqvm.h"
#include "utils/playground.hpp"
#include <cstdarg>
#include <map>

using namespace prime::script;

const SQChar *FuncProto::GetLocal(SQVM *vm, SQUnsignedInteger stackbase,
                                  SQUnsignedInteger nseq,
                                  SQUnsignedInteger nop) {
  for (auto &lvar : localVars) {
    if (lvar.startOp <= nop && lvar.endOp >= nop) {
      if (nseq == 0) {
        vm->Push(vm->_stack[stackbase + lvar.pos]);
        return lvar.name.raw();
      }
      nseq--;
    }
  }

  return nullptr;
}

SQInteger FuncProto::GetLine(SQInstruction *curr) {
  SQInteger op = (SQInteger)(curr - instructions.begin());
  SQInteger line = lineInfos[0].line;
  SQInteger low = 0;
  SQInteger high = lineInfos.numItems - 1;
  SQInteger mid = 0;
  while (low <= high) {
    mid = low + ((high - low) >> 1);
    SQInteger curop = lineInfos[mid].op;
    if (curop > op) {
      high = mid - 1;
    } else if (curop < op) {
      if (mid < (lineInfos.numItems - 1) && lineInfos[mid + 1].op >= op) {
        break;
      }
      low = mid + 1;
    } else { // equal
      break;
    }
  }

  while (mid > 0 && lineInfos[mid].op >= op)
    mid--;

  line = lineInfos[mid].line;

  return line;
}

// force debugger to register this class
void LiteralString::Release() {}

void prime::script::BuildFuncProto(SQFuncState *fs) {
  namespace pu = prime::utils;
  pu::PlayGround::Pointer<FuncProto> &proto = fs->curProto;
  pu::PlayGround &pg = *proto.owner;
  proto->isGenerator = fs->_bgenerator;
  proto->stackSize = fs->_stacksize;
  proto->varParams = fs->_varparams;
  SQString *s = _string(fs->_sourcename);
  pg.NewCString(proto->sourceName, {s->Val(), size_t(s->Size())});
  s = _string(fs->_name);
  pg.NewCString(proto->name, {s->Val(), size_t(s->Size())});

  for (uint32 i = 0; i < fs->_instructions.size(); i++) {
    pg.ArrayEmplace(proto->instructions, fs->_instructions[i]);
  }

  for (uint32 i = 0; i < fs->_outervalues.size(); i++) {
    SQOuterVar &var = fs->_outervalues[i];
    pu::PlayGround::Pointer<OuterVar> oVar = pg.ArrayEmplace(proto->outerVars);
    oVar->type = OuterType(var._type);
    oVar->src = int32(_integer(var._src));
    SQString *s = _string(var._name);
    pg.NewCString(oVar->name, {s->Val(), size_t(s->Size())});
  }

  for (uint32 i = 0; i < fs->_localvarinfos.size(); i++) {
    SQLocalVarInfo &var = fs->_localvarinfos[i];
    pu::PlayGround::Pointer<LocalVar> oVar = pg.ArrayEmplace(proto->localVars);
    oVar->startOp = var._start_op;
    oVar->endOp = var._end_op;
    oVar->pos = var._pos;
    SQString *s = _string(var._name);
    pg.NewCString(oVar->name, {s->Val(), size_t(s->Size())});
  }

  for (uint32 i = 0; i < fs->_lineinfos.size(); i++) {
    SQLineInfo &var = fs->_lineinfos[i];
    pg.ArrayEmplace(proto->lineInfos, LineInfo{
                                          .op = uint32(var._op),
                                          .line = uint32(var._line),
                                      });
  }

  for (uint32 i = 0; i < fs->_defaultparams.size(); i++) {
    pg.ArrayEmplace(proto->defaultParams, fs->_defaultparams[i]);
  }

  SQObjectPtr refidx;
  SQObjectPtr key;
  SQObjectPtr val;
  SQInteger idx;
  std::map<SQInteger, SQObjectPtr> items;

  while ((idx = _table(fs->_literals)->Next(false, refidx, key, val)) != -1) {
    items[_integer(val)] = key;
    refidx = idx;
  }

  auto NewString = [&pg](SQObjectPtr &v) {
    char buffer[0x1000]{};
    LiteralString *str = reinterpret_cast<LiteralString *>(buffer);
    str->_uiRef = 1;
    str->_hash = _string(v)->_hash;
    str->GetInfo()._len = _string(v)->Size();

    size_t strSize = sizeof(LiteralString);

    if (_string(v)->GetInfo()._valOffset > 0) {
      str->GetInfo()._valOffset = sizeof(SQHash);
      memcpy(str->_val + sizeof(SQHash), _string(v)->Val(), _string(v)->Size());
      strSize += _string(v)->Size() + 1;
    }

    return pg.NewBytes<LiteralString>(buffer, strSize);
  };

  for (auto [_, v] : items) {
    if (v._type == OT_STRING) {
      pu::PlayGround::Pointer<Literal> lit = pg.ArrayEmplace(proto->literals);
      lit->type = v._type;
      pu::PlayGround::Pointer<LiteralString> ls = NewString(v);

      pg.Link(lit->pString, ls.operator->());
    } else {
      Literal lt;
      lt.type = v._type;
      lt.raw = v._unVal.raw;
      pg.ArrayEmplace(proto->literals, lt);
    }
  }

  for (uint32 i = 0; i < fs->_parameters.size(); i++) {
    pu::PlayGround::Pointer<Literal> lit = pg.ArrayEmplace(proto->parameters);
    lit->type = OT_STRING;
    pu::PlayGround::Pointer<LiteralString> ls = NewString(fs->_parameters[i]);

    pg.Link(lit->pString, ls.operator->());
  }
}

void printfunc(HSQUIRRELVM SQ_UNUSED_ARG(v), const SQChar *s, ...) {
  va_list vl;
  va_start(vl, s);
  vprintf(s, vl);
  va_end(vl);
}

void CompilerError(HSQUIRRELVM v, const SQChar *sErr, const SQChar *sSource,
                   SQInteger line, SQInteger column) {
  SQPRINTFUNCTION pf = sq_geterrorfunc(v);
  pf(v, _SC("%s:%d:%d: error: %s"), sSource, line, column, sErr);
}

SQInteger RuntimeError(HSQUIRRELVM v) {
  SQPRINTFUNCTION pf = sq_geterrorfunc(v);
  if (pf) {
    const SQChar *sErr = 0;
    if (sq_gettop(v) >= 1) {
      if (SQ_SUCCEEDED(sq_getstring(v, 2, &sErr))) {
        pf(v, _SC("\nAN ERROR HAS OCCURRED [%s]\n"), sErr);
      } else {
        pf(v, _SC("\nAN ERROR HAS OCCURRED [unknown]\n"));
      }
      // sqstd_printcallstack(v);
    }
  }
  return 0;
}

HSQUIRRELVM GLOBAL_VM = [] {
  HSQUIRRELVM v = sq_open(1024);
  sq_setprintfunc(v, printfunc, printfunc);

  sq_pushroottable(v);

  /*
  sqstd_register_systemlib(v);
  sqstd_register_mathlib(v);
  sqstd_register_stringlib(v);

  sqigui_register_ImVec2(v);
  sqigui_register_ImDrawList(v);*/

  sq_setcompilererrorhandler(v, CompilerError);
  sq_newclosure(v, RuntimeError, 0);
  sq_seterrorhandler(v);
  sq_poptop(v);

  return v;
}();

template <> class prime::common::InvokeGuard<FuncProto> {
  static inline const bool data = prime::common::AddResourceHandle<FuncProto>({
      .Process =
          [](ResourceData &data) {
            data.As<script::FuncProto>(); // validate
            SQFunctionProto *proto = SQFunctionProto::Create(_ss(GLOBAL_VM));
            proto->playground.NewBytes<script::FuncProto>(data.buffer.data(),
                                                          data.buffer.size());
            SQClosure *closure = SQClosure::Create(
                _ss(GLOBAL_VM), proto,
                _table(GLOBAL_VM->_roottable)->GetWeakRef(OT_TABLE));
            GLOBAL_VM->Push(closure);
            sq_pushroottable(GLOBAL_VM);
            sq_call(GLOBAL_VM, 1, SQFalse, SQTrue);
            sq_poptop(GLOBAL_VM);
          },
      .Delete = nullptr,
  });
};

REGISTER_CLASS(prime::script::FuncProto);
