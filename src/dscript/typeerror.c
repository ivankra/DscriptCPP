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

/* ===================== Dtypeerror_constructor ==================== */

struct Dtypeerror_constructor : Dfunction
{
    Dtypeerror_constructor(ThreadContext *tc);

    void *Construct(CONSTRUCT_ARGS);
    void *Call(CALL_ARGS);
};

Dtypeerror_constructor::Dtypeerror_constructor(ThreadContext *tc)
    : Dfunction(1, tc->Dfunction_prototype)
{
}

void *Dtypeerror_constructor::Call(CALL_ARGS)
{
    // ECMA v3 15.11.7.1
    return Construct(cc, ret, argc, arglist);
}

void *Dtypeerror_constructor::Construct(CONSTRUCT_ARGS)
{
    // ECMA 15.11.7.2
    Value *m;
    Dobject *o;
    dchar *s;

    m = (argc) ? &arglist[0] : &vundefined;
    // ECMA doesn't say what we do if m is undefined
    if (m->isUndefined())
	s = DTEXT("TypeError");
    else
	s = d_string_ptr(m->toString());
    o = new(this) Dtypeerror(s);
    Vobject::putValue(ret, o);
    return NULL;
}

/* ===================== Dtypeerror_prototype ==================== */

struct Dtypeerror_prototype : Dtypeerror
{
    Dtypeerror_prototype(ThreadContext *tc);
};

Dtypeerror_prototype::Dtypeerror_prototype(ThreadContext *tc)
    : Dtypeerror(tc->Derror_prototype)
{   Lstring *s;

    Put(TEXT_constructor, tc->Dtypeerror_constructor, DontEnum);
    Put(TEXT_name, TEXT_TypeError, 0);
    s = d_string_ctor(DTEXT("TypeError.prototype.message"));
    Put(TEXT_message, s, 0);
    Put(TEXT_description, s, 0);
    Put(TEXT_number, (d_number)(FACILITY | 1002), 0);
}

/* ===================== Dtypeerror ==================== */

Dtypeerror::Dtypeerror(dchar *m)
    : Dobject(Dtypeerror::getPrototype())
{   Lstring *s;

    classname = TEXT_Error;
    s = d_string_ctor(m);
    Put(TEXT_message, s, 0);
    Put(TEXT_description, s, 0);
    Put(TEXT_number, (d_number)(FACILITY | 1002), 0);
    errinfo.message = m;
}

Dtypeerror::Dtypeerror(ErrInfo *perrinfo)
    : Dobject(Dtypeerror::getPrototype())
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

Dtypeerror::Dtypeerror(Dobject *prototype)
    : Dobject(prototype)
{
    classname = TEXT_Error;
}

void Dtypeerror::getErrInfo(ErrInfo *perrinfo, int linnum)
{
    if (linnum && errinfo.linnum == 0)
	errinfo.linnum = linnum;
    if (perrinfo)
	*perrinfo = errinfo;
    //WPRINTF(L"getErrInfo(linnum = %d), errinfo.linnum = %d\n", linnum, errinfo.linnum);
}

Dfunction *Dtypeerror::getConstructor()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Dtypeerror_constructor;
}

Dobject *Dtypeerror::getPrototype()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Dtypeerror_prototype;
}

void Dtypeerror::init(ThreadContext *tc)
{
    GC_LOG();
    tc->Dtypeerror_constructor = new(&tc->mem) Dtypeerror_constructor(tc);
    GC_LOG();
    tc->Dtypeerror_prototype = new(&tc->mem) Dtypeerror_prototype(tc);

    tc->Dtypeerror_constructor->Put(TEXT_prototype, tc->Dtypeerror_prototype, DontEnum | DontDelete | ReadOnly);
}
