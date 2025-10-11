/* Digital Mars DMDScript source code.
 * Copyright (c) 2000-2002 by Chromium Communications
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

#include "port.h"
#include "root.h"
#include "dchar.h"
#include "regexp.h"
#include "dscript.h"
#include "value.h"
#include "dobject.h"
#include "dregexp.h"
#include "text.h"

/* ===================== Dstring_fromCharCode ==================== */

BUILTIN_FUNCTION(Dstring_, fromCharCode, 1)
{
    // ECMA 15.5.3.2
    d_string s;
    dchar *p;
    unsigned i;

    s = Dstring::alloc(argc);
    p = d_string_ptr(s);
    for (i = 0; i < argc; i++)
    {	Value *v;

	v = &arglist[i];
	p[i] = v->toUint16();
    }
    Vstring::putValue(ret, s);
    return NULL;
}

/* ===================== Dstring_constructor ==================== */

struct Dstring_constructor : Dfunction
{
    Dstring_constructor(ThreadContext *tc);

    void *Construct(CONSTRUCT_ARGS);
    void *Call(CALL_ARGS);
};

Dstring_constructor::Dstring_constructor(ThreadContext *tc)
    : Dfunction(1, tc->Dfunction_prototype)
{
    name = "String";
    Put(TEXT_fromCharCode, new(this) Dstring_fromCharCode(tc->Dfunction_prototype) , 0);
}

void *Dstring_constructor::Call(CALL_ARGS)
{
    // ECMA 15.5.1
    d_string s;

    s = (argc) ? arglist[0].toString() : TEXT_;
    Vstring::putValue(ret, s);
    return NULL;
}

void *Dstring_constructor::Construct(CONSTRUCT_ARGS)
{
    // ECMA 15.5.2
    d_string s;
    Dobject *o;

    s = (argc) ? arglist[0].toString() : TEXT_;
    o = new(cc) Dstring(s);
    Vobject::putValue(ret, o);
    return NULL;
}

/* ===================== Dstring_prototype_toString =============== */

BUILTIN_FUNCTION2(Dstring_prototype_, toString, 0)
{
    //PRINTF("Dstring.prototype.toString()\n");
    // othis must be a String
    if (!othis->isClass(TEXT_String))
    {
	ErrInfo errinfo;

	Value::copy(ret, &vundefined);
	errinfo.code = 5005;
	return pthis->RuntimeError(&errinfo,
		ERR_FUNCTION_WANTS_STRING,
		DTEXT("String.prototype"), DTEXT("toString()"),
		d_string_ptr(othis->classname));
    }
    else
    {	Value *v;

	v = &((Dstring *)othis)->value;
	Value::copy(ret, v);
    }
    return NULL;
}

/* ===================== Dstring_prototype_valueOf =============== */

BUILTIN_FUNCTION2(Dstring_prototype_, valueOf, 0)
{
    // Does same thing as String.prototype.toString()

    //WPRINTF(L"string.prototype.valueOf()\n");
    // othis must be a String
    if (!othis->isClass(TEXT_String))
    {
	ErrInfo errinfo;

	Value::copy(ret, &vundefined);
	errinfo.code = 5005;
	return pthis->RuntimeError(&errinfo,
		ERR_FUNCTION_WANTS_STRING,
		DTEXT("String.prototype"), DTEXT("valueOf()"),
		d_string_ptr(othis->classname));
    }
    else
    {	Value *v;

	v = &((Dstring *)othis)->value;
	Value::copy(ret, v);
    }
    return NULL;
}

/* ===================== Dstring_prototype_charAt =============== */

BUILTIN_FUNCTION2(Dstring_prototype_, charAt, 1)
{
    // ECMA 15.5.4.4

    Value *v;
    int pos;		// ECMA says pos should be a d_number,
			// but int should behave the same
    d_string s;
    unsigned len;
    d_string result;

    v = &othis->value;
    s = v->toString();
    v = argc ? &arglist[0] : &vundefined;
    pos = (int) v->toInteger();
    len = d_string_len(s);
    if (pos < 0 || len <= (unsigned) pos)
	result = TEXT_;
    else
    {	dchar *p;

	result = Dstring::alloc(1);
	p = d_string_ptr(result);
	p[0] = d_string_ptr(s)[pos];
    }

    Vstring::putValue(ret, result);
    return NULL;
}

/* ===================== Dstring_prototype_charCodeAt ============= */

