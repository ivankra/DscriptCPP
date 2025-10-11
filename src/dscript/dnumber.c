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
#include <malloc.h>
#include <math.h>

#include "port.h"
#include "root.h"
#include "dscript.h"
#include "value.h"
#include "dobject.h"
#include "text.h"

/* ===================== Dnumber_constructor ==================== */

struct Dnumber_constructor : Dfunction
{
    Dnumber_constructor(ThreadContext *tc);

    void *Construct(CONSTRUCT_ARGS);
    void *Call(CALL_ARGS);
};

Dnumber_constructor::Dnumber_constructor(ThreadContext *tc)
    : Dfunction(1, tc->Dfunction_prototype)
{
    unsigned attributes = DontEnum | DontDelete | ReadOnly;

    name = "Number";
    Put(TEXT_MAX_VALUE, Port::dbl_max, attributes);
    Put(TEXT_MIN_VALUE, 5e-324 /*Port::dbl_min*/, attributes);
    Put(TEXT_NaN, Port::nan, attributes);
    Put(TEXT_NEGATIVE_INFINITY, -Port::infinity, attributes);
    Put(TEXT_POSITIVE_INFINITY, Port::infinity, attributes);
}

void *Dnumber_constructor::Call(CALL_ARGS)
{
    // ECMA 15.7.1
    d_number n;
    Value *v;

    v = &arglist[0];
    n = (argc) ? v->toNumber() : 0;
    Vnumber::putValue(ret, n);
    return NULL;
}

void *Dnumber_constructor::Construct(CONSTRUCT_ARGS)
{
    // ECMA 15.7.2
    d_number n;
    Dobject *o;

    n = (argc) ? arglist[0].toNumber() : 0;
    GC_LOG();
    o = new(cc) Dnumber(n);
    Vobject::putValue(ret, o);
    return NULL;
}

/* ===================== Dnumber_prototype_toString =============== */

BUILTIN_FUNCTION(Dnumber_prototype_, toString, 1)
{
    // ECMA v3 15.7.4.2
    d_string s;

    // othis must be a Number
    if (!othis->isClass(TEXT_Number))
    {
	Value::copy(ret, &vundefined);
	ErrInfo errinfo;
	errinfo.code = 5001;
	return RuntimeError(&errinfo,
		ERR_FUNCTION_WANTS_NUMBER,
		DTEXT("Number.prototype"), DTEXT("toString()"),
		d_string_ptr(othis->classname));
    }
    else
    {	Value *v;

	v = &((Dnumber *)othis)->value;

	if (argc)
	{
	    d_number radix;
	    Value *varg;

	    varg = &arglist[0];
	    radix = varg->toNumber();
	    if (radix == 10.0 || varg->isUndefined())
		s = v->toString();
	    else
	    {
		int r;

		r = (int) radix;
		// radix must be an integer 2..36
		if (r == radix && r >= 2 && r <= 36)
		    s = v->toString(r);
		else
		    s = v->toString();
	    }
	}
	else
	    s = v->toString();
	Vstring::putValue(ret, s);
    }
    return NULL;
}

/* ===================== Dnumber_prototype_toLocaleString =============== */

BUILTIN_FUNCTION(Dnumber_prototype_, toLocaleString, 1)
{
    // ECMA v3 15.7.4.3
    d_string s;

    // othis must be a Number
    if (!othis->isClass(TEXT_Number))
    {
	Value::copy(ret, &vundefined);
	ErrInfo errinfo;
	errinfo.code = 5001;
	return RuntimeError(&errinfo,
		ERR_FUNCTION_WANTS_NUMBER,
		DTEXT("Number.prototype"), DTEXT("toLocaleString()"),
		d_string_ptr(othis->classname));
    }
    else
    {	Value *v;

	v = &((Dnumber *)othis)->value;

	s = v->toLocaleString();
	Vstring::putValue(ret, s);
    }
    return NULL;
}

/* ===================== Dnumber_prototype_valueOf =============== */

