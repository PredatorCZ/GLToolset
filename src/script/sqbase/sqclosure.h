/*  see copyright notice in squirrel.h */
#ifndef _SQCLOSURE_H_
#define _SQCLOSURE_H_
#include "sqstate.h"
#include "script/detail/funcproto.hpp"
#include "sqfuncproto.h"
#include "sqstring.h"
#include "sqvm.h"

#define _CALC_CLOSURE_SIZE(func) (sizeof(SQClosure) + (func->outerVars.numItems*sizeof(SQObjectPtr)) + (func->defaultParams.numItems*sizeof(SQObjectPtr)))


struct SQClass;
struct SQClosure : public CHAINABLE_OBJ
{
private:
    SQClosure(SQSharedState *ss,prime::script::FuncProto *func){_function = func; _base = NULL; INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this); _env = NULL; _root=NULL; _fproto=NULL;}
public:
    static SQClosure *Create(SQSharedState *ss,SQFunctionProto *func,SQWeakRef *root){
        prime::script::FuncProto *f = *func;
        __ObjAddRef(func);
        SQClosure *nc = Create(ss, f, root);
        nc->_fproto = func;
        return nc;
    }
    static SQClosure *Create(SQSharedState *ss,prime::script::FuncProto *f,SQWeakRef *root){
        SQInteger size = _CALC_CLOSURE_SIZE(f);
        SQClosure *nc=(SQClosure*)SQ_MALLOC(size);
        new (nc) SQClosure(ss,f);
        nc->_outervalues = (SQObjectPtr *)(nc + 1);
        nc->_defaultparams = &nc->_outervalues[f->outerVars.numItems];
        nc->_root = root;
        std::string_view srcName(f->sourceName);
        std::string_view cloName(f->name);
        nc->_sourcename = SQString::Create(ss, srcName.data(), srcName.size());
        nc->_name = SQString::Create(ss, cloName.data(), cloName.size());
         __ObjAddRef(nc->_root);
        _CONSTRUCT_VECTOR(SQObjectPtr,f->outerVars.numItems,nc->_outervalues);
        _CONSTRUCT_VECTOR(SQObjectPtr,f->defaultParams.numItems,nc->_defaultparams);
        return nc;
    }
    void Release(){
        SQInteger size = _CALC_CLOSURE_SIZE(_function);
        _DESTRUCT_VECTOR(SQObjectPtr,_function->outerVars.numItems,_outervalues);
        _DESTRUCT_VECTOR(SQObjectPtr,_function->defaultParams.numItems,_defaultparams);
        __ObjRelease(_fproto);
        this->~SQClosure();
        sq_vm_free(this,size);
    }
    void SetRoot(SQWeakRef *r)
    {
        __ObjRelease(_root);
        _root = r;
        __ObjAddRef(_root);
    }
    SQClosure *Clone()
    {
        SQClosure * ret = nullptr;
        if (_fproto) {
            ret = SQClosure::Create(_opt_ss(this),_fproto,_root);
        } else {
            ret = SQClosure::Create(_opt_ss(this),_function,_root);
        }

        ret->_env = _env;
        if(ret->_env) __ObjAddRef(ret->_env);
        _COPY_VECTOR(ret->_outervalues,_outervalues,_function->outerVars.numItems);
        _COPY_VECTOR(ret->_defaultparams,_defaultparams,_function->defaultParams.numItems);
        return ret;
    }
    ~SQClosure();

#ifndef NO_GARBAGE_COLLECTOR
    void Mark(SQCollectable **chain);
    void Finalize(){
        _NULL_SQOBJECT_VECTOR(_outervalues,_function->outerVars.numItems);
        _NULL_SQOBJECT_VECTOR(_defaultparams,_function->defaultParams.numItems);
    }
    SQObjectType GetType() {return OT_CLOSURE;}
#endif
    SQWeakRef *_env;
    SQWeakRef *_root;
    SQClass *_base;
    prime::script::FuncProto *_function;
    SQFunctionProto *_fproto;
    SQObjectPtr _sourcename;
    SQObjectPtr _name;
    SQObjectPtr *_outervalues;
    SQObjectPtr *_defaultparams;
};

//////////////////////////////////////////////
struct SQOuter : public CHAINABLE_OBJ
{

private:
    SQOuter(SQSharedState *ss, SQObjectPtr *outer){_valptr = outer; _next = NULL; INIT_CHAIN(); ADD_TO_CHAIN(&_ss(this)->_gc_chain,this); }

public:
    static SQOuter *Create(SQSharedState *ss, SQObjectPtr *outer)
    {
        SQOuter *nc  = (SQOuter*)SQ_MALLOC(sizeof(SQOuter));
        new (nc) SQOuter(ss, outer);
        return nc;
    }
    ~SQOuter() { REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this); }

    void Release()
    {
        this->~SQOuter();
        sq_vm_free(this,sizeof(SQOuter));
    }