BUILTIN_FUNCTION2(Dstring_prototype_, charCodeAt, 1)
{
    // ECMA 15.5.4.5

    Value *v;
    int pos;		// ECMA says pos should be a d_number,
			// but int should behave the same
    d_string s;
    unsigned len;
    d_number result;

    v = &othis->value;
    s = v->toString();
    v = argc ? &arglist[0] : &vundefined;
    pos = (int) v->toInteger();
    len = d_string_len(s);
    if (pos < 0 || len <= (unsigned) pos)
	result = Port::nan;
    else
	result = d_string_ptr(s)[pos];

    Vnumber::putValue(ret, result);
    return NULL;
}

/* ===================== Dstring_prototype_concat ============= */

BUILTIN_FUNCTION2(Dstring_prototype_, concat, 1)
{
    // ECMA v3 15.5.4.6
    d_string s;
    unsigned a;
    unsigned len;
    d_string *array;	// array of strings
    int *alen;		// array of lengths
    Value *v;

    //PRINTF("Dstring.prototype.concat()\n");
    array = (d_string *) alloca((1 + argc) * sizeof(d_string));
    assert(array);
    alen = (int *) alloca((1 + argc) * sizeof(int));
    assert(alen);

    // Fill array
    v = &othis->value;
    array[0] = v->toString();
    for (a = 0; a < argc; a++)
    {
	v = &arglist[a];
	array[a + 1] = v->toString();
    }

#if 0
    for (a = 0; a <= argc; a++)
    {
	WPRINTF(L"array[%d] = '%ls'\n", a, d_string_ptr(array[a]));
    }
#endif

    // Compute length of strings & total length
    len = 0;
    for (a = 0; a <= argc; a++)
    {	alen[a] = d_string_len(array[a]);
	len += alen[a];
    }

    // Allocate & fill
    s = Dstring::alloc(len);
    dchar *p = d_string_ptr(s);
    for (a = 0; a <= argc; a++)
    {	unsigned lenx = alen[a];

	::memcpy(p, d_string_ptr(array[a]), lenx * sizeof(dchar));
	p += lenx;
    }

    Vstring::putValue(ret, s);
    return NULL;
}

/* ===================== Dstring_prototype_indexOf ============= */

BUILTIN_FUNCTION2(Dstring_prototype_, indexOf, 1)
{
    // ECMA 15.5.4.6
    // String.prototype.indexOf(searchString, position)

    Value *v1;
    Value *v2;
    int pos;		// ECMA says pos should be a d_number,
			// but I can't find a reason.
    d_string s;
    int len;
    dchar *p;
    d_string searchString;
    int sslen;
    dchar *ssptr;
    int k;
    int kmax;

#if 1
    Vobject xx(othis);
    s = xx.toString();
#else
    Value *v;
    v = &othis->value;
    s = v->toString();
#endif
    v1 = argc ? &arglist[0] : &vundefined;
    v2 = (argc >= 2) ? &arglist[1] : &vundefined;

    searchString = v1->toString();
    pos = (int) v2->toInteger();
    len = d_string_len(s);
    p = d_string_ptr(s);

    if (pos < 0)
	pos = 0;
    else if (pos > len)
	pos = len;
    sslen = d_string_len(searchString);
    ssptr = d_string_ptr(searchString);

    if (sslen == 0)
	k = pos;
    else
    {
	dchar c = ssptr[0];

	kmax = len - sslen;
	if (sslen == 1)
	{
	    for (k = pos; ; k++)
	    {
		if (k > kmax)
		{
		    k = -1;
		    break;
		}
		if (p[k] == c)
		    break;
	    }
	}
	else
	{
	    ssptr++;
	    sslen--;
	    for (k = pos; ; k++)
	    {
		if (k > kmax)
		{
		    k = -1;
		    break;
		}
		if (p[k] == c && memcmp(p + k + 1, ssptr, sslen * sizeof(dchar)) == 0)
		    break;
	    }
	}
    }

    Vnumber::putValue(ret, k);
    return NULL;
}

/* ===================== Dstring_prototype_lastIndexOf ============= */

