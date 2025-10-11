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

#include <windows.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <assert.h>

#include "gc.h"

#include "mem.h"
#include "dchar.h"
#include "root.h"

#include "dscript.h"
#include "dobject.h"
#include "iterator.h"
#include "dregexp.h"
#include "text.h"
#include "program.h"

#if 0
#include <objbase.h>
#include <dispex.h>
#include "dsobjdispatch.h"
#endif

#define LOG 0

/************************** Invariant *************************/

#if INVARIANT

void Dobject::invariant()
{
    this->check(this);
    assert(signature == DOBJECT_SIGNATURE);
}

#endif

/************************** Dobject_constructor *************************/

struct Dobject_constructor : Dfunction
{
    Dobject_constructor(ThreadContext *tc);

    void *Construct(CONSTRUCT_ARGS);
    void *Call(CALL_ARGS);
};


Dobject_constructor::Dobject_constructor(ThreadContext *tc)
    : Dfunction(1, tc->Dfunction_prototype)
{
    if (tc->Dobject_prototype)
	Put(TEXT_prototype, tc->Dobject_prototype, DontEnum | DontDelete | ReadOnly);
}

void *Dobject_constructor::Call(CALL_ARGS)
{   Dobject *o;
    void *result;

    // ECMA 15.2.1
    if (argc == 0)
    {
	result = Construct(cc, ret, argc, arglist);
    }
    else
    {	Value *v;

	v = &arglist[0];
	if (v->isUndefinedOrNull())
	    result = Construct(cc, ret, argc, arglist);
	else
	{
	    o = v->toObject();
	    Vobject::putValue(ret, o);
	    result = NULL;
	}
    }
    return result;
}

void *Dobject_constructor::Construct(CONSTRUCT_ARGS)
{   Dobject *o;
    Value *v;

    // ECMA 15.2.2
    if (argc == 0)
    {
	GC_LOG();
	o = new(this) Dobject(Dobject::getPrototype());
    }
    else
    {
	v = &arglist[0];
	if (v->isPrimitive())
	{
	    if (v->isUndefinedOrNull())
	    {
		GC_LOG();
		o = new(this) Dobject(Dobject::getPrototype());
	    }
	    else
		o = v->toObject();
	}
	else
	    o = v->toObject();
    }
    //printf("constructed object o=%p, v=%p,'%s'\n", o, v,v->getType());
    Vobject::putValue(ret, o);
    return NULL;
}

/* ===================== Dobject_prototype_toString ================ */

BUILTIN_FUNCTION(Dobject_prototype_, toString, 0)
{
    d_string s;
    d_string string;

#if LOG
    WPRINTF(L"Dobject.prototype.toString(ret = %x)\n", ret);
#endif
    s = othis->classname;
#if 0
    // Should we do [object] or [object Object]?
    if (s == TEXT_Object)
        string = TEXT_bobjectb;
    else
#endif
    {
	unsigned len = 8 + d_string_len(s) + 1;
	string = Dstring::alloc(len);
#if defined UNICODE
	SWPRINTF(d_string_ptr(string), len + 1, DTEXT("[object %s]"), d_string_ptr(s)); 
#else
        sprintf(d_string_ptr(string), "[object %s]", d_string_ptr(s)); 
#endif
#ifdef DEBUG
        assert(d_string_len(string) == (unsigned)Dchar::len(d_string_ptr(string)));
#endif
    }
    Vstring::putValue(ret, string);
    return NULL;
}

/* ===================== Dobject_prototype_toLocaleString ================ */

BUILTIN_FUNCTION(Dobject_prototype_, toLocaleString, 0)
{
    // ECMA v3 15.2.4.3
    //	"This function returns the result of calling toString()."

    Value *v;

    //WPRINTF(L"Dobject.prototype.toLocaleString(ret = %x)\n", ret);
    v = othis->Get(TEXT_toString);
    if (v && !v->isPrimitive())	// if it's an Object
    {   void *a;
	Dobject *o;

	o = v->object;
	a = o->Call(cc, othis, ret, argc, arglist);
	if (a)			// if exception was thrown
	    return a;
    }
    return NULL;
}