BUILTIN_FUNCTION(Dnumber_prototype_, valueOf, 0)
{
    // othis must be a Number
    if (!othis->isClass(TEXT_Number))
    {
	Value::copy(ret, &vundefined);
	ErrInfo errinfo;
	errinfo.code = 5001;
	return RuntimeError(&errinfo, ERR_FUNCTION_WANTS_NUMBER,
		DTEXT("Number.prototype"), DTEXT("valueOf()"),
		d_string_ptr(othis->classname));
    }
    else
    {
	Value *v;

	v = &((Dnumber *)othis)->value;
	Value::copy(ret, v);
    }
    return NULL;
}

/* ===================== Formatting Support =============== */

#define FIXED_DIGITS	20	// ECMA says >= 20


// power of tens array, indexed by power

static d_number tens[FIXED_DIGITS+1] =
{
    1,   1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9,
    1e10,1e11,1e12,1e13,1e14,1e15,1e16,1e17,1e18,1e19,
    1e20,
};

/************************************************
 * Let e and n be integers such that
 * 10**f <= n < 10**(f+1) and for which the exact
 * mathematical value of n * 10**(e-f) - x is as close
 * to zero as possible. If there are two such sets of
 * e and n, pick the e and n for which n * 10**(e-f)
 * is larger.
 */

number_t deconstruct_real(d_number x, int f, int *pe)
{
    number_t n;
    int e;
    int i;

    e = (int) log10(x);
    i = e - f;
    if (i >= 0 && i < (int)(sizeof(tens) / sizeof(tens[0])))
	n = (number_t)(x / tens[i] + 0.5);	// table lookup for speed & accuracy
    else
	n = (number_t)(x / pow(10.0, i) + 0.5);

    *pe = e;
    return n;
}

/* ===================== Dnumber_prototype_toFixed =============== */

BUILTIN_FUNCTION(Dnumber_prototype_, toFixed, 1)
{
    // ECMA v3 15.7.4.5
    Value *v;
    d_number x;
    d_number fractionDigits;
    d_string result;
    int dup;

    v = &arglist[0];
    fractionDigits = (argc) ? v->toInteger() : 0;
    if (fractionDigits < 0 || fractionDigits > FIXED_DIGITS)
    {
	ErrInfo errinfo;

	Value::copy(ret, &vundefined);
	return RangeError(&errinfo, ERR_VALUE_OUT_OF_RANGE,
		DTEXT("Number.prototype.toFixed()"), DTEXT("fractionDigits"));
    }
    v = &othis->value;
    x = v->toNumber();
    if (Port::isnan(x))
    {
	result = TEXT_NaN;		// return "NaN"
    }
    else
    {	int sign;
	dchar *m;

	sign = 0;
	if (x < 0)
	{
	    sign = 1;
	    x = -x;
	}
	if (x >= 10.0e+21)		// exponent must be FIXED_DIGITS+1
	{
	    Vnumber vn(x);

	    m = d_string_ptr(vn.toString());
	    dup = 0;
	}
	else
	{
	    number_t n;
	    dchar buffer[32 + 1];
	    d_number tenf;
	    int f;

	    f = (int) fractionDigits;
	    assert(f < (int)(sizeof(tens) / sizeof(tens[0])));
	    tenf = tens[f];		// tenf = 10**f

	    // Compute n which gives |(n / tenf) - x| is the smallest
	    // value. If there are two such n's, pick the larger.
	    n = (number_t)(x * tenf + 0.5);		// round up & chop

	    if (n == 0)
	    {	m = DTEXT("0");
		dup = 0;
	    }
	    else
	    {
		// n still doesn't give 20 digits, only 19
		m = Port::ull_to_string(buffer, n);
		dup = 1;
	    }
	    if (f != 0)
	    {	int i;
		int k;

		k = Dchar::len(m);
		if (k <= f)
		{   dchar *s;
		    int nzeros;

		    s = (dchar *)alloca((f + 1 + 1) * sizeof(dchar));
		    assert(s);
		    nzeros = f + 1 - k;
		    for (i = 0; i < nzeros; i++)
			s[i] = DTEXT('0');

		    for ( ; i < f + 1; i++)
			s[i] = m[i - nzeros];
		    assert(m[i - nzeros] == 0);
		    s[i] = 0;

		    m = s;
		    k = f + 1;
		}

		// result = "-" + m[0 .. k-f] + "." + m[k-f .. k];
		result = Dstring::alloc(sign + k + 1);
		dchar *r = d_string_ptr(result);
		if (sign)
		    r[0] = DTEXT('-');
		for (i = 0; i < k - f; i++)
		    r[sign + i] = m[i];
		r[sign + i] = DTEXT('.');
		for (; i < k; i++)
		    r[sign + i + 1] = m[i];
		goto Ldone;
	    }
	}
	GC_LOG();
	if (sign)
	    result = Dstring::dup2(this, TEXT_dash, d_string_ctor(m));
	else
	    result = Dstring::dup(this, m);
    }

Ldone:
    Vstring::putValue(ret, result);
    return NULL;
}

