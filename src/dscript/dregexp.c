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
#include <ctype.h>
#include <assert.h>

#include "port.h"
#include "root.h"
#include "dscript.h"
#include "value.h"
#include "dobject.h"
#include "dregexp.h"
#include "dchar.h"
#include "regexp.h"
#include "text.h"

/* ===================== Dregexp_constructor ==================== */

struct Dregexp_constructor : Dfunction
{
    Dregexp_constructor(ThreadContext *tc);

    Value *Get(d_string PropertyName);
    Value *Put(d_string PropertyName, Value *value, unsigned attributes);
    Value *Put(d_string PropertyName, Dobject *o, unsigned attributes);
    Value *Put(d_string PropertyName, d_number n, unsigned attributes);
    int CanPut(d_string PropertyName);
    int HasProperty(d_string PropertyName);
    int Delete(d_string PropertyName);

    void *Construct(CONSTRUCT_ARGS);
    void *Call(CALL_ARGS);

    static d_string alias(d_string s);

    Value *input;
    Value *multiline;
    Value *lastMatch;
    Value *lastParen;
    Value *leftContext;
    Value *rightContext;
    Value *dollar[10];

    // Extensions
    Value *index;
    Value *lastIndex;
};

Dregexp_constructor::Dregexp_constructor(ThreadContext *tc)
    : Dfunction(2, tc->Dfunction_prototype)
{
    Vstring v(TEXT_);
    Vboolean vb(FALSE);
    //Vnumber vn(0);
    Vnumber vnm1(-1);

    name = "RegExp";

    // Static properties
    Put(TEXT_input, &v, DontDelete);
    Put(TEXT_multiline, &vb, DontDelete);
    Put(TEXT_lastMatch, &v, ReadOnly | DontDelete);
    Put(TEXT_lastParen, &v, ReadOnly | DontDelete);
    Put(TEXT_leftContext, &v, ReadOnly | DontDelete);
    Put(TEXT_rightContext, &v, ReadOnly | DontDelete);
    Put(TEXT_dollar1, &v, ReadOnly | DontDelete);
    Put(TEXT_dollar2, &v, ReadOnly | DontDelete);
    Put(TEXT_dollar3, &v, ReadOnly | DontDelete);
    Put(TEXT_dollar4, &v, ReadOnly | DontDelete);
    Put(TEXT_dollar5, &v, ReadOnly | DontDelete);
    Put(TEXT_dollar6, &v, ReadOnly | DontDelete);
    Put(TEXT_dollar7, &v, ReadOnly | DontDelete);
    Put(TEXT_dollar8, &v, ReadOnly | DontDelete);
    Put(TEXT_dollar9, &v, ReadOnly | DontDelete);

    Put(TEXT_index, &vnm1, ReadOnly | DontDelete);
    Put(TEXT_lastIndex, &vnm1, ReadOnly | DontDelete);

    input = Get(TEXT_input);
    multiline = Get(TEXT_multiline);
    lastMatch = Get(TEXT_lastMatch);
    lastParen = Get(TEXT_lastParen);
    leftContext = Get(TEXT_leftContext);
    rightContext = Get(TEXT_rightContext);
    dollar[0] = lastMatch;
    dollar[1] = Get(TEXT_dollar1);
    dollar[2] = Get(TEXT_dollar2);
    dollar[3] = Get(TEXT_dollar3);
    dollar[4] = Get(TEXT_dollar4);
    dollar[5] = Get(TEXT_dollar5);
    dollar[6] = Get(TEXT_dollar6);
    dollar[7] = Get(TEXT_dollar7);
    dollar[8] = Get(TEXT_dollar8);
    dollar[9] = Get(TEXT_dollar9);

    index = Get(TEXT_index);
    lastIndex = Get(TEXT_lastIndex);

    // Should lastMatch be an alias for dollar[nparens],
    // or should it be a separate property?
    // We implemented it the latter way.
    // Since both are ReadOnly, I can't see that it makes
    // any difference.
}

