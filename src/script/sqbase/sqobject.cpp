/*
    see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqstring.h"
#include "sqarray.h"
#include "sqtable.h"
#include "squserdata.h"
#include "sqclass.h"
#include "sqclosure.h"

namespace ps = prime::script;

const SQChar *IdType2Name(SQObjectType type)
{
    switch(_RAW_TYPE(type))
    {
    case _RT_NULL:return _SC("null");
    case _RT_INTEGER:return _SC("integer");
    case _RT_FLOAT:return _SC("float");
    case _RT_BOOL:return _SC("bool");
    case _RT_STRING:return _SC("string");
    case _RT_TABLE:return _SC("table");
    case _RT_ARRAY:return _SC("array");
    case _RT_GENERATOR:return _SC("generator");
    case _RT_CLOSURE:
    case _RT_NATIVECLOSURE:
        return _SC("function");
    case _RT_USERDATA:
    case _RT_USERPOINTER:
        return _SC("userdata");
    case _RT_THREAD: return _SC("thread");
    case _RT_FUNCPROTO: return _SC("function");
    case _RT_CLASS: return _SC("class");
    case _RT_INSTANCE: return _SC("instance");
    case _RT_WEAKREF: return _SC("weakref");
    case _RT_OUTER: return _SC("outer");
    default:
        return NULL;
    }
}

const SQChar *GetTypeName(const SQObjectPtr &obj1)
{
    return IdType2Name(sq_type(obj1));
}

SQString *SQString::Create(SQSharedState *ss,const SQChar *s,SQInteger len)
{
    SQString *str=ADD_STRING(ss,s,len);
    return str;
}

void SQString::Release()
{
    REMOVE_STRING(_sharedstate,this);
}

SQInteger SQString::Next(const SQObjectPtr &refpos, SQObjectPtr &outkey, SQObjectPtr &outval)
{
    SQInteger idx = (SQInteger)TranslateIndex(refpos);
    while(idx < Size()){
        outkey = (SQInteger)idx;
        outval = (SQInteger)((SQUnsignedInteger)Val()[idx]);
        //return idx for the next iteration
        return ++idx;
    }
    //nothing to iterate anymore
    return -1;
}

SQUnsignedInteger TranslateIndex(const SQObjectPtr &idx)
{
    switch(sq_type(idx)){
        case OT_NULL:
            return 0;
        case OT_INTEGER:
            return (SQUnsignedInteger)_integer(idx);
        default: assert(0); break;
    }
    return 0;
}

SQWeakRef *SQRefCounted::GetWeakRef(SQObjectType type)
{
    if(!_weakref) {
        sq_new(_weakref,SQWeakRef);
#if defined(SQUSEDOUBLE) && !defined(_SQ64)
        _weakref->_obj._unVal.raw = 0; //clean the whole union on 32 bits with double
#endif
        _weakref->_obj._type = type;
        _weakref->_obj._unVal.pRefCounted = this;
    }
    return _weakref;
}

SQRefCounted::~SQRefCounted()
{
    if(_weakref) {
        _weakref->_obj._type = OT_NULL;
        _weakref->_obj._unVal.pRefCounted = NULL;
    }
}

void SQWeakRef::Release() {
    if(ISREFCOUNTED(_obj._type)) {
        _obj._unVal.pRefCounted->_weakref = NULL;
    }
    sq_delete(this,SQWeakRef);
}

bool SQDelegable::GetMetaMethod(SQVM *v,SQMetaMethod mm,SQObjectPtr &res) {
    if(_delegate) {
        return _delegate->Get((*_ss(v)->_metamethods)[mm],res);
    }
    return false;
}

bool SQDelegable::SetDelegate(SQTable *mt)
{
    SQTable *temp = mt;
    if(temp == this) return false;
    while (temp) {
        if (temp->_delegate == this) return false; //cycle detected
        temp = temp->_delegate;
    }
    if (mt) __ObjAddRef(mt);
    __ObjRelease(_delegate);
    _delegate = mt;
    return true;
}

bool SQGenerator::Yield(SQVM *v,SQInteger target)
{
    if(_state==eSuspended) { v->Raise_Error(_SC("internal vm error, yielding dead generator"));  return false;}
    if(_state==eDead) { v->Raise_Error(_SC("internal vm error, yielding a dead generator")); return false; }
    SQInteger size = v->_top-v->_stackbase;

    _stack.resize(size);
    SQObject _this = v->_stack[v->_stackbase];
    _stack[0] = ISREFCOUNTED(sq_type(_this)) ? SQObjectPtr(_refcounted(_this)->GetWeakRef(sq_type(_this))) : _this;
    for(SQInteger n =1; n<target; n++) {
        _stack[n] = v->_stack[v->_stackbase+n];
    }
    for(SQInteger j =0; j < size; j++)
    {
        v->_stack[v->_stackbase+j].Null();
    }

    _ci = *v->ci;
    _ci._generator=NULL;
    for(SQInteger i=0;i<_ci._etraps;i++) {
        _etraps.push_back(v->_etraps.back());
        v->_etraps.pop_back();
        // store relative stack base and size in case of resume to other _top
        SQExceptionTrap &et = _etraps.back();
        et._stackbase -= v->_stackbase;
        et._stacksize -= v->_stackbase;
    }
    _state=eSuspended;
    return true;
}

bool SQGenerator::Resume(SQVM *v,SQObjectPtr &dest)
{
    if(_state==eDead){ v->Raise_Error(_SC("resuming dead generator")); return false; }
    if(_state==eRunning){ v->Raise_Error(_SC("resuming active generator")); return false; }
    SQInteger size = _stack.size();
    SQInteger target = &dest - &(v->_stack[v->_stackbase]);
    assert(target>=0 && target<=255);
    SQInteger newbase = v->_top;
    if(!v->EnterFrame(v->_top, v->_top + size, false))
        return false;
    v->ci->_generator   = this;
    v->ci->_target      = (SQInt32)target;
    v->ci->_closure     = _ci._closure;
    v->ci->_ip          = _ci._ip;
    v->ci->_literals    = _ci._literals;
    v->ci->_ncalls      = _ci._ncalls;
    v->ci->_etraps      = _ci._etraps;
    v->ci->_root        = _ci._root;


    for(SQInteger i=0;i<_ci._etraps;i++) {
        v->_etraps.push_back(_etraps.back());
        _etraps.pop_back();
        SQExceptionTrap &et = v->_etraps.back();
        // restore absolute stack base and size
        et._stackbase += newbase;
        et._stacksize += newbase;
    }
    SQObject _this = _stack[0];
    v->_stack[v->_stackbase] = sq_type(_this) == OT_WEAKREF ? _weakref(_this)->_obj : _this;

    for(SQInteger n = 1; n<size; n++) {
        v->_stack[v->_stackbase+n] = _stack[n];
        _stack[n].Null();
    }

    _state=eRunning;
    if (v->_debughook)
        v->CallDebugHook(_SC('c'));

    return true;
}

void SQArray::Extend(const SQArray *a){
    SQInteger xlen;
    if((xlen=a->Size()))
        for(SQInteger i=0;i<xlen;i++)
            Append(a->_values[i]);
}

SQClosure::~SQClosure()
{
    __ObjRelease(_root);
    __ObjRelease(_env);
    __ObjRelease(_base);
    REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
}

#ifndef NO_GARBAGE_COLLECTOR

#define START_MARK()    if(!(_uiRef&MARK_FLAG)){ \
        _uiRef|=MARK_FLAG;

#define END_MARK() RemoveFromChain(&_sharedstate->_gc_chain, this); \
        AddToChain(chain, this); }

void SQFunctionProto::Mark(SQCollectable **chain) {
    START_MARK()
    END_MARK()
}

void SQVM::Mark(SQCollectable **chain)
{
    START_MARK()
        SQSharedState::MarkObject(_lasterror,chain);
        SQSharedState::MarkObject(_errorhandler,chain);
        SQSharedState::MarkObject(_debughook_closure,chain);
        SQSharedState::MarkObject(_roottable, chain);
        SQSharedState::MarkObject(temp_reg, chain);
        for(SQUnsignedInteger i = 0; i < _stack.size(); i++) SQSharedState::MarkObject(_stack[i], chain);
        for(SQInteger k = 0; k < _callsstacksize; k++) SQSharedState::MarkObject(_callsstack[k]._closure, chain);
    END_MARK()
}

void SQArray::Mark(SQCollectable **chain)
{
    START_MARK()
        SQInteger len = _values.size();
        for(SQInteger i = 0;i < len; i++) SQSharedState::MarkObject(_values[i], chain);
    END_MARK()
}
void SQTable::Mark(SQCollectable **chain)
{
    START_MARK()
        if(_delegate) _delegate->Mark(chain);
        SQInteger len = _numofnodes;
        for(SQInteger i = 0; i < len; i++){
            SQSharedState::MarkObject(_nodes[i].key, chain);
            SQSharedState::MarkObject(_nodes[i].val, chain);
        }
    END_MARK()
}

void SQClass::Mark(SQCollectable **chain)
{
    START_MARK()
        _members->Mark(chain);
        if(_base) _base->Mark(chain);
        SQSharedState::MarkObject(_attributes, chain);
        for(SQUnsignedInteger i =0; i< _defaultvalues.size(); i++) {
            SQSharedState::MarkObject(_defaultvalues[i].val, chain);
            SQSharedState::MarkObject(_defaultvalues[i].attrs, chain);
        }
        for(SQUnsignedInteger j =0; j< _methods.size(); j++) {
            SQSharedState::MarkObject(_methods[j].val, chain);
            SQSharedState::MarkObject(_methods[j].attrs, chain);
        }
        for(SQUnsignedInteger k =0; k< MT_LAST; k++) {
            SQSharedState::MarkObject(_metamethods[k], chain);
        }
    END_MARK()
}

void SQInstance::Mark(SQCollectable **chain)
{
    START_MARK()
        _class->Mark(chain);
        SQUnsignedInteger nvalues = _class->_defaultvalues.size();
        for(SQUnsignedInteger i =0; i< nvalues; i++) {
            SQSharedState::MarkObject(_values[i], chain);
        }
    END_MARK()
}

void SQGenerator::Mark(SQCollectable **chain)
{
    START_MARK()
        for(SQUnsignedInteger i = 0; i < _stack.size(); i++) SQSharedState::MarkObject(_stack[i], chain);
        SQSharedState::MarkObject(_closure, chain);
    END_MARK()
}

void SQClosure::Mark(SQCollectable **chain)
{
    START_MARK()
        if(_base) _base->Mark(chain);
        ps::FuncProto *fp = _function;
        //fp->Mark(chain);
        for(SQInteger i = 0; i < fp->outerVars.numItems; i++) SQSharedState::MarkObject(_outervalues[i], chain);
        for(SQInteger k = 0; k < fp->defaultParams.numItems; k++) SQSharedState::MarkObject(_defaultparams[k], chain);
    END_MARK()
}

void SQNativeClosure::Mark(SQCollectable **chain)
{
    START_MARK()
        for(SQUnsignedInteger i = 0; i < _noutervalues; i++) SQSharedState::MarkObject(_outervalues[i], chain);
    END_MARK()
}

void SQOuter::Mark(SQCollectable **chain)
{
    START_MARK()
    /* If the valptr points to a closed value, that value is alive */
    if(_valptr == &_value) {
      SQSharedState::MarkObject(_value, chain);
    }
    END_MARK()
}

void SQUserData::Mark(SQCollectable **chain){
    START_MARK()
        if(_delegate) _delegate->Mark(chain);
    END_MARK()
}

void SQCollectable::UnMark() { _uiRef&=~MARK_FLAG; }

#endif

