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


#ifndef DOBJECT_H
#define DOBJECT_H

#include "dchar.h"
#include "mem.h"
#include "property.h"
#include "value.h"
#include "text.h"

struct Dcomobject;
struct Dfunction;
class DSobjDispatch;
struct IDispatch;

struct Dobject : Mem		// pick up new/delete from Mem
{
    PropTable proptable;
    Dobject *internal_prototype;
    d_string classname;
    Value value;
#if !defined linux
    DSobjDispatch *com;		// if !=NULL, this is the COM wrapper for this Dobject
#endif

    Dobject(Dobject *prototype);

#if INVARIANT
#   define DOBJECT_SIGNATURE	0xAA31EE31
    unsigned signature;
    virtual void invariant();
#else
    void invariant() { }
#endif

    virtual Dobject *Prototype();
    virtual Value *GetLambda(d_string PropertyName, unsigned hash) { return Get(PropertyName, hash); }
    virtual Value *Get(d_string PropertyName, unsigned hash);
    virtual Value *Get(d_string PropertyName);
    virtual Value *Get(d_uint32 index);
    virtual Value *Put(d_string PropertyName, Value *value, unsigned attributes);
    virtual Value *Put(d_string PropertyName, Dobject *o, unsigned attributes);
    virtual Value *Put(d_string PropertyName, d_number n, unsigned attributes);
    virtual Value *Put(d_string PropertyName, d_string s, unsigned attributes);
    virtual Value *Put(d_uint32 index, Value *value, unsigned attributes);
    virtual int CanPut(d_string PropertyName);
    virtual int HasProperty(d_string PropertyName);
    virtual int Delete(d_string PropertyName);
    virtual int Delete(d_uint32 index);
    virtual int implementsDelete();
    virtual void *DefaultValue(Value *ret, dchar *Hint);
    virtual void *Construct(CONSTRUCT_ARGS);
    virtual void *Call(CALL_ARGS);
    virtual void *HasInstance(Value *ret, Value *v);
    virtual d_string getTypeof();

#if 0
    #define isDcomobject() __istype(Dcomobject)
#else
    virtual int isDcomobject();
    virtual IDispatch* toComObject();
#endif

    // COM support
    virtual Value *PutDefault(Value *value);
    virtual Value *put_Value(Value *ret, unsigned argc, Value *arglist);

    int isClass(d_string classname);

    // dynamic_class fails under GCC, so use these instead
    int isDarray() { return isClass(TEXT_Array); }
    int isDdate() { return isClass(TEXT_Date); }
    int isDregexp() { return isClass(TEXT_RegExp); }
    int isDenumerator() { return isClass(TEXT_Enumerator); }
    int isDvariant() { return isClass(TEXT_unknown); }
    int isDvbarray() { return isClass(TEXT_VBArray); }
    virtual int isDarguments();
    virtual int isCatch();
    virtual int isFinally();


    virtual void getErrInfo(ErrInfo *, int linnum);
    static Value *RuntimeError(ErrInfo *perrinfo, int msgnum, ...);
    static Value *RangeError(ErrInfo *perrinfo, int msgnum, ...);

    Value *putIterator(Value *v);

    static Dfunction *getConstructor();
    static Dobject *getPrototype();

    static void init(ThreadContext *tc);
};

struct Darguments : Dobject
{
    Dobject *actobj;		// activation object
    unsigned nparams;
    Array *parameters;

    Darguments(Dobject *caller, Dobject *callee, Dobject *actobj,
	Array *parameters, unsigned argc, Value *arglist);

    Value *Get(d_string PropertyName);
    Value *Get(d_uint32 index);
    Value *Put(d_string PropertyName, Value *value, unsigned attributes);
    Value *Put(d_string PropertyName, Dobject *o, unsigned attributes);
    Value *Put(d_string PropertyName, d_number n, unsigned attributes);
    Value *Put(d_uint32 index, Value *value, unsigned attributes);
    int CanPut(d_string PropertyName);
    int HasProperty(d_string PropertyName);
    int Delete(d_string PropertyName);

    int isDarguments() { return TRUE; }
};

struct Dfunction : Dobject
{   char *name;

    Array *scopex; // Function object's scope chain per 13.2 step 7
    size_t scopex_dim;	// amount used in scopex[]

    Dfunction(d_uint32 length, Dobject *prototype);
    Dfunction(d_uint32 length);
    d_string getTypeof();
    virtual d_string toString();
    void *HasInstance(Value *ret, Value *v);
//    void *Construct(CONSTRUCT_ARGS);

    static Dfunction *isFunction(Value *v);

    static void init(ThreadContext *tc);

    static Dfunction *getConstructor();
    static Dobject *getPrototype();
};