#ifndef NO_GARBAGE_COLLECTOR
    void Mark(SQCollectable **chain);
    void Finalize() { _value.Null(); }
    SQObjectType GetType() {return OT_OUTER;}
#endif

    SQObjectPtr *_valptr;  /* pointer to value on stack, or _value below */
    SQInteger    _idx;     /* idx in stack array, for relocation */
    SQObjectPtr  _value;   /* value of outer after stack frame is closed */
    SQOuter     *_next;    /* pointer to next outer when frame is open   */
};

//////////////////////////////////////////////
struct SQGenerator : public CHAINABLE_OBJ
{
    enum SQGeneratorState{eRunning,eSuspended,eDead};
private:
    SQGenerator(SQSharedState *ss,SQClosure *closure){_closure=closure;_state=eRunning;_ci._generator=NULL;INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this);}
public:
    static SQGenerator *Create(SQSharedState *ss,SQClosure *closure){
        SQGenerator *nc=(SQGenerator*)SQ_MALLOC(sizeof(SQGenerator));
        new (nc) SQGenerator(ss,closure);
        return nc;
    }
    ~SQGenerator()
    {
        REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
    }
    void Kill(){
        _state=eDead;
        _stack.resize(0);
        _closure.Null();}
    void Release(){
        sq_delete(this,SQGenerator);
    }

    bool Yield(SQVM *v,SQInteger target);
    bool Resume(SQVM *v,SQObjectPtr &dest);
#ifndef NO_GARBAGE_COLLECTOR
    void Mark(SQCollectable **chain);
    void Finalize(){_stack.resize(0);_closure.Null();}
    SQObjectType GetType() {return OT_GENERATOR;}
#endif
    SQObjectPtr _closure;
    SQObjectPtrVec _stack;
    SQVM::CallInfo _ci;
    ExceptionsTraps _etraps;
    SQGeneratorState _state;
};

#define _CALC_NATVIVECLOSURE_SIZE(noutervalues) (sizeof(SQNativeClosure) + (noutervalues*sizeof(SQObjectPtr)))

struct SQNativeClosure : public CHAINABLE_OBJ
{
private:
    SQNativeClosure(SQSharedState *ss,SQFUNCTION func){_function=func;INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this); _env = NULL;}
public:
    static SQNativeClosure *Create(SQSharedState *ss,SQFUNCTION func,SQInteger nouters)
    {
        SQInteger size = _CALC_NATVIVECLOSURE_SIZE(nouters);
        SQNativeClosure *nc=(SQNativeClosure*)SQ_MALLOC(size);
        new (nc) SQNativeClosure(ss,func);
        nc->_outervalues = (SQObjectPtr *)(nc + 1);
        nc->_noutervalues = nouters;
        _CONSTRUCT_VECTOR(SQObjectPtr,nc->_noutervalues,nc->_outervalues);
        return nc;
    }
    SQNativeClosure *Clone()
    {
        SQNativeClosure * ret = SQNativeClosure::Create(_opt_ss(this),_function,_noutervalues);
        ret->_env = _env;
        if(ret->_env) __ObjAddRef(ret->_env);
        ret->_name = _name;
        _COPY_VECTOR(ret->_outervalues,_outervalues,_noutervalues);
        ret->_typecheck = _typecheck;
        ret->_nparamscheck = _nparamscheck;
        return ret;
    }
    ~SQNativeClosure()
    {
        __ObjRelease(_env);
        REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
    }
    void Release(){
        SQInteger size = _CALC_NATVIVECLOSURE_SIZE(_noutervalues);
        _DESTRUCT_VECTOR(SQObjectPtr,_noutervalues,_outervalues);
        this->~SQNativeClosure();
        sq_free(this,size);
    }

#ifndef NO_GARBAGE_COLLECTOR
    void Mark(SQCollectable **chain);
    void Finalize() { _NULL_SQOBJECT_VECTOR(_outervalues,_noutervalues); }
    SQObjectType GetType() {return OT_NATIVECLOSURE;}
#endif
    SQInteger _nparamscheck;
    SQIntVec _typecheck;
    SQObjectPtr *_outervalues;
    SQUnsignedInteger _noutervalues;
    SQWeakRef *_env;
    SQFUNCTION _function;
    SQObjectPtr _name;
};



#endif //_SQCLOSURE_H_