void *Dregexp_constructor::Construct(CONSTRUCT_ARGS)
{
    // ECMA 262 v3 15.10.4.1

    Value *pattern;
    Value *flags;
    d_string P;
    d_string F;
    Dregexp *r;
    Dregexp *R;

    //PRINTF("Dregexp_constructor::Construct()\n");
    Value::copy(ret, &vundefined);
    pattern = &vundefined;
    flags = &vundefined;
    switch (argc)
    {
	case 0:
	    break;

	default:
	    flags = &arglist[1];
	case 1:
	    pattern = &arglist[0];
	    break;
    }
    R = Dregexp::isRegExp(pattern);
    if (R)
    {
	if (flags->isUndefined())
	{
	    P = d_string_ctor(R->re->pattern);
	    F = d_string_ctor(R->re->flags);
	}
	else
	{
	    ErrInfo errinfo;
	    return RuntimeError(&errinfo, ERR_TYPE_ERROR,
		    DTEXT("RegExp.prototype.constructor"));
	}
    }
    else
    {
	P = pattern->isUndefined() ? TEXT_ : pattern->toString();
	F = flags->isUndefined() ? TEXT_ : flags->toString();
    }
    r = new(this) Dregexp(P, F);
    if (r->re->errors)
    {	Dobject *o;
	ErrInfo errinfo;

#if 0
	WPRINTF(L"P = '%s'\nF = '%s'\n", d_string_ptr(P), d_string_ptr(F));
	for (int i = 0; i < d_string_len(P); i++)
	    WPRINTF(L"x%02x\n", d_string_ptr(P)[i]);
#endif
	errinfo.code = 5017;
	errinfo.message = errmsg(ERR_REGEXP_COMPILE);
	o = new(this) Dsyntaxerror(&errinfo);
	return new(this) Vobject(o);
    }
    else
    {
	Vobject::putValue(ret, r);
	return NULL;
    }
}

void *Dregexp_constructor::Call(CALL_ARGS)
{
    // ECMA 262 v3 15.10.3.1
    if (argc >= 1)
    {	Value *pattern;
	Dobject *o;

	pattern = &arglist[0];
	if (!pattern->isPrimitive())
	{
	    o = pattern->object;
	    if (o->isDregexp() &&
		(argc == 1 || arglist[1].isUndefined())
	       )
	    {   Vobject::putValue(ret, o);
		return NULL;
	    }
	}
    }
    return Construct(cc, ret, argc, arglist);
}

// Translate Perl property names to dscript property names
d_string Dregexp_constructor::alias(d_string s)
{
    d_string t;
    dchar *p;

    static dchar from[] = { DTEXT("_*&+`'") };
    static d_string *to[] =
    {
	&TEXT_input,
	&TEXT_multiline,
	&TEXT_lastMatch,
	&TEXT_lastParen,
	&TEXT_leftContext,
	&TEXT_rightContext,
    };

    t = s;
    p = d_string_ptr(s);
    if (p[0] == '$' && p[1] && p[2] == 0)
    {
	p = Dchar::memchr(from, p[1], sizeof(from) / sizeof(dchar));
	if (p)
	    t = *to[p - from];
    }
    return t;
}

Value *Dregexp_constructor::Get(d_string PropertyName)
{
    return Dfunction::Get(alias(PropertyName));
}

Value *Dregexp_constructor::Put(d_string PropertyName, Value *value, unsigned attributes)
{
    return Dfunction::Put(alias(PropertyName), value, attributes);
}

Value *Dregexp_constructor::Put(d_string PropertyName, Dobject *o, unsigned attributes)
{
    return Dfunction::Put(alias(PropertyName), o, attributes);
}

Value *Dregexp_constructor::Put(d_string PropertyName, d_number n, unsigned attributes)
{
    return Dfunction::Put(alias(PropertyName), n, attributes);
}

int Dregexp_constructor::CanPut(d_string PropertyName)
{
    return Dfunction::CanPut(alias(PropertyName));
}

int Dregexp_constructor::HasProperty(d_string PropertyName)
{
    return Dfunction::HasProperty(alias(PropertyName));
}

int Dregexp_constructor::Delete(d_string PropertyName)
{
    return Dfunction::Delete(alias(PropertyName));
}

/* ===================== Dregexp_prototype_toString =============== */

BUILTIN_FUNCTION(Dregexp_prototype_, toString, 0)
{
    // othis must be a RegExp
    Dregexp *r;

    if (!othis->isDregexp())
    {
	Value::copy(ret, &vundefined);
	ErrInfo errinfo;
	return RuntimeError(&errinfo, ERR_NOT_TRANSFERRABLE,
		DTEXT("RegExp.prototype.toString()"));
    }
    else
    {
	OutBuffer buf;
	d_string s;

	r = (Dregexp *)(othis);
	buf.writedchar('/');
	buf.writedstring(r->re->pattern);
	buf.writedchar('/');
	buf.writedstring(r->re->flags);
	buf.writedchar(0);
	s = d_string_ctor((dchar *)buf.data);
	buf.data = NULL;
	Vstring::putValue(ret, s);
    }
    return NULL;
}

