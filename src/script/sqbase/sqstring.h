/*  see copyright notice in squirrel.h */
#ifndef _SQSTRING_H_
#define _SQSTRING_H_
#include "sqobject.h"
#include <cstddef>

inline SQHash _hashstr (const SQChar *s, size_t l)
{
    SQHash h = 0;

    if (l < sizeof(SQHash)) {
        memcpy(&h, s, l);
        return h;
    }

    h = (SQHash)l;  /* seed */
	size_t step = (l >> 5) + 1;  /* if string is too long, don't hash all its chars */
	size_t l1;
	for (l1 = l; l1 >= step; l1 -= step)
		h = h ^ ((h << 5) + (h >> 2) + ((unsigned short)s[l1 - 1]));
	return h;
}

struct SQStringInfo {
    SQInt32 _valOffset : 8;
    SQInt32 _len : 24;
};

struct SQString : public SQRefCounted
{
    SQString(){}
    ~SQString(){}
public:
    static SQString *Create(SQSharedState *ss, const SQChar *, SQInteger len = -1 );
    SQInteger Next(const SQObjectPtr &refpos, SQObjectPtr &outkey, SQObjectPtr &outval);
    void Release();
    SQStringInfo &GetInfo() { return reinterpret_cast<SQStringInfo&>(_pad); }
    SQChar *Val() { return _val_ + GetInfo()._valOffset; }
    SQInt32 Size() { return GetInfo()._len; }
    union {
        SQChar _val_[1];
        struct {
            SQHash _hash;
            SQSharedState *_sharedstate;
            SQString *_next; //chain for the string table
        };
    };

};



#endif //_SQSTRING_H_