/* ===================== Dobject_prototype_valueOf ================ */

BUILTIN_FUNCTION(Dobject_prototype_, valueOf, 0)
{
    Vobject tmp(othis);
    Value::copy(ret, &tmp);
    return NULL;
}

/* ===================== Dobject_prototype_toSource ================ */

BUILTIN_FUNCTION(Dobject_prototype_, toSource, 0)
{
    Property *p;
    dchar *s;
    Value *v;
    OutBuffer buf;
    int any;

    //PRINTF("Dobject.prototype.toSource(this = %p, ret = %p)\n", this, ret);

    buf.writedchar('{');
    any = 0;
    for (p = othis->proptable.start; p; p = p->next)
    {
	if (!(p->attributes & (DontEnum | Deleted)))
	{
	    if (any)
		buf.writedchar(',');
	    any = 1;
	    v = &p->key;
	    buf.writedstring(d_string_ptr(v->toString()));
	    buf.writedchar(':');
	    v = &p->value;
	    buf.writedstring(d_string_ptr(v->toSource()));
	}
    }
    buf.writedchar('}');
    buf.writedchar(0);
    s = (dchar *)buf.data;
    buf.data = NULL;
    Vstring::putValue(ret, d_string_ctor(s));
    return NULL;
}

/* ===================== Dobject_prototype_hasOwnProperty ================ */

BUILTIN_FUNCTION(Dobject_prototype_, hasOwnProperty, 1)
{
    // ECMA v3 15.2.4.5
    Value *v;

    v = argc ? &arglist[0] : &vundefined;
    Vboolean::putValue(ret, othis->proptable.hasownproperty(v, 0));
    return NULL;
}

/* ===================== Dobject_prototype_isPrototypeOf ================ */

BUILTIN_FUNCTION(Dobject_prototype_, isPrototypeOf, 0)
{
    // ECMA v3 15.2.4.6
    d_boolean result = FALSE;
    Value *v;
    Dobject *o;

    v = argc ? &arglist[0] : &vundefined;
    if (!v->isPrimitive())
    {
	o = v->toObject();
	for (;;)
	{
	    o = o->internal_prototype;
	    if (!o)
		break;
	    if (o == othis)
	    {	result = TRUE;
		break;
	    }
	}
    }

    Vboolean::putValue(ret, result);
    return NULL;
}

/* ===================== Dobject_prototype_propertyIsEnumerable ================ */

BUILTIN_FUNCTION(Dobject_prototype_, propertyIsEnumerable, 0)
{
    // ECMA v3 15.2.4.7
    Value *v;

    v = argc ? &arglist[0] : &vundefined;
    Vboolean::putValue(ret, othis->proptable.hasownproperty(v, 1));
    return NULL;
}

/* ===================== Dobject_prototype ========================= */

struct Dobject_prototype : Dobject
{
    Dobject_prototype(ThreadContext *tc);
};

Dobject_prototype::Dobject_prototype(ThreadContext *tc)
    : Dobject(NULL)
{
}

/* ====================== Dobject ======================= */

Dobject::Dobject(Dobject *prototype)
{
    //WPRINTF(L"new Dobject = %x, prototype = %x, line = %d, file = '%s'\n", this, prototype, GC::line, ascii2unicode(GC::file));
    //PRINTF("Dobject::Dobject(prototype = %p)\n", prototype);
    internal_prototype = prototype;
    if (prototype)
	proptable.previous = &prototype->proptable;
    classname = TEXT_Object;
    Vobject::putValue(&value, this);
#if !defined linux
    com = NULL;
#endif
#if INVARIANT
    signature = DOBJECT_SIGNATURE;
#endif
    invariant();
}

Dobject *Dobject::Prototype()
{
    return internal_prototype;
}