/* ===================== Dnumber_prototype_toExponential =============== */

BUILTIN_FUNCTION(Dnumber_prototype_, toExponential, 1)
{
    // ECMA v3 15.7.4.6
    Value *varg;
    Value *v;
    d_number x;
    d_number fractionDigits;
    d_string result;

    varg = &arglist[0];
    fractionDigits = (argc) ? varg->toInteger() : 0;

    v = &othis->value;
    x = v->toNumber();
    if (Port::isnan(x))
    {
	result = TEXT_NaN;		// return "NaN"
    }
    else
    {	int sign;

	sign = 0;
	if (x < 0)
	{
	    sign = 1;
	    x = -x;
	}
	if (Port::isinfinity(x))
	{
	    result = sign ? TEXT_negInfinity : TEXT_Infinity;
	}
	else
	{   int f;
	    number_t n;
	    int e;
	    dchar *m;
	    int i;
	    dchar buffer[32 + 1];

	    if (fractionDigits < 0 || fractionDigits > FIXED_DIGITS)
            {
		ErrInfo errinfo;

	        Value::copy(ret, &vundefined);
		return RangeError(&errinfo,
			ERR_VALUE_OUT_OF_RANGE,
			DTEXT("Number.prototype.toExponential()"),
			DTEXT("fractionDigits"));
            }

	    f = (int) fractionDigits;
	    if (x == 0)
	    {
		m = (dchar *)alloca((f + 1 + 1) * sizeof(dchar));
		assert(m);
		for (i = 0; i < f + 1; i++)
		    m[i] = DTEXT('0');
		m[i] = 0;
		e = 0;
	    }
	    else
	    {
		if (argc && !varg->isUndefined())
		{
		    /* Step 12
		     * Let e and n be integers such that
		     * 10**f <= n < 10**(f+1) and for which the exact
		     * mathematical value of n * 10**(e-f) - x is as close
		     * to zero as possible. If there are two such sets of
		     * e and n, pick the e and n for which n * 10**(e-f)
		     * is larger.
		     * [Note: this is the same as Step 15 in toPrecision()
		     *  with f = p - 1]
		     */
		    n = deconstruct_real(x, f, &e);
		}
		else
		{
		    /* Step 19
		     * Let e, n, and f be integers such that f >= 0,
		     * 10**f <= n < 10**(f+1), the number value for
		     * n * 10**(e-f) is x, and f is as small as possible.
		     * Note that the decimal representation of n has f+1
		     * digits, n is not divisible by 10, and the least
		     * significant digit of n is not necessarilly uniquely
		     * determined by these criteria.
		     */
		    /* Implement by trying maximum digits, and then
		     * lopping off trailing 0's.
		     */
		    f = 19;		// should use FIXED_DIGITS
		    n = deconstruct_real(x, f, &e);

		    // Lop off trailing 0's
		    assert(n);
		    while ((n % 10) == 0)
		    {
			n /= 10;
			f--;
			assert(f >= 0);
		    }
		}
		// n still doesn't give 20 digits, only 19
		m = Port::ull_to_string(buffer, n);
	    }
	    if (f)
	    {	dchar *s;

		// m = m[0] + "." + m[1 .. f+1];
		s = (dchar *)alloca((f + 1 + 1 + 1) * sizeof(dchar));
		assert(s);
		s[0] = m[0];
		s[1] = DTEXT('.');
		for (i = 1; i < f + 1; i++)
		    s[1 + i] = m[i];
		s[1 + i] = 0;
		m = s;
	    }

	    // result = sign + m + "e" + c + e;
	    dchar *c = DTEXT("+");
	    if (e < 0)
		c++;

	    int len = sign + Dchar::len(m) + 1 + 1 + 32;
	    result = Dstring::alloc(len);
#if defined UNICODE
	    SWPRINTF(d_string_ptr(result), len + 1, L"%s%se%s%d", L"-" + (1 - sign), m, c, e);
#else
	    sprintf(d_string_ptr(result), "%s%se%s%d", "-" + (1 - sign), m, c, e);
#endif
	    d_string_setLen(result, Dchar::len(d_string_ptr(result)));
	}
    }

    Vstring::putValue(ret, result);
    return NULL;
}

