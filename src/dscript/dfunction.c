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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if _MSC_VER
#include <malloc.h>
#endif

#include "root.h"
#include "dscript.h"
#include "value.h"
#include "dobject.h"
#include "statement.h"
#include "parse.h"
#include "scope.h"
#include "text.h"
#include "function.h"

/* ===================== Dfunction_constructor ==================== */

struct Dfunction_constructor : Dfunction
{
    Dfunction_constructor(ThreadContext *tc);

    void *Construct(CONSTRUCT_ARGS);
    void *Call(CALL_ARGS);
};

Dfunction_constructor::Dfunction_constructor(ThreadContext *tc)
    : Dfunction(1, tc->Dfunction_prototype)
{

    // Actually put in later by Dfunction::init()
    //unsigned attributes = DontEnum | DontDelete | ReadOnly;
    //Put(TEXT_prototype, Dfunction::getPrototype(), attributes);
}

void *Dfunction_constructor::Call(CALL_ARGS)
{
    // ECMA 15.3.1
    return Construct(cc, ret, argc, arglist);
}

void *Dfunction_constructor::Construct(CONSTRUCT_ARGS)
{
    // ECMA 15.3.2.1
    d_string body;
    d_string P;
    OutBuffer params;
    FunctionDefinition *fd;
    ErrInfo errinfo;

    //WPRINTF(L"Dfunction_constructor::Construct()\n");

    // Get parameter list (P) and body from arglist[]
    P = TEXT_;
    if (argc == 0)
	body = TEXT_;
    else
    {	unsigned a;

	body = arglist[argc - 1].toString();
	params.reserve(argc * 2 * sizeof(dchar));
	for (a = 0; a < argc - 1; a++)
	{
	    if (a)
		params.writedchar(',');
	    params.writedstring(d_string_ptr(arglist[a].toString()));
	}
	params.writedchar(0);
	P = d_string_ctor((dchar *)params.data);
	params.data = NULL;
    }

    if (Parser::parseFunctionDefinition(&fd, P, body, &errinfo))
	goto Lsyntaxerror;

    if (fd)
    {
	Scope sc = Scope(fd);

	fd->semantic(&sc);
	errinfo = sc.errinfo;
	if (errinfo.message)
	    goto Lsyntaxerror;
	fd->toIR(NULL);
	Dfunction *fobj = new(this) DdeclaredFunction(fd);
	Vobject::putValue(ret, fobj);
    }
    else
	Value::copy(ret, &vundefined);

    return NULL;

Lsyntaxerror:
    Dobject *o;

    Value::copy(ret, &vundefined);
    o = new(this) Dsyntaxerror(&errinfo);
    return new(this) Vobject(o);
}

/* ===================== Dfunction_prototype_toString =============== */

BUILTIN_FUNCTION(Dfunction_prototype_, toString, 0)
{
    d_string s;
    Dfunction *f;

    //PRINTF("function.prototype.toString()\n");
    // othis must be a Function
    if (!othis->isClass(TEXT_Function))
    {	ErrInfo errinfo;
	Value::copy(ret, &vundefined);
	return RuntimeError(&errinfo, ERR_TS_NOT_TRANSFERRABLE);
    }
    else
    {
	// Generate string that looks like a FunctionDeclaration
	// FunctionDeclaration:
	//	function Identifier (Identifier, ...) Block

	// If anonymous function, the name should be "anonymous"
	// per ECMA 15.3.2.1.19

	f = (Dfunction *)othis;
	s = f->toString();
	Vstring::putValue(ret, s);
    }
    return NULL;
}

/* ===================== Dfunction_prototype_apply =============== */

BUILTIN_FUNCTION(Dfunction_prototype_, apply, 2)
{
    // ECMA v3 15.3.4.3

    Value *thisArg;
    Value *argArray;
    Dobject *o;
    void *v;

    thisArg = &vundefined;
    argArray = &vundefined;
    switch (argc)
    {
	case 0:
	    break;
	default:
	    argArray = &arglist[1];
	case 1:
	    thisArg = &arglist[0];
	    break;
    }

    if (thisArg->isUndefinedOrNull())
	o = cc->global;
    else
	o = thisArg->toObject();

    if (argArray->isUndefinedOrNull())
    {
	v = othis->Call(cc, o, ret, 0, NULL);
    }
    else
    {
	if (argArray->isPrimitive())
	{
	  Ltypeerror:
	    Value::copy(ret, &vundefined);
	    ErrInfo errinfo;
	    return RuntimeError(&errinfo, ERR_ARRAY_ARGS);
	}
	Dobject *a;

	a = argArray->toObject();

	// Must be array or arguments object
	if (!a->isDarray() && !a->isDarguments())
	    goto Ltypeerror;

	unsigned len;
	unsigned i;
	Value *alist;
	Value *x;

	x = a->Get(TEXT_length);
	len = x ? x->toUint32() : 0;

	void *p1 = NULL;
	if (len >= 128 || (alist = (Value *)alloca(len * sizeof(Value))) == NULL)
	{   p1 = cc->malloc(len * sizeof(Value));
	    alist = (Value *)p1;
	}

	for (i = 0; i < len; i++)
	{
	    x = a->Get(i);
	    Value::copy(&alist[i], x);
	}

	v = othis->Call(cc, o, ret, len, alist);

	mem.free(p1);
    }
    return v;
}

/* ===================== Dfunction_prototype_call =============== */