Value *Dobject::Get(d_string PropertyName)
{
    return Get(PropertyName, d_string_hash(PropertyName));
}

Value *Dobject::Get(d_string PropertyName, unsigned hash)
{
    Value *v;

    invariant();
//WPRINTF(L"Dobject::Get(this = %x, '%ls')\n", this, d_string_ptr(PropertyName));
//PRINTF("\tinternal_prototype = %p\n", this->internal_prototype);
//PRINTF("\tDfunction::getPrototype() = %p\n", Dfunction::getPrototype());
    v = proptable.get(PropertyName, hash);
//if (v) PRINTF("found it %p\n", v->object);
    return v;
}

Value *Dobject::Get(d_uint32 index)
{
    Value *v;

    v = proptable.get(index);
//    if (!v)
//	v = &vundefined;
    return v;
}

Value *Dobject::Put(d_string PropertyName, Value *value, unsigned attributes)
{
    invariant();
    // ECMA 8.6.2.2
    //PRINTF("Dobject::Put(this = %p)\n", this);
    proptable.put(PropertyName, value, attributes);
    return NULL;
}

Value *Dobject::Put(d_string PropertyName, Dobject *o, unsigned attributes)
{
    // ECMA 8.6.2.2
    Vobject v(o);

    proptable.put(PropertyName, &v, attributes);
    return NULL;
}

Value *Dobject::Put(d_string PropertyName, d_number n, unsigned attributes)
{
    // ECMA 8.6.2.2
    Vnumber v(n);

    proptable.put(PropertyName, &v, attributes);
    return NULL;
}

Value *Dobject::Put(d_string PropertyName, d_string s, unsigned attributes)
{
    // ECMA 8.6.2.2
    Vstring v(s);

    proptable.put(PropertyName, &v, attributes);
    return NULL;
}

Value *Dobject::Put(d_uint32 index, Value *value, unsigned attributes)
{
    // ECMA 8.6.2.2
    proptable.put(index, value, attributes);
    return NULL;
}

Value *Dobject::PutDefault(Value *value)
{
    // Not ECMA, Microsoft extension
    //PRINTF("Dobject::PutDefault(this = %p)\n", this);
    ErrInfo errinfo;
    return RuntimeError(&errinfo, ERR_NO_DEFAULT_PUT);
}

Value *Dobject::put_Value(Value *ret, unsigned argc, Value *arglist)
{
    // Not ECMA, Microsoft extension
    //PRINTF("Dobject::put_Value(this = %p)\n", this);
    ErrInfo errinfo;
    return RuntimeError(&errinfo, ERR_FUNCTION_NOT_LVALUE);
}

int Dobject::CanPut(d_string PropertyName)
{
    // ECMA 8.6.2.3
    return proptable.canput(PropertyName);
}

int Dobject::HasProperty(d_string PropertyName)
{
    // ECMA 8.6.2.4
    return proptable.hasproperty(PropertyName);
}

/***********************************
 * Return:
 *	TRUE	not found or successful delete
 *	FALSE	property is marked with DontDelete attribute
 */

int Dobject::Delete(d_string PropertyName)
{
    // ECMA 8.6.2.5
    //WPRINTF(L"Dobject::Delete('%ls')\n", d_string_ptr(PropertyName));
    return proptable.del(PropertyName);
}

int Dobject::Delete(d_uint32 index)
{
    // ECMA 8.6.2.5
    return proptable.del(index);
}

int Dobject::implementsDelete()
{
    // ECMA 8.6.2 says every object implements [[Delete]],
    // but ECMA 11.4.1 says that some objects may not.
    // Assume the former is correct.
    return TRUE;
}