BUILTIN_FUNCTION2(Dstring_prototype_, lastIndexOf, 1)
{
    // ECMA v3 15.5.4.8
    // String.prototype.lastIndexOf(searchString, position)

    Value *v1;
    int pos;		// ECMA says pos should be a d_number,
			// but I can't find a reason.
    d_string s;
    int len;
    dchar *p;
    d_string searchString;
    int sslen;
    dchar *ssptr;
    int k;

#if 1
    // This is the 'transferable' version
    Value *v;
    void *a;
    v = othis->Get(TEXT_toString);
    a = v->Call(cc, othis, ret, 0, NULL);
    if (a)				// if exception was thrown
	return a;
    s = ret->toString();
#else
    // the 'builtin' version
    Value *v;
    v = &othis->value;
    s = v->toString();
#endif

    len = d_string_len(s);
    p = d_string_ptr(s);

    v1 = argc ? &arglist[0] : &vundefined;
    searchString = v1->toString();
    if (argc >= 2)
    {	d_number n;
	Value *v = &arglist[1];

	n = v->toNumber();
	if (Port::isnan(n) || n > len)
	    pos = len;
	else if (n < 0)
	    pos = 0;
	else
	    pos = (int) n;
    }
    else
	pos = len;

    sslen = d_string_len(searchString);
    ssptr = d_string_ptr(searchString);

    //WPRINTF(L"len = %d, p = '%ls'\n", len, p);
    //WPRINTF(L"pos = %d, sslen = %d, ssptr = '%ls'\n", pos, sslen, ssptr);

    dchar c;

    if (sslen == 1)
    {
	c = ssptr[0];
	for (k = pos; k >= 0; k--)
	{
	    //WPRINTF(L"k = %d, p+k = '%ls'\n", k, p+k);
	    if (p[k] == c)
		break;
	}
    }
    else if (sslen == 0)
	k = pos;
    else
    {
	c = ssptr[0];
	ssptr++;
	sslen--;
	for (k = pos; k >= 0; k--)
	{
	    //WPRINTF(L"k = %d, p+k = '%ls'\n", k, p+k);
	    if (p[k] == c && Dchar::memcmp(p + k + 1, ssptr, sslen) == 0)
		break;
	}
    }

    Vnumber::putValue(ret, k);
    return NULL;
}

/* ===================== Dstring_prototype_localeCompare ============= */

BUILTIN_FUNCTION2(Dstring_prototype_, localeCompare, 1)
{
    // ECMA v3 15.5.4.9
    d_string s1;
    d_string s2;
    d_number n;
    Value *v;

    v = &othis->value;
    s1 = v->toString();
    s2 = argc ? arglist[0].toString() : vundefined.toString();
    n = localeCompare(cc, s1, s2);
    Vnumber::putValue(ret, n);
    return NULL;
}

/* ===================== Dstring_prototype_match ============= */

BUILTIN_FUNCTION2(Dstring_prototype_, match, 1)
{
    // ECMA v3 15.5.4.10
    Dregexp *r;
    Dobject *o;

    if (argc && !arglist[0].isPrimitive() &&
	(o = arglist[0].toObject())->isDregexp())
    {
	;
    }
    else
    {	Vobject regret(NULL);

	Dregexp::getConstructor()->Construct(cc, &regret, argc, arglist);
	o = regret.object;
    }

    r = (Dregexp *)o;
    if (r->global->boolean)
    {
	Darray *a = new(cc) Darray;
	d_int32 n;
	d_int32 i;
	d_int32 lasti;

	i = 0;
	lasti = 0;
	for (n = 0; ; n++)
	{
	    Vnumber::putValue(r->lastIndex, i);
	    Dregexp::exec(r, ret, 1, &othis->value, EXEC_STRING);
	    if (!ret->x.string)		// if match failed
	    {
		Vnumber::putValue(r->lastIndex, i);
		break;
	    }
	    lasti = i;
	    i = (d_int32) r->lastIndex->toInt32();
	    if (i == lasti)		// if no source was consumed
		i++;			// consume a character

	    a->Put(n, ret, 0);		// a[n] = ret;
	}
	Vobject::putValue(ret, a);
    }
    else
    {
	Dregexp::exec(r, ret, 1, &othis->value, EXEC_ARRAY);
    }
    return NULL;
}

/* ===================== Dstring_prototype_replace ============= */