/* ===================== Dregexp_prototype_test =============== */

BUILTIN_FUNCTION(Dregexp_prototype_, test, 1)
{
    // ECMA v3 15.10.6.3 says this is equivalent to:
    //	RegExp.prototype.exec(string) != null
    return Dregexp::exec(othis, ret, argc, arglist, EXEC_BOOLEAN);
}

/* ===================== Dregexp_prototype_exec ============= */

BUILTIN_FUNCTION(Dregexp_prototype_, exec, 1)
{
    return Dregexp::exec(othis, ret, argc, arglist, EXEC_ARRAY);
}

void *Dregexp::exec(Dobject *othis, Value *ret, unsigned argc, Value *arglist, int rettype)
{
    //PRINTF("Dregexp::exec(argc = %d, rettype = %d)\n", argc, rettype);

    // othis must be a RegExp
    if (!othis->isClass(TEXT_RegExp))
    {
	Value::copy(ret, &vundefined);
	ErrInfo errinfo;
	return RuntimeError(&errinfo, ERR_NOT_TRANSFERRABLE,
		DTEXT("RegExp.prototype.exec()"));
    }
    else
    {
	d_string s;
	Dregexp *dr;
	RegExp *r;
	Dregexp_constructor *dc;
	unsigned i;
	d_int32 lasti;

	if (argc)
	    s = arglist[0].toString();
	else
	{   Dfunction *df;

	    df = Dregexp::getConstructor();
	    s = ((Dregexp_constructor *)df)->input->x.string;
	}

	dr = (Dregexp *)othis;
	r = dr->re;
	dc = (Dregexp_constructor *)Dregexp::getConstructor();

	// Decide if we are multiline
	if (dr->multiline->boolean)
	    r->attributes |= REAmultiline;
	else
	    r->attributes &= ~REAmultiline;

	if (r->attributes & REAglobal && rettype != EXEC_INDEX)
	    lasti = (int)dr->lastIndex->toInteger();
	else
	    lasti = 0;

	if (r->test(d_string_ptr(s), lasti))
	{   // Successful match
	    Value *lastv;
	    unsigned nmatches;

	    if (r->attributes & REAglobal && rettype != EXEC_INDEX)
	    {
		Vnumber::putValue(dr->lastIndex, r->text.iend);
	    }

	    Vstring::putValue(dc->input, d_string_ctor(r->input));

	    s = Dstring::substring(r->input, r->text.istart, r->text.iend);
	    Vstring::putValue(dc->lastMatch, s);

	    s = Dstring::substring(r->input, 0, r->text.istart);
	    Vstring::putValue(dc->leftContext, s);

	    s = Dstring::substring(r->input, r->text.iend, Dchar::len(r->input));
	    Vstring::putValue(dc->rightContext, s);

	    Vnumber::putValue(dc->index, r->text.istart);
	    Vnumber::putValue(dc->lastIndex, r->text.iend);

	    // Fill in $1..$9
	    lastv = &vundefined;
	    nmatches = 0;
	    for (i = 1; i <= 9; i++)
	    {
		if (i <= r->nparens)
		{   int n;

		    // Use last 9 entries for $1..$9
		    n = i - 1;
		    if (r->nparens > 9)
			n += (r->nparens - 9);

		    if (r->parens[n].istart != -1)
		    {	s = Dstring::substring(r->input, r->parens[n].istart, r->parens[n].iend);
			Vstring::putValue(dc->dollar[i], s);
			nmatches = i;
		    }
		    else
			Value::copy(dc->dollar[i], &vundefined);
		    lastv = dc->dollar[i];
		}
		else
		    Value::copy(dc->dollar[i], &vundefined);
	    }
	    // Last substring in $1..$9, or "" if none
	    if (r->nparens)
		Value::copy(dc->lastParen, lastv);
	    else
		Vstring::putValue(dc->lastParen, TEXT_);

	    switch (rettype)
	    {
		case EXEC_ARRAY:
		{
		    Darray *a = new Darray();

		    a->Put(TEXT_input, d_string_ctor(r->input), 0);
		    a->Put(TEXT_index, r->text.istart, 0);
		    a->Put(TEXT_lastIndex, r->text.iend, 0);

		    a->Put((d_uint32)0, dc->lastMatch, (unsigned)0);

		    // [1]..[nparens]
		    for (i = 1; i <= r->nparens; i++)
		    {
			if (i > nmatches)
			    a->Put(i, TEXT_, 0);

			// Reuse values already put into dc->dollar[]
			else if (r->nparens <= 9)
			    a->Put(i, dc->dollar[i], 0);
			else if (i > r->nparens - 9)
			    a->Put(i, dc->dollar[i - (r->nparens - 9)], 0);
			else if (r->parens[i - 1].istart == -1)
			{
			    a->Put(i, &vundefined, 0);
			}
			else
			{
			    s = Dstring::substring(r->input, r->parens[i - 1].istart, r->parens[i - 1].iend);
			    a->Put(i, s, 0);
			}
		    }
		    Vobject::putValue(ret, a);
		    break;
		}
		case EXEC_STRING:
		    Value::copy(ret, dc->lastMatch);
		    break;

		case EXEC_BOOLEAN:
		    Vboolean::putValue(ret, TRUE);	// success
		    break;

		case EXEC_INDEX:
		    Vnumber::putValue(ret, r->text.istart);
		    break;

		default:
		    assert(0);
	    }
	}
	else	// failed to match
	{
	    //PRINTF("failed\n");
	    switch (rettype)
	    {
		case EXEC_ARRAY:
		    //PRINTF("memcpy\n");
		    Value::copy(ret, &vnull);	    // Return null
		    Vnumber::putValue(dr->lastIndex, 0);
		    break;

		case EXEC_STRING:
		    Vstring::putValue(ret, (Lstring *)NULL);
		    Vnumber::putValue(dr->lastIndex, 0);
		    break;

		case EXEC_BOOLEAN:
		    Vboolean::putValue(ret, FALSE);
		    Vnumber::putValue(dr->lastIndex, 0);
		    break;

		case EXEC_INDEX:
		    Vnumber::putValue(ret, -1.0);
		    // Do not set lastIndex
		    break;

		default:
		    assert(0);
	    }
	}
    }
    return NULL;
}

