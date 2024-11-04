/*  see copyright notice in squirrel.h */
#ifndef _SQFUNCSTATE_H_
#define _SQFUNCSTATE_H_
///////////////////////////////////
#include "squtils.h"
#include "sqcompiler.h"
#include <memory>
#include <vector>

enum SQOuterType {
    otLOCAL = 0,
    otOUTER = 1
};

struct SQOuterVar
{

    SQOuterVar(){}
    SQOuterVar(const SQObjectPtr &name,const SQObjectPtr &src,SQOuterType t)
    {
        _name = name;
        _src=src;
        _type=t;
    }
    SQOuterVar(const SQOuterVar &ov)
    {
        _type=ov._type;
        _src=ov._src;
        _name=ov._name;
    }
    SQOuterType _type;
    SQObjectPtr _name;
    SQObjectPtr _src;
};

struct SQLocalVarInfo
{
    SQLocalVarInfo():_start_op(0),_end_op(0),_pos(0){}
    SQLocalVarInfo(const SQLocalVarInfo &lvi)
    {
        _name=lvi._name;
        _start_op=lvi._start_op;
        _end_op=lvi._end_op;
        _pos=lvi._pos;
    }
    SQObjectPtr _name;
    SQUnsignedInteger _start_op;
    SQUnsignedInteger _end_op;
    SQUnsignedInteger _pos;
};

struct SQLineInfo { SQInteger _line;SQInteger _op; };

namespace prime::utils {
    struct PlayGround;
}

typedef std::vector<std::unique_ptr<prime::utils::PlayGround>> SQFunctionVec;
typedef std::vector<SQOuterVar> SQOuterVarVec;
typedef std::vector<SQLocalVarInfo> SQLocalVarInfoVec;
typedef std::vector<SQLineInfo> SQLineInfoVec;

struct SQFuncState
{
    SQFuncState(SQSharedState *ss,SQFuncState *parent,CompilerErrorFunc efunc,void *ed);
    ~SQFuncState();
#ifdef _DEBUG_DUMP
    void Dump(SQFunctionProto *func);
#endif
    void Error(const SQChar *err);
    SQFuncState *PushChildState(SQSharedState *ss);
    void PopChildState();
    void AddInstruction(SQOpcode _op,SQInteger arg0=0,SQInteger arg1=0,SQInteger arg2=0,SQInteger arg3=0){SQInstruction i(_op,arg0,arg1,arg2,arg3);AddInstruction(i);}
    void AddInstruction(SQInstruction &i);
    void SetInstructionParams(SQInteger pos,SQInteger arg0,SQInteger arg1,SQInteger arg2=0,SQInteger arg3=0);
    void SetInstructionParam(SQInteger pos,SQInteger arg,SQInteger val);
    SQInstruction &GetInstruction(SQInteger pos){return _instructions[pos];}
    void PopInstructions(SQInteger size){for(SQInteger i=0;i<size;i++)_instructions.pop_back();}
    void SetStackSize(SQInteger n);
    SQInteger CountOuters(SQInteger stacksize);
    void SnoozeOpt(){_optimization=false;}
    void AddDefaultParam(SQInteger trg) { _defaultparams.push_back(trg); }
    SQInteger GetDefaultParamCount() { return _defaultparams.size(); }
    SQInteger GetCurrentPos(){return _instructions.size()-1;}
    SQInteger GetNumericConstant(const SQInteger cons);
    SQInteger GetNumericConstant(const SQFloat cons);
    SQInteger PushLocalVariable(const SQObject &name);
    void AddParameter(const SQObject &name);
    //void AddOuterValue(const SQObject &name);
    SQInteger GetLocalVariable(const SQObject &name);
    void MarkLocalAsOuter(SQInteger pos);
    SQInteger GetOuterVariable(const SQObject &name);
    SQInteger GenerateCode();
    SQInteger GetStackSize();
    SQInteger CalcStackFrameSize();
    void AddLineInfos(SQInteger line,bool lineop,bool force=false);
    SQInteger AllocStackPos();
    SQInteger PushTarget(SQInteger n=-1);
    SQInteger PopTarget();
    SQInteger TopTarget();
    SQInteger GetUpTarget(SQInteger n);
    void DiscardTarget();
    bool IsLocal(SQUnsignedInteger stkpos);
    SQObject CreateString(const SQChar *s,SQInteger len = -1);
    SQObject CreateTable();
    bool IsConstant(const SQObject &name,SQObject &e);
    SQInteger _returnexp;
    SQLocalVarInfoVec _vlocals;
    SQIntVec _targetstack;
    SQInteger _stacksize;
    bool _varparams;
    bool _bgenerator;
    SQIntVec _unresolvedbreaks;
    SQIntVec _unresolvedcontinues;
    SQFunctionVec _functions;
    SQObjectPtrVec _parameters;
    SQOuterVarVec _outervalues;
    SQInstructionVec _instructions;
    SQLocalVarInfoVec _localvarinfos;
    SQObjectPtr _literals;
    SQObjectPtr _strings;
    SQObjectPtr _name;
    SQObjectPtr _sourcename;
    SQInteger _nliterals;
    SQLineInfoVec _lineinfos;
    SQFuncState *_parent;
    SQIntVec _scope_blocks;
    SQIntVec _breaktargets;
    SQIntVec _continuetargets;
    SQIntVec _defaultparams;
    SQInteger _lastline;
    SQInteger _traps; //contains number of nested exception traps
    SQInteger _outers;
    bool _optimization;
    SQSharedState *_sharedstate;
    std::vector<SQFuncState*> _childstates;
    SQInteger GetConstant(const SQObject &cons);
private:
    CompilerErrorFunc _errfunc;
    void *_errtarget;
    SQSharedState *_ss;
};


#endif //_SQFUNCSTATE_H_