void *Dobject::DefaultValue(Value *ret, dchar *Hint)
{   Dobject *o;
    Value *v;
    static d_string *table[2] = { &TEXT_toString, &TEXT_valueOf };
    int i = 0;			// initializer necessary for /W4

    // ECMA 8.6.2.6
    //WPRINTF(L"Dobject::DefaultValue(ret = %x, Hint = '%ls')\n", ret, Hint ? Hint : DTEXT(""));
    if (Hint == TypeString ||
	(Hint == NULL && this->isDdate()))
    {
	i = 0;
    }
    else if (Hint == TypeNumber ||
	     Hint == NULL)
    {
	i = 1;
    }
    else
	assert(0);

    for (int j = 0; j < 2; j++)
    {	d_string htab = *table[i];

	v = Get(htab, d_string_hash(htab));
	if (v && !v->isPrimitive())	// if it's an Object
	{   void *a;
	    CallContext *cc;

	    o = v->object;
	    cc = Program::getProgram()->callcontext;
	    a = o->Call(cc, this, ret, 0, NULL);
	    if (a)			// if exception was thrown
		return a;
	    if (ret->isPrimitive())
		return NULL;
	}
	i ^= 1;
    }
    Vstring::putValue(ret, classname);
    return NULL;
    //ErrInfo errinfo;
    //return RuntimeError(&errinfo, DTEXT("no Default Value for object"));
}

void *Dobject::Construct(CONSTRUCT_ARGS)
{   ErrInfo errinfo;
    return RuntimeError(&errinfo, ERR_S_NO_CONSTRUCT, d_string_ptr(classname));
}

void *Dobject::Call(CALL_ARGS)
{
    ErrInfo errinfo;
    return RuntimeError(&errinfo, ERR_S_NO_CALL, d_string_ptr(classname));
}

void *Dobject::HasInstance(Value *ret, Value *v)
{   // ECMA v3 8.6.2
    ErrInfo errinfo;
    return RuntimeError(&errinfo, ERR_S_NO_INSTANCE, d_string_ptr(classname));
}

d_string Dobject::getTypeof()
{   // ECMA 11.4.3
    return TEXT_object;
}

int Dobject::isDcomobject()
{
    return FALSE;
}

int Dobject::isClass(d_string classname)
{
    return this->classname == classname ||
	d_string_cmp(this->classname, classname) == 0;
}

int Dobject::isDarguments()	{ return FALSE; }
int Dobject::isCatch()		{ return FALSE; }
int Dobject::isFinally()	{ return FALSE; }

void Dobject::getErrInfo(ErrInfo *perrinfo, int linnum)
{
    ErrInfo errinfo;
    Vobject o(this);

    errinfo.message = d_string_ptr(o.toString());
    if (perrinfo)
	*perrinfo = errinfo;
}

Value *Dobject::RuntimeError(ErrInfo *perrinfo, int msgnum, ...)
{   Dobject *o;
    OutBuffer buf;
    va_list ap;

    buf.reset();
    va_start(ap, msgnum);
    dchar *msg = errmsg(msgnum);
#if LOG
    WPRINTF(L"RuntimeError('%ls')\n", msg);
#endif
    buf.vprintf(msg, ap);
    va_end(ap);
    buf.writedchar(0);
    perrinfo->message = (dchar *)buf.data;
    buf.data = NULL;
    if (!perrinfo->code)
	perrinfo->code = errcodtbl[msgnum];
    o = new Dtypeerror(perrinfo);
    return new Vobject(o);
}

Value *Dobject::RangeError(ErrInfo *perrinfo, int msgnum, ...)
{   Dobject *o;
    OutBuffer buf;
    va_list ap;

    buf.reset();
    va_start(ap, msgnum);
    dchar *msg = errmsg(msgnum);
    //WPRINTF(L"RangeError('%ls')\n", msg);
    buf.vprintf(msg, ap);
    va_end(ap);
    buf.writedchar(0);
    perrinfo->message = (dchar *)buf.data;
    buf.data = NULL;
    if (!perrinfo->code)
	perrinfo->code = errcodtbl[msgnum];
    o = new Drangeerror(perrinfo);
    return new Vobject(o);
}