BUILTIN_FUNCTION2(Dstring_prototype_, replace, 2)
{
    // ECMA v3 15.5.4.11
    // String.prototype.replace(searchValue, replaceValue)

    d_string string;
    d_string searchString;
    d_string newstring;
    Value *searchValue;
    Value *replaceValue;
    Dregexp *r;
    RegExp *re;
    dchar *replacement;
    d_string result;
    int m;
    int i;
    int lasti;
    Match text;
    Dfunction *f;
    Value *v;

    v = &othis->value;
    string = v->toString();
    searchValue  = (argc >= 1) ? &arglist[0] : &vundefined;
    replaceValue = (argc >= 2) ? &arglist[1] : &vundefined;
    r = Dregexp::isRegExp(searchValue);
    f = Dfunction::isFunction(replaceValue);
    if (r)
    {	int offset = 0;

	re = r->re;
	i = 0;
	result = string;

	Vnumber::putValue(r->lastIndex, 0);
	for (;;)
	{
	    Dregexp::exec(r, ret, 1, &othis->value, EXEC_STRING);
	    if (!ret->x.string)		// if match failed
		break;

	    m = re->nparens;
	    if (f)
	    {
		Value *arglist;

		arglist = (Value *)alloca((m + 3) * sizeof(Value));
		assert(arglist);
		Vstring::putValue(&arglist[0], ret->x.string);
		for (i = 0; i < m; i++)
		{
		    Vstring::putValue(&arglist[1 + i],
			    Dstring::substring(string, re->parens[i].istart, re->parens[i].iend));
		}
		Vnumber::putValue(&arglist[m + 1], re->text.istart);
		Vstring::putValue(&arglist[m + 2], string);
		f->Call(cc, f, ret, m + 3, arglist);
		replacement = d_string_ptr(ret->toString());
	    }
	    else
	    {
		newstring = replaceValue->toString();
		replacement = re->replace2(d_string_ptr(newstring));
	    }
	    text.istart = re->text.istart + offset;
	    text.iend   = re->text.iend   + offset;
	    result = d_string_ctor(RegExp::replace4(d_string_ptr(result), &text, replacement));

	    if (re->attributes & REAglobal)
	    {
		offset += Dchar::len(replacement) - (text.iend - text.istart);

		// If no source was consumed, consume a character
		lasti = i;
		i = (d_int32) r->lastIndex->toInt32();
		if (i == lasti)
		{   i++;
		    Vnumber::putValue(r->lastIndex, i);
		}
	    }
	    else
		break;
	}
    }
    else
    {	dchar *match;

	searchString = searchValue->toString();
	match = Dchar::str(d_string_ptr(string), d_string_ptr(searchString));
	if (match)
	{
	    text.istart = match - d_string_ptr(string);
	    text.iend = text.istart + d_string_len(searchString);
	    if (f)
	    {
		Value *arglist;

		arglist = (Value *)alloca(3 * sizeof(Value));
		assert(arglist);
		Vstring::putValue(&arglist[0], searchString);
		Vnumber::putValue(&arglist[1], text.istart);
		Vstring::putValue(&arglist[2], string);
		f->Call(cc, f, ret, 3, arglist);
		replacement = d_string_ptr(ret->toString());
	    }
	    else
	    {
		newstring = replaceValue->toString();
		replacement = RegExp::replace3(d_string_ptr(newstring), d_string_ptr(string), &text, 0, NULL);
	    }
	    result = d_string_ctor(RegExp::replace4(d_string_ptr(string), &text, replacement));
	}
	else
	{
	    result = string;
	}
    }

    Vstring::putValue(ret, result);
    return NULL;
}

/* ===================== Dstring_prototype_search ============= */

BUILTIN_FUNCTION2(Dstring_prototype_, search, 1)
{
    // ECMA v3 15.5.4.12
    Dregexp *r;
    Dobject *o;

    //WPRINTF(L"String.prototype.search()\n");
    if (argc && !arglist[0].isPrimitive() &&
	(o = arglist[0].toObject())->isDregexp())
    {
	;
    }
    else
    {	Vobject regret(NULL);

	Dregexp::getConstructor()->Construct(cc, &regret, argc, arglist);
	o = regret.object;
    }

    r = (Dregexp *)o;
    Dregexp::exec(r, ret, 1, &othis->value, EXEC_INDEX);
    return NULL;
}

/* ===================== Dstring_prototype_slice ============= */

BUILTIN_FUNCTION2(Dstring_prototype_, slice, 2)
{
    // ECMA v3 15.5.4.13
    d_int32 start;
    d_int32 end;
    d_int32 len;
    d_string s;
    d_string r;
    Value *v;

    v = &othis->value;
    s = v->toString();
    len = d_string_len(s);
    switch (argc)
    {
	case 0:
	    start = 0;
	    end = len;
	    break;

	case 1:
	    start = arglist[0].toInt32();
	    end = len;
	    break;

	default:
	    start = arglist[0].toInt32();
	    end   = arglist[1].toInt32();
	    break;
    }

    if (start < 0)
    {
	start += len;
	if (start < 0)
	    start = 0;
    }
    else if (start >= len)
	start = len;

    if (end < 0)
    {
	end += len;
	if (end < 0)
	    end = 0;
    }
    else if (end >= len)
	end = len;

    len = end - start;
    if (len < 0)
	len = 0;

    r = Dstring::alloc(len);
    memcpy(d_string_ptr(r), d_string_ptr(s) + start, len * sizeof(dchar));

    Vstring::putValue(ret, r);
    return NULL;
}


/* ===================== Dstring_prototype_split ============= */

#if 1