BUILTIN_FUNCTION(Dfunction_prototype_, call, 1)
{
    // ECMA v3 15.3.4.4
    Value *thisArg;
    Dobject *o;
    void *v;

    if (argc == 0)
    {
	o = cc->global;
	v = othis->Call(cc, o, ret, argc, arglist);
    }
    else
    {
	thisArg = &arglist[0];
	if (thisArg->isUndefinedOrNull())
	    o = cc->global;
	else
	    o = thisArg->toObject();
	v = othis->Call(cc, o, ret, argc - 1, arglist + 1);
    }
    return v;
}

/* ===================== Dfunction_prototype ==================== */

struct Dfunction_prototype : Dfunction
{
    Dfunction_prototype(ThreadContext *tc);

    void *Call(CALL_ARGS);
};

Dfunction_prototype::Dfunction_prototype(ThreadContext *tc)
    : Dfunction(0, tc->Dobject_prototype)
{
    unsigned attributes = DontEnum;

    classname = TEXT_Function;
    name = "prototype";
    Put(TEXT_constructor, tc->Dfunction_constructor, attributes);
    Put(TEXT_toString, new(this) Dfunction_prototype_toString(this), attributes);
    Put(TEXT_apply,    new(this) Dfunction_prototype_apply(this), attributes);
    Put(TEXT_call,     new(this) Dfunction_prototype_call(this), attributes);
}

void *Dfunction_prototype::Call(CALL_ARGS)
{
    // ECMA v3 15.3.4
    // Accept any arguments and return "undefined"
    Value::copy(ret, &vundefined);
    return NULL;
}

/* ===================== Dfunction ==================== */

Dfunction::Dfunction(d_uint32 length)
    : Dobject(Dfunction::getPrototype())
{
    classname = TEXT_Function;
    name = "Function";
    scopex = NULL;
    scopex_dim = 0;
    Put(TEXT_length, length, DontDelete | DontEnum | ReadOnly);
    Put(TEXT_arity, length, DontDelete | DontEnum | ReadOnly);
}

Dfunction::Dfunction(d_uint32 length, Dobject *prototype)
    : Dobject(prototype)
{
    classname = TEXT_Function;
    name = "Function";
    scopex = NULL;
    scopex_dim = 0;
    Put(TEXT_length, length, DontDelete | DontEnum | ReadOnly);
    Put(TEXT_arity, length, DontDelete | DontEnum | ReadOnly);
}

d_string Dfunction::getTypeof()
{   // ECMA 11.4.3
    return TEXT_function;
}

d_string Dfunction::toString()
{
    // Native overrides of this function replace Identifier with the actual name.
    // Don't need to do parameter list, though.
    OutBuffer buf;
    d_string s;

    buf.printf(DTEXT("function %s() { [native code] }"), ascii2unicode(name));
    s = Lstring::ctor((dchar *) buf.data, buf.offset / sizeof(dchar));
    buf.data = NULL;		// buf doesn't own it anymore
    return s;
}

void *Dfunction::HasInstance(Value *ret, Value *v)
{
    // ECMA v3 15.3.5.3
    Dobject *V;
    Value *w;
    Dobject *o;

    if (v->isPrimitive())
	goto Lfalse;
    V = v->toObject();
    w = Get(TEXT_prototype);
    if (w->isPrimitive())
    {	ErrInfo errinfo;
	return RuntimeError(&errinfo, ERR_MUST_BE_OBJECT, w->getType());
    }
    o = w->toObject();
    for (;;)
    {
	V = V->internal_prototype;
	if (!V)
	    goto Lfalse;
	if (o == V)
	    goto Ltrue;
    }

Ltrue:
    Vboolean::putValue(ret, TRUE);
    return NULL;

Lfalse:
    Vboolean::putValue(ret, FALSE);
    return NULL;
}

Dfunction *Dfunction::isFunction(Value *v)
{
    Dfunction *r;
    Dobject *o;

    r = NULL;
    if (!v->isPrimitive())
    {
	o = v->toObject();
	if (o->isClass(TEXT_Function))
	    r = (Dfunction *)o;
    }
    return r;
}

#if 0

void *Dfunction::Construct(CONSTRUCT_ARGS)
{
    // ECMA 3 13.2.2
    Dobject *o;
    Dobject *proto;
    void *v;
    Value *p;

    p = Get(TEXT_prototype);
PRINTF("internal_prototype = %p\n", internal_prototype);
WPRINTF(L"toString = '%ls'\n", toString());
#if 1
    assert(p);
    if (!p || p->isPrimitive())
#else
    if (p->isPrimitive)
#endif
	proto = Dobject::getPrototype();
    else
	proto = p->toObject();
    o = new Dobject(proto);

    v = Call(cc, o, ret, argc, arglist);
    if (!v)
    {
	if (ret->isPrimitive())
	    Vobject::putValue(ret, o);
    }
    return NULL;
}

#endif


Dfunction *Dfunction::getConstructor()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Dfunction_constructor;
}

Dobject *Dfunction::getPrototype()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Dfunction_prototype;
}

void Dfunction::init(ThreadContext *tc)
{
    GC_LOG();
    tc->Dfunction_constructor = new(&tc->mem) Dfunction_constructor(tc);
    GC_LOG();
    tc->Dfunction_prototype = new(&tc->mem) Dfunction_prototype(tc);

    tc->Dfunction_constructor->Put(TEXT_prototype, tc->Dfunction_prototype, DontEnum | DontDelete | ReadOnly);

    tc->Dfunction_constructor->internal_prototype = tc->Dfunction_prototype;
    tc->Dfunction_constructor->proptable.previous = &tc->Dfunction_prototype->proptable;
}


