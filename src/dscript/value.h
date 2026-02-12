/* Digital Mars DMDScript source code.
 * Copyright (c) 2000-2001 by Chromium Communications
 * Copyright (c) 2004-2007 by Digital Mars
 * All Rights Reserved.
 * written by Walter Bright
 * http://www.digitalmars.com/dscript/cppscript.html
 *
 * This software is for evaluation purposes only.
 * It may not be redistributed in binary or source form,
 * incorporated into any product or program,
 * or used for any purpose other than evaluating it.
 * To purchase a license,
 * see http://www.digitalmars.com/dscript/cpplicense.html
 *
 * Use at your own risk. There is no warranty, express or implied.
 */

#ifndef VALUE_H
#define VALUE_H

#include <string.h>

#include "mem.h"
#include "lstring.h"
#include "dscript.h"

#define VALUE_INVARIANT	1	// do invariant checking

struct Dobject;
struct Vnumber;
struct CallContext;

// Porting issues:
// A lot of scaling is done on arrays of Value's. Therefore, adjusting
// it to come out to a size of 16 bytes makes the scaling an efficient
// operation. In fact, in some cases (opcodes.c) we prescale the addressing
// by 16 bytes at compile time instead of runtime.
// So, Value must be looked at in any port to verify that:
// 1) the size comes out as 16 bytes, padding as necessary
// 2) Value::copy() copies the used data bytes, NOT the padding.
//    It's faster to not copy the padding, and the
//    padding can contain garbage stack pointers which can
//    prevent memory from being garbage collected.
// 3) We use VPTR_HACK because it's faster. Needs to be verified
//    for each port.


#ifdef __GNUC__
#define VPTR_HACK	1	// use non-portable vptr hack
#elif _MSC_VER
#define VPTR_HACK	1	// use non-portable vptr hack
#elif __DMC__
#define VPTR_HACK	1	// use non-portable vptr hack
#else
#define VPTR_HACK	0	// don't use vptr hack
#endif

// VPTR_INDEX is the ith pointer where the vptr is stored in the Value object

#if !defined(VPTR_INDEX)
#ifdef __GNUC__
#define VPTR_INDEX	3
#endif

#ifdef _MSC_VER
#define VPTR_INDEX	0
#endif

#ifdef __DMC__
#define VPTR_INDEX	0
#endif
#endif

struct Value
{
    union
    {
	d_boolean boolean;	// can be true or false
	d_number number;
	struct
	{   unsigned hash;
	    Lstring *string;
	} x;
	Dobject *object;
	d_int32  int32;
	d_uint32 uint32;
	d_uint16 uint16;
	void *safearray;	// used by Vvbarray

	void *ptrvalue;		// used by lexer and by symbol table management
    };
#if defined __GNUC__
    int padforgcc;		// make sizeof(Value) come out to 16 bytes
#endif

    static void init();

    Value() { }

    virtual void invariant() { }

    // Use our own instead of from struct Mem because some compilers (like GCC)
    // add size to Value even though Mem has no data members.
    // If you derive from Mem instead, the GCC compiled version will crash,
    // because the code assumes (sizeof(Value)==16)
    void * operator new(size_t m_size);
    void * operator new(size_t m_size, Mem *mymem);
    void * operator new(size_t m_size, GC *mygc);
    void operator delete(void *p);

    static void copy(Value *to, Value *from)
    {
#if defined __DMC__
	(sizeof(Value) == 16) ?
	(
	    (((long long *)to)[0] = ((long long *)from)[0]),
	    (((long *)to)[2] = ((long *)from)[2])
	    // The last word is just padding, so we skip it
	)
	:
	    (void)memcpy(to, from, sizeof(Value));
#elif defined _MSC_VER
	((long *)to)[0] = ((long *)from)[0];
	((long *)to)[2] = ((long *)from)[2];
	((long *)to)[3] = ((long *)from)[3];
#else
	memcpy(to, from, sizeof(Value));
#endif
    }

    virtual void *toPrimitive(Value *ret, dchar *PreferredType);
    virtual d_boolean toBoolean();
    virtual d_number toNumber();
    virtual d_number toInteger();
    virtual d_int32 toInt32();
    virtual d_uint32 toUint32();
    virtual d_uint16 toUint16();
    virtual d_string toString();
    virtual d_string toLocaleString();
    virtual d_string toString(int radix);
    virtual d_string toSource();
    virtual Dobject *toObject();

