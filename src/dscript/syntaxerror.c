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

/* ===================== Dsyntaxerror_constructor ==================== */

struct Dsyntaxerror_constructor : Dfunction
{
    Dsyntaxerror_constructor(ThreadContext *tc);

    void *Construct(CONSTRUCT_ARGS);
    void *Call(CALL_ARGS);
};

Dsyntaxerror_constructor::Dsyntaxerror_constructor(ThreadContext *tc)
    : Dfunction(1, tc->Dfunction_prototype)
{
}

void *Dsyntaxerror_constructor::Call(CALL_ARGS)
{
    // ECMA v3 15.11.7.1
    return Construct(cc, ret, argc, arglist);
}

void *Dsyntaxerror_constructor::Construct(CONSTRUCT_ARGS)
{
    // ECMA 15.11.7.2
    Value *m;
    Dobject *o;
    dchar *s;

    m = (argc) ? &arglist[0] : &vundefined;
    // ECMA doesn't say what we do if m is undefined
    if (m->isUndefined())
	s = DTEXT("SyntaxError");
    else
	s = d_string_ptr(m->toString());
    o = new(this) Dsyntaxerror(s);
    Vobject::putValue(ret, o);
    return NULL;
}

/* ===================== Dsyntaxerror_prototype ==================== */

struct Dsyntaxerror_prototype : Dsyntaxerror
{
    Dsyntaxerror_prototype(ThreadContext *tc);
};

Dsyntaxerror_prototype::Dsyntaxerror_prototype(ThreadContext *tc)
    : Dsyntaxerror(tc->Derror_prototype)
{   Lstring *s;

    Put(TEXT_constructor, tc->Dsyntaxerror_constructor, DontEnum);
    Put(TEXT_name, TEXT_SyntaxError, 0);
    s = d_string_ctor(DTEXT("SyntaxError.prototype.message"));
    Put(TEXT_message, s, 0);
    Put(TEXT_description, s, 0);
    Put(TEXT_number, (d_number)(FACILITY | 1002), 0);
}

/* ===================== Dsyntaxerror ==================== */

Dsyntaxerror::Dsyntaxerror(dchar *m)
    : Dobject(Dsyntaxerror::getPrototype())
{   Lstring *s;

    classname = TEXT_Error;
    s = d_string_ctor(m);
    Put(TEXT_message, s, 0);
    Put(TEXT_description, s, 0);
    Put(TEXT_number, (d_number)(FACILITY | 1002), 0);
    errinfo.message = m;
}

Dsyntaxerror::Dsyntaxerror(ErrInfo *perrinfo)
    : Dobject(Dsyntaxerror::getPrototype())
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

Dsyntaxerror::Dsyntaxerror(Dobject *prototype)
    : Dobject(prototype)
{
    classname = TEXT_Error;
}

void Dsyntaxerror::getErrInfo(ErrInfo *perrinfo, int linnum)
{
    if (linnum && errinfo.linnum == 0)
	errinfo.linnum = linnum;
    if (perrinfo)
	*perrinfo = errinfo;
    //WPRINTF(L"getErrInfo(linnum = %d), errinfo.linnum = %d\n", linnum, errinfo.linnum);
}

Dfunction *Dsyntaxerror::getConstructor()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Dsyntaxerror_constructor;
}

Dobject *Dsyntaxerror::getPrototype()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Dsyntaxerror_prototype;
}

void Dsyntaxerror::init(ThreadContext *tc)
{
    GC_LOG();
    tc->Dsyntaxerror_constructor = new(&tc->mem) Dsyntaxerror_constructor(tc);
    GC_LOG();
    tc->Dsyntaxerror_prototype = new(&tc->mem) Dsyntaxerror_prototype(tc);

    tc->Dsyntaxerror_constructor->Put(TEXT_prototype, tc->Dsyntaxerror_prototype, DontEnum | DontDelete | ReadOnly);
}