BUILTIN_FUNCTION2(Dstring_prototype_, split, 2)
{
    // ECMA v3 15.5.4.14
    // String.prototype.split(separator, limit)
    d_uint32 lim;
    d_uint32 s;
    d_uint32 p;
    d_uint32 q;
    d_uint32 e;
    Value *separator = &vundefined;
    Value *limit = &vundefined;
    Dregexp *R;
    RegExp *re;
    d_string rs;
    d_uint32 rslen;
    d_string T;
    d_string S;
    Darray *A;

    switch (argc)
    {
	default:
	    limit = &arglist[1];
	case 1:
	    separator = &arglist[0];
	case 0:
	    break;
    }

    Value *v;
    v = &othis->value;
    S = v->toString();
    A = new(cc) Darray();
    if (limit->isUndefined())
	lim = ~0u;
    else
	lim = limit->toUint32();
    s = d_string_len(S);
    p = 0;
    R = Dregexp::isRegExp(separator);
    if (R)	// regular expression
    {	re = R->re;
	rs = d_string_null;
	rslen = 0;			// necessary for /W4
    }
    else	// string
    {	re = NULL;
	rs = separator->toString();
	rslen = d_string_len(rs);
    }
    if (lim == 0)
	goto Lret;

    // ECMA v3 15.5.4.14 is specific: "If separator is undefined, then the
    // result array contains just one string, which is the this value
    // (converted to a string)." However, neither Javascript nor Jscript
    // do that, they regard an undefined as being the string "undefined".
    // We match Javascript/Jscript behavior here, not ECMA.

#if 0  // set to 1 for ECMA compatibility
    if (!separator->isUndefined())
#endif
    {
	if (s)
	{
	L10:
	    for (q = p; q != s; q++)
	    {
		if (rs)			// string
		{
		    if (q + rslen <= s && !memcmp(d_string_ptr(S) + q, d_string_ptr(rs), rslen * sizeof(dchar)))
		    {
			e = q + rslen;
			if (e != p)
			{
			    T = Dstring::substring(S,p,q);
			    A->Put((unsigned) A->length.number, T, 0);
			    if (A->length.number == lim)
				goto Lret;
			    p = e;
			    goto L10;
			}
		    }
		}
		else		// regular expression
		{
		    if (re->test(d_string_ptr(S), q))
		    {	q = re->text.istart;
			e = re->text.iend;
			if (e != p)
			{
			    T = Dstring::substring(S,p,q);
			    //WPRINTF(L"S = '%ls', T = '%ls', p = %d, q = %d, e = %d\n", S, T, p, q, e);
			    A->Put((unsigned) A->length.number, T, 0);
			    if (A->length.number == lim)
				goto Lret;
			    p = e;
			    for (unsigned i = 0; i < re->nparens; i++)
			    {
				T = Dstring::substring(S, re->parens[i].istart, re->parens[i].iend);
				A->Put((unsigned) A->length.number, T, 0);
				if (A->length.number == lim)
				    goto Lret;
			    }
			    goto L10;
			}
		    }
		}
	    }
	    T = Dstring::substring(S, p, s);
	    A->Put((unsigned) A->length.number, T, 0);
	    goto Lret;
	}
	if (d_string_ptr(rs))		// string
	{
	    if (rslen <= s && !memcmp(d_string_ptr(S), d_string_ptr(rs), rslen * sizeof(dchar)))
		goto Lret;
	}
	else		// regular expression
	{
	    if (re->test(d_string_ptr(S), 0))
		goto Lret;
	}
    }

    A->Put(0u, S, 0);
Lret:
    Vobject::putValue(ret, A);
    return NULL;
}

#else

// This is the old ECMA v1 version (no regular expression support)

BUILTIN_FUNCTION2(Dstring_prototype_, split, 1)
{
    // ECMA 15.5.4.8
    // String.prototype.split(separator)
    d_string separator;
    Darray *a;
    d_string s;
    d_uint32 separator_length;
    d_uint32 s_length;
    Value *v;

    v = &othis->value;
    s = v->toString();
    s_length = d_string_len(s);
    a = new(cc) Darray();
    if (argc == 0)
    {
	a->Put((d_uint32) 0, s, 0);
    }
    else
    {
	separator = arglist[0].toString();
	separator_length = d_string_len(separator);
	if (separator_length == 0)
	{   d_uint32 i;

	    for (i = 0; i < s_length; i++)
	    {
		d_string sb;

		sb = Dstring::alloc(1);
		sb[0] = s[i];
		sb[1] = 0;
		a->Put(i, sb, 0);
	    }
	}
	else
	{
	    d_uint32 p;
	    d_uint32 k;
	    d_uint32 i;
	    d_string sb;
	    d_uint32 sb_len;

	    i = 0;
	    p = 0;
	L1:
	    for (k = p; k + separator_length <= s_length; k++)
	    {
		if (memcmp(&s[k], separator, separator_length * sizeof(dchar)) == 0)
		{
		    d_string sb;

		    sb_len = k - p;
		    sb = Dstring::alloc(sb_len);
		    memcpy(sb, s + p, sb_len * sizeof(dchar));
		    a->Put(i, sb, 0);
		    i++;
		    p = k + separator_length;
		    goto L1;
		}
	    }
	    sb_len = s_length - p;
	    sb = Dstring::alloc(sb_len);
	    memcpy(sb, s + p, sb_len * sizeof(dchar));
	    a->Put(i, sb, 0);
	}
    }
    Vobject::putValue(ret, a);
    return NULL;
}
#endif

