/*  see copyright notice in squirrel.h */
#ifndef _SQFUNCTION_H_
#define _SQFUNCTION_H_

#include "sqstate.h"
#include "sqvm.h"
#include "utils/playground.hpp"

struct SQFunctionProto : public CHAINABLE_OBJ
{
private:
    SQFunctionProto(SQSharedState *ss)
    {
        INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this);
        ss->_funcProtos->push_back(this);
    }

    ~SQFunctionProto()
    {
        REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
    }

public:
    static SQFunctionProto *Create(SQSharedState *ss)
    {
        SQFunctionProto *f;
        f = (SQFunctionProto *)sq_vm_malloc(sizeof(SQFunctionProto));
        new (f) SQFunctionProto(ss);
        return f;
    }
    void Release(){
        this->~SQFunctionProto();
        sq_vm_free(this,sizeof(SQFunctionProto));
    }

#ifndef NO_GARBAGE_COLLECTOR
    void Mark(SQCollectable **);
    void Finalize(){}
    SQObjectType GetType() {return OT_FUNCPROTO;}
#endif

    prime::utils::PlayGround playground;

    operator prime::script::FuncProto *()
    {
        return playground.As<prime::script::FuncProto>();
    }
};

#endif //_SQFUNCTION_H_