/* ===================== Dnumber_prototype_toPrecision =============== */

BUILTIN_FUNCTION(Dnumber_prototype_, toPrecision, 1)
{
    // ECMA v3 15.7.4.7
    Value *varg;
    Value *v;
    d_number x;
    d_number precision;
    d_string result;

    v = &othis->value;
    x = v->toNumber();

    varg = (argc == 0) ? &vundefined : &arglist[0];

    if (argc == 0 || varg->isUndefined())
    {
	Vnumber vn(x);

	result = vn.toString();
    }
    else
    {
	if (Port::isnan(x))
	    result = TEXT_NaN;
	else
	{   int sign;
	    int e;
	    int p;
	    int i;
	    dchar *m;
	    number_t n;
	    dchar buffer[32 + 1];

	    sign = 0;
	    if (x < 0)
	    {
		sign = 1;
		x = -x;
	    }

	    if (Port::isinfinity(x))
	    {
		result = sign ? TEXT_negInfinity : TEXT_Infinity;
		goto Ldone;
	    }

	    precision = varg->toInteger();
	    if (precision < 1 || precision > 21)
            {
		ErrInfo errinfo;

	        Value::copy(ret, &vundefined);
		return RangeError(&errinfo,
			ERR_VALUE_OUT_OF_RANGE,
			DTEXT("Number.prototype.toPrecision()"),
			DTEXT("precision"));
            }

	    p = (int) precision;
	    if (x != 0)
	    {
		/* Step 15
		 * Let e and n be integers such that 10**(p-1) <= n < 10**p
		 * and for which the exact mathematical value of n * 10**(e-p+1) - x
		 * is as close to zero as possible. If there are two such sets
		 * of e and n, pick the e and n for which n * 10**(e-p+1) is larger.
		 */
		n = deconstruct_real(x, p - 1, &e);

		// n still doesn't give 20 digits, only 19
		m = Port::ull_to_string(buffer, n);

		if (e < -6 || e >= p)
		{
		    // result = sign + m[0] + "." + m[1 .. p] + "e" + c + e;
		    dchar *s;
		    dchar *c = DTEXT("+");
		    if (e < 0)
			c++;

		    int len = sign + p + 1 + 1 + 1 + 32 + 1;
		    s = (dchar *)alloca(len * sizeof(dchar));
		    assert(s);
#if UNICODE
		    SWPRINTF(s, len, L"%s%c.%se%s%d", L"-" + (1 - sign), m[0], m + 1, c, e);
#else
		    sprintf(s, "%s%c.%se%s%d", "-" + (1 - sign), m[0], m + 1, c, e);
#endif
		    GC_LOG();
		    result = Dstring::dup(this, s);
		    goto Ldone;
		}
	    }
	    else
	    {
		// Step 12
		// m = array[p] of '0'
		m = (dchar *)alloca((p + 1) * sizeof(dchar));
		assert(m);
		for (i = 0; i < p; i++)
		    m[i] = DTEXT('0');
		m[i] = 0;

		e = 0;
	    }
	    if (e != p - 1)
	    {	dchar *s;

		if (e >= 0)
		{
		    // m = m[0 .. e+1] + "." + m[e+1 .. p-(e+1)];

		    s = (dchar *)alloca((p + 1 + 1) * sizeof(dchar));
		    assert(s);
		    for (i = 0; i < e + 1; i++)
			s[i] = m[i];
		    s[i] = DTEXT('.');
		    for (; i <= p; i++)			// copy trailing 0 too
			s[i + 1] = m[i];
		    m = s;
		}
		else
		{
		    // m = "0." + (-(e+1) occurrences of the character '0') + m;
		    int imax = 2 + -(e + 1);

		    s = (dchar *)alloca((imax + p + 1) * sizeof(dchar));
		    assert(s);
		    s[0] = DTEXT('0');
		    s[1] = DTEXT('.');
		    for (i = 2; i < imax; i++)
			s[i] = DTEXT('0');
		    for (int j = 0; j <= p; j++)	// copy trailing 0 too
			s[i + j] = m[j];
		    m = s;
		}
	    }
	    GC_LOG();
	    if (sign)
		result = Dstring::dup2(this, TEXT_dash, d_string_ctor(m));
	    else
		result = Dstring::dup(this, m);
	}
    }

Ldone:
    Vstring::putValue(ret, result);
    return NULL;
}

