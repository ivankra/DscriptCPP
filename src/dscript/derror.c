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

#include "port.h"
#include "root.h"
#include "dscript.h"
#include "value.h"
#include "dobject.h"
#include "text.h"

// Comes from MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0)
#define FACILITY	0x800A0000

/* ===================== Derror_constructor ==================== */

struct Derror_constructor : Dfunction
{
    Derror_constructor(ThreadContext *tc);

    void *Construct(CONSTRUCT_ARGS);
    void *Call(CALL_ARGS);
};

Derror_constructor::Derror_constructor(ThreadContext *tc)
    : Dfunction(1, tc->Dfunction_prototype)
{
}

void *Derror_constructor::Call(CALL_ARGS)
{
    // ECMA v3 15.11.1
    return Construct(cc, ret, argc, arglist);
}

void *Derror_constructor::Construct(CONSTRUCT_ARGS)
{
    // ECMA 15.7.2
    Dobject *o;
    Value *m;
    Value *n;
    Vstring vemptystring(TEXT_);

    switch (argc)
    {
	case 0:	// ECMA doesn't say what we do if m is undefined
		m = &vemptystring;
		n = &vundefined;
		break;
	case 1:
		m = &arglist[0];
		if (m->isNumber())
		{
		    n = m;
		    m = &vemptystring;
		}
		else
		    n = &vundefined;
		break;
	default:
		m = &arglist[0];
		n = &arglist[1];
		break;
    }
    o = new(this) Derror(m, n);
    Vobject::putValue(ret, o);
    return NULL;
}

/* ===================== Derror_prototype_toString =============== */

BUILTIN_FUNCTION(Derror_prototype_, toString, 0)
{
    // ECMA v3 15.11.4.3
    // Return implementation defined string
    Value *v;

    //PRINTF("Error.prototype.toString()\n");
    v = othis->Get(TEXT_message);
    if (!v)
	v = &vundefined;
    Vstring::putValue(ret, v->toString());
    return NULL;
}

/* ===================== Derror_prototype ==================== */

struct Derror_prototype : Derror
{
    Derror_prototype(ThreadContext *tc);
};

Derror_prototype::Derror_prototype(ThreadContext *tc)
    : Derror(tc->Dobject_prototype)
{
    Dobject *f = tc->Dfunction_prototype;
    //d_string m = d_string_ctor(DTEXT("Error.prototype.message"));

    Put(TEXT_constructor, tc->Derror_constructor, DontEnum);
    Put(TEXT_toString, new(this) Derror_prototype_toString(f), 0);
    Put(TEXT_name, TEXT_Error, 0);
    Put(TEXT_message, TEXT_, 0);
    Put(TEXT_description, TEXT_, 0);
    Put(TEXT_number, (d_number)(/*FACILITY |*/ 0), 0);
}

/* ===================== Derror ==================== */

Derror::Derror(Value *m, Value *v2)
    : Dobject(Derror::getPrototype())
{
    d_string msg;

    classname = TEXT_Error;
    msg = m->toString();
    Put(TEXT_message, msg, 0);
    Put(TEXT_description, msg, 0);
    if (m->isString())
    {
    }
    else if (m->isNumber())
    {	d_number n = m->toNumber();
	n = (d_number)(/*FACILITY |*/ (int)n);
	Put(TEXT_number, n, 0);
    }
    if (v2->isString())
    {
	Put(TEXT_description, v2->toString(), 0);
	Put(TEXT_message, v2->toString(), 0);
    }
    else if (v2->isNumber())
    {	d_number n = v2->toNumber();
	n = (d_number)(/*FACILITY |*/ (int)n);
	Put(TEXT_number, n, 0);
    }
}

Derror::Derror(Dobject *prototype)
    : Dobject(prototype)
{
    classname = TEXT_Error;
}

Dfunction *Derror::getConstructor()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Derror_constructor;
}

Dobject *Derror::getPrototype()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Derror_prototype;
}

void Derror::init(ThreadContext *tc)
{
    GC_LOG();
    tc->Derror_constructor = new(&tc->mem) Derror_constructor(tc);
    GC_LOG();
    tc->Derror_prototype = new(&tc->mem) Derror_prototype(tc);

    tc->Derror_constructor->Put(TEXT_prototype, tc->Derror_prototype, DontEnum | DontDelete | ReadOnly);
}