/* ===================== Dregexp_prototype_compile ============= */

BUILTIN_FUNCTION(Dregexp_prototype_, compile, 2)
{
    // RegExp.prototype.compile(pattern, attributes)

    // othis must be a RegExp
    if (!othis->isClass(TEXT_RegExp))
    {
	ErrInfo errinfo;
	return RuntimeError(&errinfo, ERR_NOT_TRANSFERRABLE,
		DTEXT("RegExp.prototype.compile()"));
    }
    else
    {
	d_string pattern;
	d_string attributes;
	Dregexp *dr;
	RegExp *r;

	dr = (Dregexp *)othis;
	pattern = TEXT_;
	attributes = TEXT_;
	switch (argc)
	{
	    case 0:
		break;

	    default:
		attributes = arglist[1].toString();
	    case 1:
		pattern = arglist[0].toString();
		break;
	}

	r = dr->re;
	if (!r->compile(d_string_ptr(pattern), d_string_ptr(attributes), 1))
	{
	    // Affect source, global and ignoreCase properties
	    Vstring::putValue(dr->source, d_string_ctor(r->pattern));
	    Vboolean::putValue(dr->global, (r->attributes & REAglobal) != 0);
	    Vboolean::putValue(dr->ignoreCase, (r->attributes & REAignoreCase) != 0);
	}
	//PRINTF("r->attributes = x%x\n", r->attributes);
    }
    // Documentation says nothing about a return value,
    // so let's use "undefined"
    Value::copy(ret, &vundefined);
    return NULL;
}

/* ===================== Dregexp_prototype ==================== */

struct Dregexp_prototype : Dregexp
{
    Dregexp_prototype(ThreadContext *tc);
};

Dregexp_prototype::Dregexp_prototype(ThreadContext *tc)
    : Dregexp(tc->Dobject_prototype)
{
    classname = TEXT_Object;
    unsigned attributes = ReadOnly | DontDelete | DontEnum;
    Dobject *f = tc->Dfunction_prototype;

    Put(TEXT_constructor, tc->Dregexp_constructor, attributes);
    Put(TEXT_toString, new(this) Dregexp_prototype_toString(f), attributes);

    Put(TEXT_compile, new(this) Dregexp_prototype_compile(f), attributes);
    Put(TEXT_exec, new(this) Dregexp_prototype_exec(f), attributes);
    Put(TEXT_test, new(this) Dregexp_prototype_test(f), attributes);
}

