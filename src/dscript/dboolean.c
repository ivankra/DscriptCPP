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

#include "root.h"
#include "dscript.h"
#include "value.h"
#include "dobject.h"
#include "text.h"


/* ===================== Dboolean_constructor ==================== */

struct Dboolean_constructor : Dfunction
{
    Dboolean_constructor(ThreadContext *tc);

    void *Construct(CONSTRUCT_ARGS);
    void *Call(CALL_ARGS);
};

Dboolean_constructor::Dboolean_constructor(ThreadContext *tc)
    : Dfunction(1, tc->Dfunction_prototype)
{
    name = "Boolean";
}

void *Dboolean_constructor::Call(CALL_ARGS)
{
    // ECMA 15.6.1
    d_boolean b;

    b = (argc) ? arglist[0].toBoolean() : FALSE;
    Vboolean::putValue(ret, b);
    return NULL;
}

void *Dboolean_constructor::Construct(CONSTRUCT_ARGS)
{
    // ECMA 15.6.2
    d_boolean b;
    Dobject *o;

    b = (argc) ? arglist[0].toBoolean() : FALSE;
    o = new(this) Dboolean(b);
    Vobject::putValue(ret, o);
    return NULL;
}

/* ===================== Dboolean_prototype_toString =============== */

BUILTIN_FUNCTION(Dboolean_prototype_, toString, 0)
{
    d_string s;

    // othis must be a Boolean
    if (!othis->isClass(TEXT_Boolean))
    {
	ErrInfo errinfo;

	Value::copy(ret, &vundefined);
	errinfo.code = 5010;
	return RuntimeError(&errinfo, ERR_FUNCTION_WANTS_BOOL,
		DTEXT("Boolean.prototype"), DTEXT("toString()"),
		d_string_ptr(othis->classname));
    }
    else
    {	Value *v;

	v = &((Dboolean *)othis)->value;
	s = v->toString();
	Vstring::putValue(ret, s);
    }
    return NULL;
}

/* ===================== Dboolean_prototype_valueOf =============== */

BUILTIN_FUNCTION(Dboolean_prototype_, valueOf, 0)
{
    //FuncLog f(L"Boolean.prototype.valueOf()");
    //logflag = 1;

    // othis must be a Boolean
    if (!othis->isClass(TEXT_Boolean))
    {
	ErrInfo errinfo;

	Value::copy(ret, &vundefined);
	errinfo.code = 5010;
	return RuntimeError(&errinfo, ERR_FUNCTION_WANTS_BOOL,
		DTEXT("Boolean.prototype"), DTEXT("valueOf()"),
		d_string_ptr(othis->classname));
    }
    else
    {	Value *v;

	v = &((Dboolean *)othis)->value;
	Value::copy(ret, v);
    }
    return NULL;
}

/* ===================== Dboolean_prototype ==================== */

struct Dboolean_prototype : Dboolean
{
    Dboolean_prototype(ThreadContext *tc);
};

Dboolean_prototype::Dboolean_prototype(ThreadContext *tc)
    : Dboolean(tc->Dobject_prototype)
{
    Dobject *f = tc->Dfunction_prototype;

    Put(TEXT_constructor, tc->Dboolean_constructor, DontEnum);
    Put(TEXT_toString, new(this) Dboolean_prototype_toString(f), DontEnum);
    Put(TEXT_valueOf,  new(this) Dboolean_prototype_valueOf(f),  DontEnum);
}

/* ===================== Dboolean ==================== */

Dboolean::Dboolean(d_boolean b)
    : Dobject(Dboolean::getPrototype())
{
    Vboolean::putValue(&value, b);
    classname = TEXT_Boolean;
}

Dboolean::Dboolean(Dobject *prototype)
    : Dobject(prototype)
{
    Vboolean::putValue(&value, FALSE);
    classname = TEXT_Boolean;
}

Dfunction *Dboolean::getConstructor()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Dboolean_constructor;
}

Dobject *Dboolean::getPrototype()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Dboolean_prototype;
}

void Dboolean::init(ThreadContext *tc)
{
    GC_LOG();
    tc->Dboolean_constructor = new(&tc->mem) Dboolean_constructor(tc);
    GC_LOG();
    tc->Dboolean_prototype = new(&tc->mem) Dboolean_prototype(tc);

    tc->Dboolean_constructor->Put(TEXT_prototype, tc->Dboolean_prototype, DontEnum | DontDelete | ReadOnly);
}