/* ===================== Dstring_prototype_substr ============= */

void *dstring_substring(d_string s, d_int32 s_length, d_number start, d_number end, Value *ret)
{
    d_string sb;
    d_int32 sb_len;

    if (Port::isnan(start))
	start = 0;
    else if (start > s_length)
	start = s_length;
    else if (start < 0)
	start = 0;

    if (Port::isnan(end))
	end = 0;
    else if (end > s_length)
	end = s_length;
    else if (end < 0)
	end = 0;

    if (end < start)		// swap
    {	d_number t;

	t = start;
	start = end;
	end = t;
    }

    sb_len = (d_int32)(end - start);
    sb = Dstring::alloc(sb_len);
    memcpy(d_string_ptr(sb), d_string_ptr(s) + (d_int32)start, sb_len * sizeof(dchar));

    Vstring::putValue(ret, sb);
    return NULL;
}

BUILTIN_FUNCTION2(Dstring_prototype_, substr, 2)
{
    // Javascript: TDG pg. 689
    // String.prototype.substr(start, length)
    d_number start;
    d_number end;
    d_number length;
    d_string s;
    d_int32 s_length;
    Value *v;

    v = &othis->value;
    s = v->toString();
    s_length = d_string_len(s);
    start = 0;
    end = 0;
    if (argc >= 1)
    {
	Value *v = &arglist[0];
	start = v->toInteger();
	if (start < 0)
	    start = s_length + start;
	if (argc >= 2)
	{   v = &arglist[1];
	    length = v->toInteger();
	    if (Port::isnan(length) || length < 0)
		length = 0;
	}
	else
	    length = s_length - start;
    }
    else
	length = 0;
    end = start + length;

    return dstring_substring(s, s_length, start, end, ret);
}

/* ===================== Dstring_prototype_substring ============= */

BUILTIN_FUNCTION2(Dstring_prototype_, substring, 2)
{
    // ECMA 15.5.4.9
    // String.prototype.substring(start)
    // String.prototype.substring(start, end)
    d_number start;
    d_number end;
    d_string s;
    d_int32 s_length;
    Value *v;

    v = &othis->value;
    s = v->toString();
    s_length = d_string_len(s);
    start = 0;
    end = s_length;
    if (argc >= 1)
    {
	Value *v = &arglist[0];
	start = v->toInteger();
	if (argc >= 2)
	{   v = &arglist[1];
	    end = v->toInteger();
	}
	//WPRINTF(L"s = '%ls', start = %d, end = %d\n", s, start, end);
    }

    return dstring_substring(s, s_length, start, end, ret);
}

/* ===================== Dstring_prototype_toLowerCase ============= */

enum CASE
{
    CaseLower,
    CaseUpper,
    CaseLocaleLower,
    CaseLocaleUpper
};

void *tocase(Dobject *othis, Value *ret, enum CASE caseflag)
{
    d_string s;
    dchar *p;
    unsigned c;
    dchar *outp;
    int len;
    int i;
    Value *v;

    v = &othis->value;
    s = v->toString();
    p = d_string_ptr(s);
    len = d_string_len(s);
    outp = (dchar *)alloca(len * sizeof(dchar));
    //WPRINTF(L"tocase('%ls')\n", p);
    for (i = 0; i < len; i++)
    {
	c = Dchar::get(p);
	switch (caseflag)
	{
	    case CaseLower:
		c = Dchar::toLower((dchar)c);
		break;
	    case CaseUpper:
		c = Dchar::toUpper((dchar)c);
		break;
	    case CaseLocaleLower:
		c = Dchar::toLower((dchar)c);
		break;
	    case CaseLocaleUpper:
		c = Dchar::toUpper((dchar)c);
		break;
	    default:
		assert(0);
	}
	outp[i] = c;
	p = Dchar::inc(p);
    }
    // Create new string only if it differs from the old one
    if (memcmp(outp, p, len * sizeof(dchar)))
	s = Lstring::ctor(outp, len);
    Vstring::putValue(ret, s);
    return NULL;
}