    virtual int compareTo(Value *v);
    void copyTo(Value *v);
    virtual dchar *getType();    // use instead of slow dynamic_cast
    virtual d_string getTypeof();
    virtual int isPrimitive();
    virtual int isUndefined();
    virtual int isUndefinedOrNull();
#if VPTR_HACK
    static void *vptr_Number;
    static void *vptr_String;
    static void *vptr_Date;
    //static void *vptr_VBArray;
#endif
#if ISTYPE
    #define isNumber() __istype(Vnumber)
    #define isString() __istype(Vstring)
    #define isDate() __istype(Vdate)
    //#define isVBArray() __istype(Vvbarray)
#elif VPTR_HACK
    // Note: [VPTR_INDEX] is where the vptr is stored. Non-portable hack.
    int isNumber() { return (((void **)this)[VPTR_INDEX] == vptr_Number); }
    int isString() { return (((void **)this)[VPTR_INDEX] == vptr_String); }
    int isDate()   { return (((void **)this)[VPTR_INDEX] == vptr_Date); }
    //int isVBArray(){ return (((void **)this)[VPTR_INDEX] == vptr_VBArray); }
#else
    // portable, but slower, version
    virtual int isNumber();
    virtual int isString();
    virtual int isDate();
    //virtual int isVBArray();
#endif
    virtual int isArrayIndex(d_uint32 *index);
    virtual unsigned getHash();

    virtual Value *Put(d_string PropertyName, Value *value);
    virtual Value *Put(d_uint32 index, Value *value);
    virtual Value *Get(d_string PropertyName);
    virtual Value *Get(d_uint32 index);
    virtual Value *Get(d_string PropertyName, unsigned hash);
    virtual void *Construct(CONSTRUCT_ARGS);
    virtual void *Call(CALL_ARGS);
    virtual Value *putIterator(Value *v);

    virtual void getErrInfo(ErrInfo *, int linnum);

    void dump();
};

struct Vundefined : Value
{
    Vundefined();
    void invariant();

    d_boolean toBoolean();
    d_number toNumber();
    d_number toInteger();
    d_int32 toInt32();
    d_uint32 toUint32();
    d_uint16 toUint16();
    d_string toString();
    Dobject *toObject();
    unsigned getHash();
    int isUndefined();
    int isUndefinedOrNull();
    int compareTo(Value *v);
    dchar *getType();
    d_string getTypeof();
};

extern Vundefined vundefined;

struct Vnull : Value
{
    Vnull();

    d_boolean toBoolean();
    d_number toNumber();
    d_number toInteger();
    d_int32 toInt32();
    d_uint32 toUint32();
    d_uint16 toUint16();
    d_string toString();
    Dobject *toObject();
    unsigned getHash();
    int isUndefinedOrNull();
    int compareTo(Value *v);
    dchar *getType();
    d_string getTypeof();
};

extern Vnull vnull;

struct Vboolean : Value
{
    static Vboolean tmp;

    static void putValue(Value *v, d_boolean b)
    {
    #if defined __DMC__
	*(long *)v = *(long *)&tmp;		// stuff vptr
	v->boolean = b;
    #else
	Vboolean tmp(b);

	Value::copy(v, &tmp);
    #endif
    }

    Vboolean(d_boolean b) { boolean = b; }

    d_boolean toBoolean();
    d_number toNumber();
    d_number toInteger();
    d_int32 toInt32();
    d_uint32 toUint32();
    d_uint16 toUint16();
    d_string toString();
    Dobject *toObject();
    unsigned getHash();
    int compareTo(Value *v);
    dchar *getType();
    d_string getTypeof();
};

struct Vstring : Value
{
    static unsigned calcHash(d_string s);

    static void putValue(Value *v, dchar *s);

    static void Vstring::putValue(Value *v, Lstring *s)
    {
	//assert(s);
#if VPTR_HACK
	((void **)v)[VPTR_INDEX] = Value::vptr_String;
	v->x.hash = 0;
	v->x.string = s;
#else
	Vstring tmp(s);
	Value::copy(v, &tmp);
#endif
    }