struct Dglobal : Dobject
{
    Dglobal(int argc, char *argv[]);
};

struct Darray : Dobject
{
    Vnumber length;		// length property

    Darray();
    Darray(Dobject *prototype);

    Value *Put(d_string PropertyName, Value *value, unsigned attributes);
    Value *Put(d_string PropertyName, Dobject *o, unsigned attributes);
    Value *Put(d_string PropertyName, d_number n, unsigned attributes);
    Value *Put(d_string PropertyName, d_string string, unsigned attributes);
    Value *Put(d_uint32 index, Value *value, unsigned attributes);
    Value *Put(d_uint32 index, d_string string, unsigned attributes);
    Value *Get(d_string PropertyName, unsigned hash);
    Value *Get(d_uint32);
    int Delete(d_string PropertyName);
    int Delete(d_uint32 index);

    static void init(ThreadContext *tc);

    static Dfunction *getConstructor();
    static Dobject *getPrototype();
};

struct Dstring : Dobject
{
    Dstring(d_string);
    Dstring(Dobject *prototype);

    static void init(ThreadContext *tc);

    static Dfunction *getConstructor();
    static Dobject *getPrototype();

    static int isStrWhiteSpaceChar(dchar c);
    static d_number toNumber(d_string s, dchar **p_endptr);
#if defined UNICODE
    static d_string dup(Mem *mem, char *p);
#endif
#if !ASCIZ
    static d_string dup(Mem *mem, dchar *p);
#endif
    static d_string dup(Mem *mem, d_string s);
    static d_string dup2(Mem *mem, d_string s1, d_string s2);
    static d_string substring(d_string string, int start, int end);
#if !ASCIZ
    static d_string substring(dchar *string, int start, int end);
#endif
    static int cmp(d_string s1, d_string s2);
    static d_string alloc(d_uint32 length);
    static d_uint32 len(d_string s);
};

struct Dboolean : Dobject
{
    Dboolean(d_boolean);
    Dboolean(Dobject *prototype);

    static void init(ThreadContext *tc);

    static Dfunction *getConstructor();
    static Dobject *getPrototype();
};

struct Dnumber : Dobject
{
    Dnumber(d_number);
    Dnumber(Dobject *prototype);

    static void init(ThreadContext *tc);

    static Dfunction *getConstructor();
    static Dobject *getPrototype();
};

struct Derror : Dobject
{
    Derror(Value *message, Value *v2);
    Derror(Dobject *prototype);

    static void init(ThreadContext *tc);

    static Dfunction *getConstructor();
    static Dobject *getPrototype();
};

#define NativeError(n)				\
    struct D##n##error : Dobject		\
    {						\
	D##n##error(dchar *message);		\
	D##n##error(ErrInfo *perrinfo);		\
	D##n##error(Dobject *prototype);	\
						\
	ErrInfo errinfo;			\
	virtual void getErrInfo(ErrInfo *, int linnum);	\
						\
	static void init(ThreadContext *tc);	\
						\
	static Dfunction *getConstructor();	\
	static Dobject *getPrototype();		\
    }

NativeError(eval);
NativeError(range);
NativeError(reference);
NativeError(syntax);
NativeError(type);
NativeError(uri);

#undef NativeError

struct Dmath : Dobject
{
    Dmath(ThreadContext *tc);

    static void init(ThreadContext *tc);
};

struct Ddate : Dobject
{
    Ddate(d_number n);
    Ddate(Dobject *prototype);

    static void init(ThreadContext *tc);

    static Dfunction *getConstructor();
    static Dobject *getPrototype();
};

#define BUILTIN_FUNCTION(prefix,n,l)				\
    struct prefix##n : Dfunction				\
    {								\
	prefix##n() : Dfunction(l)				\
		{ name = #n; }					\
	prefix##n(Dobject *o) : Dfunction(l,o)			\
		{ name = #n; }					\
	void *Call(CALL_ARGS);					\
    };								\
    void *prefix##n::Call(CALL_ARGS)


#define BUILTIN_FUNCTION2(prefix,n,l)				\
    void *prefix##n(Dobject *pthis, CALL_ARGS)

typedef void *(*PCall)(Dobject *pthis, CALL_ARGS);

struct NativeFunctionData
{
    d_string *string;
    char *name;
    PCall pcall;
    d_uint32 length;
};

struct DnativeFunction : Dfunction
{
    PCall pcall;

    DnativeFunction(PCall func, char *name, d_uint32 length);
    DnativeFunction(PCall func, char *name, d_uint32 length, Dobject *prototype);

    void *Call(CALL_ARGS);

    static void init(Dobject *o, NativeFunctionData *nfd, int nfd_dim, unsigned attributes);
};

#endif
