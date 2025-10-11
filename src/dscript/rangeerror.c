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

// NOTE: This file is also used as a prototype for the other
// NativeError objects, using sed to generate the others from
// this file.

// Comes from MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, 0)
#define FACILITY	((int)0x800A0000)

/* ===================== Drangeerror_constructor ==================== */

struct Drangeerror_constructor : Dfunction
{
    Drangeerror_constructor(ThreadContext *tc);

    void *Construct(CONSTRUCT_ARGS);
    void *Call(CALL_ARGS);
};

Drangeerror_constructor::Drangeerror_constructor(ThreadContext *tc)
    : Dfunction(1, tc->Dfunction_prototype)
{
}

void *Drangeerror_constructor::Call(CALL_ARGS)
{
    // ECMA v3 15.11.7.1
    return Construct(cc, ret, argc, arglist);
}

void *Drangeerror_constructor::Construct(CONSTRUCT_ARGS)
{
    // ECMA 15.11.7.2
    Value *m;
    Dobject *o;
    dchar *s;

    m = (argc) ? &arglist[0] : &vundefined;
    // ECMA doesn't say what we do if m is undefined
    if (m->isUndefined())
	s = DTEXT("RangeError");
    else
	s = d_string_ptr(m->toString());
    o = new(this) Drangeerror(s);
    Vobject::putValue(ret, o);
    return NULL;
}

/* ===================== Drangeerror_prototype ==================== */

struct Drangeerror_prototype : Drangeerror
{
    Drangeerror_prototype(ThreadContext *tc);
};

Drangeerror_prototype::Drangeerror_prototype(ThreadContext *tc)
    : Drangeerror(tc->Derror_prototype)
{   Lstring *s;

    Put(TEXT_constructor, tc->Drangeerror_constructor, DontEnum);
    Put(TEXT_name, TEXT_RangeError, 0);
    s = d_string_ctor(DTEXT("RangeError.prototype.message"));
    Put(TEXT_message, s, 0);
    Put(TEXT_description, s, 0);
    Put(TEXT_number, (d_number)(FACILITY | 1002), 0);
}

/* ===================== Drangeerror ==================== */

Drangeerror::Drangeerror(dchar *m)
    : Dobject(Drangeerror::getPrototype())
{   Lstring *s;

    classname = TEXT_Error;
    s = d_string_ctor(m);
    Put(TEXT_message, s, 0);
    Put(TEXT_description, s, 0);
    Put(TEXT_number, (d_number)(FACILITY | 1002), 0);
    errinfo.message = m;
}

Drangeerror::Drangeerror(ErrInfo *perrinfo)
    : Dobject(Drangeerror::getPrototype())
{   Lstring *s;
    int code;

    classname = TEXT_Error;
    errinfo = *perrinfo;
    s = d_string_ctor(perrinfo->message);
    Put(TEXT_message, s, 0);
    Put(TEXT_description, s, 0);
    code = perrinfo->code;
    if ((code & 0xFFFF0000) == 0)
	code |= FACILITY;
    Put(TEXT_number, (d_number)code, 0);
}

Drangeerror::Drangeerror(Dobject *prototype)
    : Dobject(prototype)
{
    classname = TEXT_Error;
}

void Drangeerror::getErrInfo(ErrInfo *perrinfo, int linnum)
{
    if (linnum && errinfo.linnum == 0)
	errinfo.linnum = linnum;
    if (perrinfo)
	*perrinfo = errinfo;
    //WPRINTF(L"getErrInfo(linnum = %d), errinfo.linnum = %d\n", linnum, errinfo.linnum);
}

Dfunction *Drangeerror::getConstructor()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Drangeerror_constructor;
}

Dobject *Drangeerror::getPrototype()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Drangeerror_prototype;
}

void Drangeerror::init(ThreadContext *tc)
{
    GC_LOG();
    tc->Drangeerror_constructor = new(&tc->mem) Drangeerror_constructor(tc);
    GC_LOG();
    tc->Drangeerror_prototype = new(&tc->mem) Drangeerror_prototype(tc);

    tc->Drangeerror_constructor->Put(TEXT_prototype, tc->Drangeerror_prototype, DontEnum | DontDelete | ReadOnly);
}