    Vstring(Lstring *s)
    {
	x.hash = 0;
	x.string = s;
    }
    Vstring(Lstring *s, unsigned hash)
    {
	x.hash = hash;
	x.string = s;
    }
    Vstring(dchar *s)
    {
	x.hash = 0;
	x.string = Lstring::ctor(s);
    }
    Vstring(dchar *s, unsigned hash)
    {
	x.hash = hash;
	x.string = Lstring::ctor(s);
    }
    d_boolean toBoolean();
    d_number toNumber();
    d_string toString();
    d_string toSource();
    Dobject *toObject();
    int isArrayIndex(d_uint32 *index);
    unsigned getHash();
    int compareTo(Value *v);
#if !ISTYPE && !VPTR_HACK
    int isString();
#endif
    dchar *getType();
    d_string getTypeof();
};

struct Vnumber : Value
{
    static Vnumber tmp;

    static void putValue(Value *v, d_number n)
    {
    #if defined __DMC__
	*(long *)v = *(long *)&tmp;		// stuff vptr
	v->number = n;
    #elif VPTR_HACK
	((void **)v)[VPTR_INDEX] = Value::vptr_Number;
	v->number = n;
    #else
	Vnumber tmp(n);

	Value::copy(v, &tmp);
    #endif
    }

    Vnumber(d_number n) { this->number = n; }

    d_boolean toBoolean();
    d_number toNumber();
    d_string toString();
    d_string toString(int radix);
    Dobject *toObject();
    int isArrayIndex(d_uint32 *index);
    unsigned getHash();
    int compareTo(Value *v);
#if !ISTYPE && !VPTR_HACK
    int isNumber();
#endif
    dchar *getType();
    d_string getTypeof();
};

struct Vobject : Value
{
    static void putValue(Value *v, Dobject *o);
    Vobject(Dobject *o) { this->object = o; }

    void *toPrimitive(Value *ret, dchar *PreferredType);
    d_boolean toBoolean();
    d_number toNumber();
    d_string toString();
    d_string toSource();
    Dobject *toObject();
    int isPrimitive();
    unsigned getHash();
    int compareTo(Value *v);

    Value *Put(d_string PropertyName, Value *value);
    Value *Put(d_uint32 index, Value *value);
    Value *Get(d_string PropertyName);
    Value *Get(d_uint32 index);
    Value *Get(d_string PropertyName, unsigned hash);
    dchar *getType();
    d_string getTypeof();
    void *Construct(CONSTRUCT_ARGS);
    void *Call(CALL_ARGS);
    Value *putIterator(Value *v);

    void getErrInfo(ErrInfo *, int linnum);
};

// Support for VT_DATE

struct Vdate : Value
{
    static Vdate tmp;

    static void putValue(Value *v, d_number n)
    {
    #if defined __DMC__
	*(long *)v = *(long *)&tmp;		// stuff vptr
	v->number = n;
    #elif VPTR_HACK
	((void **)v)[VPTR_INDEX] = Value::vptr_Date;
	v->number = n;
    #else
	Vdate tmp(n);

	Value::copy(v, &tmp);
    #endif
    }

    Vdate(d_number n) { this->number = n; }

    d_boolean toBoolean();
    d_number toNumber();
    d_string toString();
    Dobject *toObject();
    unsigned getHash();
    int compareTo(Value *v);
#if !ISTYPE && !VPTR_HACK
    int isDate();
#endif
    dchar *getType();
    d_string getTypeof();
};

#if 0
// Support for VT_VARIANT | VT_ARRAY

struct Vvbarray : Value
{
    static Vvbarray tmp;

    static void putValue(Value *v, void *p)
    {
    #if defined __DMC__
	*(long *)v = *(long *)&tmp;		// stuff vptr
	v->safearray = p;
    #elif VPTR_HACK
	((void **)v)[VPTR_INDEX] = Value::vptr_VBArray;
	v->safearray = p;
    #else
	Vvbarray tmp(p);

	Value::copy(v, &tmp);
    #endif
    }

    Vvbarray(void *p) { this->safearray = p; }

    d_boolean toBoolean();
    d_number toNumber();
    d_string toString();
    Dobject *toObject();
    unsigned getHash();
    int compareTo(Value *v);
#if !ISTYPE && !VPTR_HACK
    int isVBArray();
#endif
    dchar *getType();
    d_string getTypeof();
};
#endif

extern dchar TypeUndefined[];
extern dchar TypeNull[];
extern dchar TypeBoolean[];
extern dchar TypeNumber[];
extern dchar TypeString[];
extern dchar TypeObject[];

extern dchar TypeDate[];
//extern dchar TypeVBArray[];


#endif