/* ===================== Dregexp ==================== */

Dregexp::Dregexp(d_string pattern, d_string attributes)
    : Dobject(Dregexp::getPrototype())
{
    Vstring v(TEXT_);
    Vboolean vb(FALSE);
    classname = TEXT_RegExp;

    //WPRINTF(L"Dregexp::Dregexp(pattern = '%ls', attributes = '%ls')\n", d_string_ptr(pattern), d_string_ptr(attributes));
    Put(TEXT_source, &v, ReadOnly | DontDelete | DontEnum);
    Put(TEXT_global, &vb, ReadOnly | DontDelete | DontEnum);
    Put(TEXT_ignoreCase, &vb, ReadOnly | DontDelete | DontEnum);
    Put(TEXT_multiline, &vb, ReadOnly | DontDelete | DontEnum);
    Put(TEXT_lastIndex, 0.0, DontDelete | DontEnum);

    source = Get(TEXT_source);
    global = Get(TEXT_global);
    ignoreCase = Get(TEXT_ignoreCase);
    multiline = Get(TEXT_multiline);
    lastIndex = Get(TEXT_lastIndex);

    re = new(this) RegExp(d_string_ptr(pattern), d_string_ptr(attributes), 1);
    if (re->errors == 0)
    {
	Vstring::putValue(source, pattern);
	//PRINTF("source = '%s'\n", source->x.string->toDchars());
	Vboolean::putValue(global, (re->attributes & REAglobal) != 0);
	Vboolean::putValue(ignoreCase, (re->attributes & REAignoreCase) != 0);
	Vboolean::putValue(multiline, (re->attributes & REAmultiline) != 0);
    }
    else
    {
	// have caller throw SyntaxError
    }
}

Dregexp::Dregexp(Dobject *prototype)
    : Dobject(prototype)
{
    Vstring v(TEXT_);
    Vboolean vb(FALSE);
    classname = TEXT_RegExp;

    Put(TEXT_source, &v, ReadOnly | DontDelete | DontEnum);
    Put(TEXT_global, &vb, ReadOnly | DontDelete | DontEnum);
    Put(TEXT_ignoreCase, &vb, ReadOnly | DontDelete | DontEnum);
    Put(TEXT_multiline, &vb, ReadOnly | DontDelete | DontEnum);
    Put(TEXT_lastIndex, 0.0, DontDelete | DontEnum);

    source = Get(TEXT_source);
    global = Get(TEXT_global);
    ignoreCase = Get(TEXT_ignoreCase);
    multiline = Get(TEXT_multiline);
    lastIndex = Get(TEXT_lastIndex);

    re = new RegExp(DTEXT(""), DTEXT(""), 1);
}

void *Dregexp::Call(CALL_ARGS)
{
    // This is the same as calling RegExp.prototype.exec(str)
    Value *v;

    v = Get(TEXT_exec);
    return v->toObject()->Call(cc, this, ret, argc, arglist);
}

Dregexp *Dregexp::isRegExp(Value *v)
{
    Dregexp *r;

    r = NULL;
    if (!v->isPrimitive() && v->toObject()->isDregexp())
    {
	r = (Dregexp *)(v->toObject());
    }
    return r;
}


Dfunction *Dregexp::getConstructor()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Dregexp_constructor;
}

Dobject *Dregexp::getPrototype()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Dregexp_prototype;
}

void Dregexp::init(ThreadContext *tc)
{
    GC_LOG();
    tc->Dregexp_constructor = new(&tc->mem) Dregexp_constructor(tc);
    GC_LOG();
    tc->Dregexp_prototype = new(&tc->mem) Dregexp_prototype(tc);

#if 0
WPRINTF(L"tc->Dregexp_constructor = %x\n", tc->Dregexp_constructor);
unsigned *p;
p = (unsigned *)tc->Dregexp_constructor;
WPRINTF(L"p = %x\n", p);
if (p)
    WPRINTF(L"*p = %x, %x, %x, %x\n", p[0], p[1], p[2], p[3]);
#endif

    tc->Dregexp_constructor->Put(TEXT_prototype, tc->Dregexp_prototype, DontEnum | DontDelete | ReadOnly);
}