/* ===================== Dnumber_prototype ==================== */

struct Dnumber_prototype : Dnumber
{
    Dnumber_prototype(ThreadContext *tc);
};

Dnumber_prototype::Dnumber_prototype(ThreadContext *tc)
    : Dnumber(tc->Dobject_prototype)
{   unsigned attributes = DontEnum;

    Dobject *f = tc->Dfunction_prototype;

    Put(TEXT_constructor, tc->Dnumber_constructor, attributes);
    Put(TEXT_toString, new(this) Dnumber_prototype_toString(f), attributes);
    // Permissible to use toString()
    Put(TEXT_toLocaleString, new(this) Dnumber_prototype_toLocaleString(f), attributes);
    Put(TEXT_valueOf, new(this) Dnumber_prototype_valueOf(f), attributes);
    Put(TEXT_toFixed, new(this) Dnumber_prototype_toFixed(f), attributes);
    Put(TEXT_toExponential, new(this) Dnumber_prototype_toExponential(f), attributes);
    Put(TEXT_toPrecision, new(this) Dnumber_prototype_toPrecision(f), attributes);
}

/* ===================== Dnumber ==================== */

Dnumber::Dnumber(d_number n)
    : Dobject(Dnumber::getPrototype())
{
    classname = TEXT_Number;
    Vnumber::putValue(&value, n);
}

Dnumber::Dnumber(Dobject *prototype)
    : Dobject(prototype)
{
    classname = TEXT_Number;
    Vnumber::putValue(&value, 0);
}

Dfunction *Dnumber::getConstructor()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Dnumber_constructor;
}

Dobject *Dnumber::getPrototype()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Dnumber_prototype;
}

void Dnumber::init(ThreadContext *tc)
{
    GC_LOG();
    tc->Dnumber_constructor = new(&tc->mem) Dnumber_constructor(tc);
    GC_LOG();
    tc->Dnumber_prototype = new(&tc->mem) Dnumber_prototype(tc);

    tc->Dnumber_constructor->Put(TEXT_prototype, tc->Dnumber_prototype, DontEnum | DontDelete | ReadOnly);
}