Value *Dobject::putIterator(Value *v)
{
    Iterator i(this);

    assert(sizeof(Iterator) <= sizeof(Value));
    memcpy(v, &i, sizeof(Iterator));
    return NULL;
}

#if 0
Dfunction *Dobject::constructor;
Dobject *Dobject::prototype;

Dfunction *Dobject::getConstructor()
{
    return constructor;
}

Dobject *Dobject::getPrototype()
{
    return prototype;
}

#else
Dfunction *Dobject::getConstructor()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Dobject_constructor;
}

Dobject *Dobject::getPrototype()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Dobject_prototype;
}
#endif

void Dobject::init(ThreadContext *tc)
{
    GC_LOG();
    tc->Dobject_prototype = new(&tc->mem) Dobject_prototype(tc);
    Dfunction::init(tc);
    GC_LOG();
    tc->Dobject_constructor = new(&tc->mem) Dobject_constructor(tc);

    Dobject *op = tc->Dobject_prototype;
    Dobject *f = tc->Dfunction_prototype;

    op->Put(TEXT_constructor, tc->Dobject_constructor, DontEnum);

    GC_LOG();
    op->Put(TEXT_toString, new(op) Dobject_prototype_toString(f), DontEnum);
    GC_LOG();
    op->Put(TEXT_toLocaleString, new(op) Dobject_prototype_toLocaleString(f), DontEnum);
    GC_LOG();
    op->Put(TEXT_toSource, new(op) Dobject_prototype_toSource(f), DontEnum);
    GC_LOG();
    op->Put(TEXT_valueOf, new(op) Dobject_prototype_valueOf(f), DontEnum);
    GC_LOG();
    op->Put(TEXT_hasOwnProperty, new(op) Dobject_prototype_hasOwnProperty(f), DontEnum);
    GC_LOG();
    op->Put(TEXT_isPrototypeOf, new(op) Dobject_prototype_isPrototypeOf(f), DontEnum);
    GC_LOG();
    op->Put(TEXT_propertyIsEnumerable, new(op) Dobject_prototype_propertyIsEnumerable(f), DontEnum);
}

/*********************************************
 * Initialize the built-in's.
 */

void dobject_init(ThreadContext *tc)
{
    //WPRINTF(L"dobject_init()\n");
    if (tc->Dobject_prototype)
	return;			// already initialized for this thread

    //WPRINTF(L"\tdoing()\n");
#if 0
WPRINTF(L"sizeof(Mem) = %d\n", sizeof(Mem));
WPRINTF(L"sizeof(Dobject) = %d\n", sizeof(Dobject));
WPRINTF(L"sizeof(PropTable) = %d\n", sizeof(PropTable));
WPRINTF(L"offsetof(proptable) = %d\n", offsetof(Dobject, proptable));
WPRINTF(L"offsetof(internal_prototype) = %d\n", offsetof(Dobject, internal_prototype));
WPRINTF(L"offsetof(classname) = %d\n", offsetof(Dobject, classname));
WPRINTF(L"offsetof(value) = %d\n", offsetof(Dobject, value));
#endif

    Dobject::init(tc);
    Dboolean::init(tc);
    Dstring::init(tc);
    Dnumber::init(tc);
    Darray::init(tc);
    Dmath::init(tc);
    Ddate::init(tc);
    Dregexp::init(tc);
    Derror::init(tc);
    Devalerror::init(tc);
    Drangeerror::init(tc);
    Dreferenceerror::init(tc);
    Dsyntaxerror::init(tc);
    Dtypeerror::init(tc);
    Durierror::init(tc);

    // COM specific
    Dcomobject_init(tc);
    Denumerator_init(tc);
    Dvbarray_init(tc);
}

void dobject_term(ThreadContext *tc)
{
    //WPRINTF(L"dobject_term(program = %x)\n", tc->program);

    memset(&tc->program, 0,
	sizeof(ThreadContext) - offsetof(ThreadContext, program));
}