BUILTIN_FUNCTION2(Dstring_prototype_, toLowerCase, 0)
{
    // ECMA 15.5.4.11
    // String.prototype.toLowerCase()

    //WPRINTF(L"Dstring_prototype_toLowerCase()\n");
    return tocase(othis, ret, CaseLower);
}

/* ===================== Dstring_prototype_toLocaleLowerCase ============= */

BUILTIN_FUNCTION2(Dstring_prototype_, toLocaleLowerCase, 0)
{
    // ECMA v3 15.5.4.17

    //WPRINTF(L"Dstring_prototype_toLocaleLowerCase()\n");
    return tocase(othis, ret, CaseLocaleLower);
}

/* ===================== Dstring_prototype_toUpperCase ============= */

BUILTIN_FUNCTION2(Dstring_prototype_, toUpperCase, 0)
{
    // ECMA 15.5.4.12
    // String.prototype.toUpperCase()

    return tocase(othis, ret, CaseUpper);
}

/* ===================== Dstring_prototype_toLocaleUpperCase ============= */

BUILTIN_FUNCTION2(Dstring_prototype_, toLocaleUpperCase, 0)
{
    // ECMA v3 15.5.4.18

    return tocase(othis, ret, CaseLocaleUpper);
}

/* ===================== Dstring_prototype_anchor ============= */

/********************
 * Copy char into dchar string.
 */

void my_cpy(dchar *to, char *from)
{
    unsigned len = strlen(from);
    unsigned i;

    for (i = 0; i < len; i++)
	to[i] = from[i];
}

void *dstring_anchor(Dobject *othis, Value *ret, char *tag, char *name, unsigned argc, Value *arglist)
{
    // For example:
    //	"foo".anchor("bar")
    // produces:
    //	<tag name="bar">foo</tag>

    d_string s;
    d_int32 s_length;
    Value *v;

    Value *va;
    d_string anchor;
    d_int32 anchor_length;

    d_string result;
    unsigned len;
    dchar *p;

    unsigned tag_length = strlen(tag);
    unsigned name_length = strlen(name);

    v = &othis->value;
    s = v->toString();
    s_length = d_string_len(s);

    va = argc ? &arglist[0] : &vundefined;
    anchor = va->toString();
    anchor_length = d_string_len(anchor);

    len = 9 + tag_length * 2 + name_length + anchor_length + s_length;
    result = Dstring::alloc(len);
    p = d_string_ptr(result);
    p[0] = '<';
    p += 1;
    my_cpy(p, tag);
    p += tag_length;
    p[0] = ' ';
    p += 1;
    my_cpy(p, name);
    p += name_length;
    p[0] = '=';
    p[1] = '"';
    p += 2;
    memcpy(p, d_string_ptr(anchor), anchor_length * sizeof(dchar));
    p += anchor_length;
    p[0] = '"';
    p[1] = '>';
    p += 2;
    memcpy(p, d_string_ptr(s), s_length * sizeof(dchar));
    p += s_length;
    p[0] = '<';
    p[1] = '/';
    p += 2;
    my_cpy(p, tag);
    p += tag_length;
    p[0] = '>';

    Vstring::putValue(ret, result);
    return NULL;
}


BUILTIN_FUNCTION2(Dstring_prototype_, anchor, 1)
{
    // Non-standard extension
    // String.prototype.anchor(anchor)
    // For example:
    //	"foo".anchor("bar")
    // produces:
    //	<A NAME="bar">foo</A>

    return dstring_anchor(othis, ret, "A", "NAME", argc, arglist);
}

BUILTIN_FUNCTION2(Dstring_prototype_, fontcolor, 1)
{
    return dstring_anchor(othis, ret, "FONT", "COLOR", argc, arglist);
}

BUILTIN_FUNCTION2(Dstring_prototype_, fontsize, 1)
{
    return dstring_anchor(othis, ret, "FONT", "SIZE", argc, arglist);
}

BUILTIN_FUNCTION2(Dstring_prototype_, link, 1)
{
    return dstring_anchor(othis, ret, "A", "HREF", argc, arglist);
}


/* ===================== Dstring_prototype bracketing ============= */

/***************************
 * Produce <tag>othis</tag>
 */

void *dstring_bracket(Dobject *othis, Value *ret, char *tag)
{
    d_string s;
    d_int32 s_length;
    Value *v;

    unsigned tag_length;

    d_string result;
    unsigned len;
    dchar *p;

    v = &othis->value;
    s = v->toString();
    s_length = d_string_len(s);

    tag_length = strlen(tag);

    len = 5 + 2 * tag_length + s_length;
    result = Dstring::alloc(len);
    p = d_string_ptr(result);

    p[0] = '<';
    my_cpy(p + 1, tag);
    p += 1 + tag_length;
    p[0] = '>';
    memcpy(p + 1, d_string_ptr(s), s_length * sizeof(dchar));
    p += 1 + s_length;
    p[0] = '<';
    p[1] = '/';
    my_cpy(p + 2, tag);
    p += 2 + tag_length;
    p[0] = '>';

    Vstring::putValue(ret, result);
    return NULL;
}

BUILTIN_FUNCTION2(Dstring_prototype_, big, 0)
{
    // Non-standard extension
    // String.prototype.big()
    // For example:
    //	"foo".big()
    // produces:
    //	<BIG>foo</BIG>

    return dstring_bracket(othis, ret, "BIG");
}

BUILTIN_FUNCTION2(Dstring_prototype_, blink, 0)
{
    return dstring_bracket(othis, ret, "BLINK");
}

BUILTIN_FUNCTION2(Dstring_prototype_, bold, 0)
{
    return dstring_bracket(othis, ret, "B");
}

BUILTIN_FUNCTION2(Dstring_prototype_, fixed, 0)
{
    return dstring_bracket(othis, ret, "TT");
}

BUILTIN_FUNCTION2(Dstring_prototype_, italics, 0)
{
    return dstring_bracket(othis, ret, "I");
}

BUILTIN_FUNCTION2(Dstring_prototype_, small, 0)
{
    return dstring_bracket(othis, ret, "SMALL");
}

BUILTIN_FUNCTION2(Dstring_prototype_, strike, 0)
{
    return dstring_bracket(othis, ret, "STRIKE");
}

BUILTIN_FUNCTION2(Dstring_prototype_, sub, 0)
{
    return dstring_bracket(othis, ret, "SUB");
}

BUILTIN_FUNCTION2(Dstring_prototype_, sup, 0)
{
    return dstring_bracket(othis, ret, "SUP");
}



/* ===================== Dstring_prototype ==================== */

struct Dstring_prototype : Dstring
{
    Dstring_prototype(ThreadContext *tc);
};

Dstring_prototype::Dstring_prototype(ThreadContext *tc)
    : Dstring(tc->Dobject_prototype)
{
    Put(TEXT_constructor, tc->Dstring_constructor, DontEnum);

    static NativeFunctionData nfd[] =
    {
    #define X(name,len) { &TEXT_##name, #name, Dstring_prototype_##name, len }
	X(toString, 0),
	X(valueOf, 0),
	X(charAt, 1),
	X(charCodeAt, 1),
	X(concat, 1),
	X(indexOf, 1),
	X(lastIndexOf, 1),
	X(localeCompare, 1),
	X(match, 1),
	X(replace, 2),
	X(search, 1),
	X(slice, 2),
	X(split, 2),
	X(substr, 2),
	X(substring, 2),
	X(toLowerCase, 0),
	X(toLocaleLowerCase, 0),
	X(toUpperCase, 0),
	X(toLocaleUpperCase, 0),
	X(anchor, 1),
	X(fontcolor, 1),
	X(fontsize, 1),
	X(link, 1),
	X(big, 0),
	X(blink, 0),
	X(bold, 0),
	X(fixed, 0),
	X(italics, 0),
	X(small, 0),
	X(strike, 0),
	X(sub, 0),
	X(sup, 0),
    #undef X
    };

    DnativeFunction::init(this, nfd, sizeof(nfd) / sizeof(nfd[0]), DontEnum);
}

/* ===================== Dstring ==================== */

Dstring::Dstring(d_string s)
    : Dobject(Dstring::getPrototype())
{
    classname = TEXT_String;

    Put(TEXT_length, d_string_len(s), DontEnum | DontDelete | ReadOnly);
    Vstring::putValue(&value, s);
}

Dstring::Dstring(Dobject *prototype)
    : Dobject(prototype)
{   d_string s = TEXT_;

    classname = TEXT_String;
    Put(TEXT_length, d_string_len(s), DontEnum | DontDelete | ReadOnly);
    Vstring::putValue(&value, s);
}

Dfunction *Dstring::getConstructor()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Dstring_constructor;
}

Dobject *Dstring::getPrototype()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Dstring_prototype;
}

void Dstring::init(ThreadContext *tc)
{
    GC_LOG();
    tc->Dstring_constructor = new(&tc->mem) Dstring_constructor(tc);
    GC_LOG();
    tc->Dstring_prototype = new(&tc->mem) Dstring_prototype(tc);

    tc->Dstring_constructor->Put(TEXT_prototype, tc->Dstring_prototype, DontEnum | DontDelete | ReadOnly);
}
