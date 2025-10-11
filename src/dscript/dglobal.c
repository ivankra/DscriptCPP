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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#if _MSC_VER
#include <malloc.h>
#endif

#include "port.h"
#include "root.h"
#include "dscript.h"
#include "dobject.h"
#include "dregexp.h"
#include "ir.h"
#include "program.h"
#include "parse.h"
#include "statement.h"
#include "scope.h"
#include "text.h"

#define LOG 0

dchar TOHEX[16+1] = L"0123456789ABCDEF";

/* ====================== Dglobal_eval ================ */

BUILTIN_FUNCTION2(Dglobal_, eval, 1)
{
    // ECMA 15.1.2.1
    Value *v;
    d_string s;
    FunctionDefinition *fd;
    ErrInfo errinfo;
    void *p1 = NULL;

    //FuncLog funclog(L"Global.eval()");

    v = argc ? &arglist[0] : &vundefined;
    if (v->getType() != TypeString)
    {
	Value::copy(ret, v);
	return NULL;
    }
    s = v->toString();
    //WPRINTF(L"eval('%ls')\n", s);

    // Parse program
    Array *topstatements;
    Parser p("eval", s);
    if (p.parseProgram(&topstatements, &errinfo))
	goto Lsyntaxerror;

    // Analyze, generate code
    fd = new(cc) FunctionDefinition(topstatements);
    fd->iseval = 1;
    {	// Use {} to get destructor for sc called
	Scope sc(fd);
	sc.src = d_string_ptr(s);
	fd->semantic(&sc);
	errinfo = sc.errinfo;
    }
    if (errinfo.message)
	goto Lsyntaxerror;
    fd->toIR(NULL);

    // Execute code
    Value *locals;

    if (fd->nlocals >= 128 ||
	(locals = (Value *) alloca(fd->nlocals * sizeof(Value))) == NULL)
    {
	p1 = mem.malloc(fd->nlocals * sizeof(Value));
	locals = (Value *)p1;
    }

    void *result;
#if 0
    Array scope;
    scope.reserve(cc->scoperoot + fd->withdepth + 2);
    for (unsigned u = 0; u < cc->scoperoot; u++)
	scope.push(cc->scope->data[u]);

    Array *scopesave = cc->scope;
    cc->scope = &scope;
    Dobject *variablesave = cc->variable;
    cc->variable = cc->global;

    fd->instantiate(cc->scope, cc->scope->dim cc->variable, 0);

    // The this value is the same as the this value of the
    // calling context.
    result = IR::call(cc, othis, fd->code, ret, locals);

    mem.free(p1);
    cc->variable = variablesave;
    cc->scope = scopesave;
    return result;
#else
    // The scope chain is initialized to contain the same objects,
    // in the same order, as the calling context's scope chain.
    // This includes objects added to the calling context's
    // scope chain by WithStatement.
    cc->scope->reserve(fd->withdepth);

    // Variable instantiation is performed using the calling
    // context's variable object and using empty
    // property attributes
    fd->instantiate(cc->scope, cc->scope->dim, cc->variable, 0);

    // The this value is the same as the this value of the
    // calling context.
    assert(cc->callerothis);
    result = IR::call(cc, cc->callerothis, fd->code, ret, locals);
    if (p1)
	mem.free(p1);
    fd = NULL;
    //if (result) WPRINTF(L"result = '%s'\n", d_string_ptr(((Value *)result)->toString()));
    return result;
#endif

Lsyntaxerror:
    Dobject *o;

    // For eval()'s, use location of caller, not the string
    errinfo.linnum = 0;

    Value::copy(ret, &vundefined);
    o = new(cc) Dsyntaxerror(&errinfo);
    return new(cc) Vobject(o);
}

/* ====================== Dglobal_parseInt ================ */

BUILTIN_FUNCTION2(Dglobal_, parseInt, 2)
{
    // ECMA 15.1.2.2
    Value *v1;
    Value *v2;
    dchar *s;
    dchar *z;
    d_int32 radix;
    int sign = 1;
    d_number number;
    unsigned len;
    unsigned i;
    d_string string;

    if (argc)
	v1 = &arglist[0];
    else
	v1 = &vundefined;
    string = v1->toString();
    s = d_string_ptr(string);
    len = d_string_len(string);
    i = len;

    while (i && Dstring::isStrWhiteSpaceChar(*s))
    {	s++;
	i--;
    }
    if (i)
    {
	if (*s == '-')
	{   sign = -1;
	    s++;
	    i--;
	}
	else if (*s == '+')
	{   s++;
	    i--;
	}
    }

    radix = 0;
    if (argc >= 2)
    {
	v2 = &arglist[1];
	radix = v2->toInt32();
    }

    if (radix)
    {
	if (radix < 2 || radix > 36)
	{
	    number = Port::nan;
	    goto Lret;
	}
	if (radix == 16 && i >= 2 && *s == '0' && 
	    (s[1] == 'x' || s[1] == 'X'))
	{
	    s += 2;
	    i -= 2;
	}
    }
    else if (i >= 1 && *s != '0')
    {
	radix = 10;
    }
    else if (i >= 2 && (s[1] == 'x' || s[1] == 'X'))
    {
	radix = 16;
	s += 2;
	i -= 2;
    }
    else
	radix = 8;

    number = 0;
    for (z = s; i; z++, i--)
    {   d_int32 n;
	dchar c;

	c = *z;
	if ('0' <= c && c <= '9')
	    n = c - '0';
	else if ('A' <= c && c <= 'Z')
	    n = c - 'A' + 10;
	else if ('a' <= c && c <= 'z')
	    n = c - 'a' + 10;
	else
	    break;
	if (radix <= n)
	    break;
	number = number * radix + n;
    }
    if (z == s)
    {
	number = Port::nan;
	goto Lret;
    }
    if (sign < 0)
	number = -number;

#if 0    // ECMA says to silently ignore trailing characters
    while (Dstring::isStrWhiteSpaceChar(*z))
	z++;
    if (z - d_string_ptr(string) != (int)len)
    {
	number = Port::nan;
	goto Lret;
    }
#endif

Lret:
    Vnumber::putValue(ret, number);
    return NULL;
}

/* ====================== Dglobal_parseFloat ================ */

BUILTIN_FUNCTION2(Dglobal_, parseFloat, 1)
{
    // ECMA 15.1.2.3
    Value *v;
    dchar *s;
    d_number n;

    if (argc)
	v = &arglist[0];
    else
	v = &vundefined;
    s = d_string_ptr(v->toString());

    // This is similar to Dstring::toNumber(), but hex literals are not
    // recognized
    dchar *nptr;
    int sign;

    // Leading whitespace
    while (isspace(*s))
	s++;

    // Check for [+|-]
    nptr = s;
    switch (*s)
    {	case '+':
	    sign = 0;
	    s++;
	    break;

	case '-':
	    sign = 1;
	    s++;
	    break;

	default:
	    sign = 0;
	    break;
    }

    if (memcmp(s, d_string_ptr(TEXT_Infinity), 8 * sizeof(dchar)) == 0)
    {
	n = sign ? -Port::infinity : Port::infinity;
    }
#if defined linux
    // linux wcstod() has a strange problem where things
    // like 0XA are converted.
    // Need to compensate.
    else if (nptr[0] == '0' && (nptr[1] == 'x' || nptr[1] == 'X'))
    {
	n = 0;
    }
#endif
    else
    {	dchar *endptr;

#if defined UNICODE
	n = wcstod(nptr, &endptr);
#else
	n = strtod(nptr, &endptr);
#endif
	if (nptr == endptr)
	    n = Port::nan;
    }
    Vnumber::putValue(ret, n);
    return NULL;
}

/* ====================== Dglobal_escape ================ */

#define ISURIALNUM(c)       ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))

BUILTIN_FUNCTION2(Dglobal_, escape, 1)
{
    // ECMA 15.1.2.4
    Value *v;
    d_string s;
    dchar *p;
    d_string R;
    dchar *r;
    unsigned len;
    unsigned escapes;
    unsigned unicodes;
    unsigned c;
    unsigned i;

    if (argc)
	v = &arglist[0];
    else
	v = &vundefined;
    s = v->toString();
    len = d_string_len(s);
    p = d_string_ptr(s);
    escapes = 0;
    unicodes = 0;
    for (i = 0; i < len; i++)
    {
	c = p[i];
#if defined UNICODE
	if (c >= 0x100)
	    unicodes++;
	else
#endif
	if (c == 0 || (!ISURIALNUM(c) && !strchr("*@-_+./", c)))
	    escapes++;
    }
    if ((escapes + unicodes) == 0)
    {
	R = s;
    }
    else
    {
	R = Dstring::alloc(len + escapes * 2 + unicodes * 5);
	r = d_string_ptr(R);
	for (i = 0; i < len; i++)
	{
	    c = p[i];
	    if (c >= 0x100)
	    {
		r[0] = '%';
		r[1] = 'u';
		r[2] = TOHEX[(c >> 12) & 15];
		r[3] = TOHEX[(c >> 8) & 15];
		r[4] = TOHEX[(c >> 4) & 15];
		r[5] = TOHEX[c & 15];
		r += 6;
	    }
	    else if (c == 0 || (!ISURIALNUM(c) && !strchr("*@-_+./", c)))
	    {
		r[0] = '%';
		r[1] = TOHEX[c >> 4];
		r[2] = TOHEX[c & 15];
		r += 3;
	    }
	    else
	    {
		r[0] = (dchar)c;
		r++;
	    }
	}
    }
    Vstring::putValue(ret, R);
    return NULL;
}

/* ====================== Dglobal_unescape ================ */

BUILTIN_FUNCTION2(Dglobal_, unescape, 1)
{
    // ECMA 15.1.2.5
    Value *v;
    d_string string;
    dchar *s;
    d_string R;
    dchar *r;
    dchar c;
    unsigned len;
    unsigned k;

    if (argc)
	v = &arglist[0];
    else
	v = &vundefined;
    string = v->toString();
    len = d_string_len(string);
    s = d_string_ptr(string);
    R = Dstring::alloc(len);
    r = d_string_ptr(R);
    for (k = 0; k < len; k = Dchar::inc(s + k) - s)
    {
	c = Dchar::get(s + k);
	if (c == '%')
	{
	    if (k <= len - 6 &&
		s[k + 1] == 'u')
	    {	unsigned u;

		u = 0;
		for (int i = 2; ; i++)
		{   dchar x;

		    if (i == 6)
		    {
			c = (dchar) u;
			k += 5;
			break;
		    }
		    x = s[k + i];
		    if ('0' <= x && x <= '9')
			x = (dchar)(x - '0');
		    else if ('A' <= x && x <= 'F')
			x = (dchar)(x - 'A' + 10);
		    else if ('a' <= x && x <= 'f')
			x = (dchar)(x - 'a' + 10);
		    else
			break;
		    u = (u << 4) + x;
		}
	    }
	    else if (k <= len - 3)
	    {	unsigned u;

		u = 0;
		for (int i = 1; ; i++)
		{   dchar x;

		    if (i == 3)
		    {
			c = (dchar) u;
			k += 2;
			break;
		    }
		    x = s[k + i];
		    if ('0' <= x && x <= '9')
			x = (dchar)(x - '0');
		    else if ('A' <= x && x <= 'F')
			x = (dchar)(x - 'A' + 10);
		    else if ('a' <= x && x <= 'f')
			x = (dchar)(x - 'a' + 10);
		    else
			break;
		    u = (u << 4) + x;
		}
	    }
	}
	r = Dchar::put(r, c);
    }
    *r = 0;
    // Fix length property of R
    d_string_setLen(R, r - d_string_ptr(R));
    Vstring::putValue(ret, R);
    return NULL;
}

/* ====================== Dglobal_isNaN ================ */

BUILTIN_FUNCTION2(Dglobal_, isNaN, 1)
{
    // ECMA 15.1.2.6
    Value *v;
    d_number n;
    d_boolean b;

    if (argc)
	v = &arglist[0];
    else
	v = &vundefined;
    n = v->toNumber();
    b = Port::isnan(n) ? TRUE : FALSE;
    Vboolean::putValue(ret, b);
    return NULL;
}

/* ====================== Dglobal_isFinite ================ */

BUILTIN_FUNCTION2(Dglobal_, isFinite, 1)
{
    // ECMA 15.1.2.7
    Value *v;
    d_number n;
    d_boolean b;

    if (argc)
	v = &arglist[0];
    else
	v = &vundefined;
    n = v->toNumber();
    b = Port::isfinite(n) ? TRUE : FALSE;
    Vboolean::putValue(ret, b);
    return NULL;
}

/* ====================== Dglobal_ URI Functions ================ */

enum URIFLAGS
{
    URI_Alpha = 1,
    URI_Reserved = 2,
    URI_Mark = 4,
    URI_Digit = 8,
    URI_Hash = 0x10,		// '#'
};

unsigned char uri_flags[128];		// indexed by character

void uri_init_helper(char *p, enum URIFLAGS flags)
{
    for (; *p; p++)
	uri_flags[*p] |= flags;
}

void uri_init()
{
    // Initialize uri_flags[]
    uri_flags['#'] |= URI_Hash;

    for (int i = 'A'; i <= 'Z'; i++)
    {	uri_flags[i] |= URI_Alpha;
	uri_flags[i + 0x20] |= URI_Alpha;	// lowercase letters
    }
    uri_init_helper("0123456789", URI_Digit);
    uri_init_helper(";/?:@&=+$,", URI_Reserved);
    uri_init_helper("-_.!~*'()", URI_Mark);
}

// Hidden Encode() function, per ECMA 15.1.3

d_string URI_Encode(d_string string, unsigned unescapedSet)
{   unsigned len;
    dchar *s;
    unsigned j;
    unsigned k;
    unsigned V;
    unsigned C;
    dchar *R;
    unsigned Rlen;
    unsigned Rsize;	// alloc'd size
    dchar buffer[50];

    len = d_string_len(string);
    s = d_string_ptr(string);
    Rsize = len;
    R = buffer;
    Rsize = (sizeof(buffer) / sizeof(buffer[0])) - 1;
    Rlen = 0;
    for (k = 0; k != len; k++)
    {
	C = s[k];
	// if (C in unescapedSet)
	if (C < sizeof(uri_flags) && uri_flags[C] & unescapedSet)
	{
	    if (Rlen == Rsize)
	    {	dchar *R2;

		Rsize *= 2;
		R2 = (dchar *)alloca((Rsize + 1) * sizeof(dchar));
		assert(R2);
		memcpy(R2, R, Rlen * sizeof(dchar));
		R = R2;
	    }
	    R[Rlen] = (dchar)C;
	    Rlen++;
	}
	else
	{   unsigned char Octet[6];
	    unsigned L;

	    if (C >= 0xDC00 && C <= 0xDFFF)
		goto LthrowURIerror;
	    if (C < 0xD800 || C > 0xDBFF)
	    {
		V = C;
	    }
	    else
	    {	unsigned r13;

		k++;
		if (k == len)
		    goto LthrowURIerror;
		r13 = s[k];
		if (r13 < 0xDC00 || r13 > 0xDFFF)
		    goto LthrowURIerror;
		// UCS-2 to UCS-4 transformation
		V = (C - 0xD800) * 0x400 + (r13 - 0xDC00) + 0x10000;
	    }

	    // Transform V into octets (step 16)
	    // Description of UTF-8 at:
	    // http://www.cl.cam.ac.uk/~mgk25/unicode.html#utf-8
	    if (V <= 0x7F)
	    {
		Octet[0] = (unsigned char) V;
		L = 1;
	    }
	    else if (V <= 0x7FF)
	    {
		Octet[0] = (unsigned char)(0xC0 | (V >> 6));
		Octet[1] = (unsigned char)(0x80 | (V & 0x3F));
		L = 2;
	    }
	    else if (V <= 0xFFFF)
	    {
		Octet[0] = (unsigned char)(0xE0 | (V >> 12));
		Octet[1] = (unsigned char)(0x80 | ((V >> 6) & 0x3F));
		Octet[2] = (unsigned char)(0x80 | (V & 0x3F));
		L = 3;
	    }
	    else if (V <= 0x1FFFFF)
	    {
		Octet[0] = (unsigned char)(0xF0 | (V >> 18));
		Octet[1] = (unsigned char)(0x80 | ((V >> 12) & 0x3F));
		Octet[2] = (unsigned char)(0x80 | ((V >> 6) & 0x3F));
		Octet[3] = (unsigned char)(0x80 | (V & 0x3F));
		L = 4;
	    }
	    else if (V <= 0x3FFFFFF)
	    {	// This supposedly cannot happen in ECMA
		Octet[0] = (unsigned char)(0xF8 | (V >> 24));
		Octet[1] = (unsigned char)(0x80 | ((V >> 18) & 0x3F));
		Octet[2] = (unsigned char)(0x80 | ((V >> 12) & 0x3F));
		Octet[3] = (unsigned char)(0x80 | ((V >> 6) & 0x3F));
		Octet[4] = (unsigned char)(0x80 | (V & 0x3F));
		L = 5;
	    }
	    else if (V <= 0x7FFFFFFF)
	    {	// This supposedly cannot happen in ECMA
		Octet[0] = (unsigned char)(0xFC | (V >> 30));
		Octet[1] = (unsigned char)(0x80 | ((V >> 24) & 0x3F));
		Octet[2] = (unsigned char)(0x80 | ((V >> 18) & 0x3F));
		Octet[3] = (unsigned char)(0x80 | ((V >> 12) & 0x3F));
		Octet[4] = (unsigned char)(0x80 | ((V >> 6) & 0x3F));
		Octet[5] = (unsigned char)(0x80 | (V & 0x3F));
		L = 6;
	    }
	    else
	    {	assert(0);		// undefined UCS code
		L = 0;			// make /W4 work
	    }

	    if (Rlen + L * 3 >= Rsize)
	    {	dchar *R2;

		Rsize = 2 * (Rlen + L * 3);
		R2 = (dchar *)alloca((Rsize + 1) * sizeof(dchar));
		assert(R2);
		memcpy(R2, R, Rlen * sizeof(dchar));
		R = R2;
	    }

	    for (j = 0;;)
	    {
#if defined UNICODE
		SWPRINTF(R + Rlen, Rsize, L"%%%02X", Octet[j]);
#else
		sprintf(R + Rlen, "%%%02X", Octet[j]);
#endif
		Rlen += 3;
		j++;
		if (j == L)
		    break;
	    }
	}
    }
    R[Rlen] = 0;		// terminate string
  {
    Mem mem;
    GC_LOG();
    return Dstring::dup(&mem, R);
  }

LthrowURIerror:
    return d_string_null;
}

// Hidden Decode() function, per ECMA 15.1.3

unsigned ascii2hex(dchar c)
{
    return (c <= '9') ? c - '0' :
	   (c <= 'F') ? c - 'A' + 10 :
			c - 'a' + 10;
}

d_string URI_Decode(d_string string, unsigned reservedSet)
{   unsigned len;
    unsigned j;
    unsigned k;
    unsigned V;
    unsigned C;
    dchar *R;
    dchar *s;
    unsigned Rlen;
    unsigned Rsize;	// alloc'd size

    len = d_string_len(string);
    s = d_string_ptr(string);

#if LOG
    WPRINTF(L"URI_Decode(s = '%s', len = %d)\n", s, len);
#endif

    // Preallocate result buffer R guaranteed to be large enough for result
    Rsize = len * 2;
    R = (dchar *)alloca((Rsize + 1) * sizeof(dchar));
    assert(R);
    Rlen = 0;

    for (k = 0; k != len; k++)
    {	unsigned char B;
	unsigned start;

	C = s[k];
	if (C != '%')
	{   R[Rlen] = (dchar)C;
	    Rlen++;
	    continue;
	}
	start = k;
	if (k + 2 >= len)
	    goto LthrowURIerror;
	if (!isxdigit(s[k + 1]) || !isxdigit(s[k + 2]))
	    goto LthrowURIerror;
	B = (unsigned char)((ascii2hex(s[k + 1]) << 4) + ascii2hex(s[k + 2]));
	k += 2;
	if ((B & 0x80) == 0)
	{
	    C = B;
	}
	else
	{   unsigned n;

	    for (n = 1; ; n++)
	    {
		if (n > 4)
		    goto LthrowURIerror;
		if (((B << n) & 0x80) == 0)
		{
		    if (n == 1)
			goto LthrowURIerror;
		    break;
		}
	    }

	    // Pick off (7 - n) significant bits of B from first byte of octet
	    V = B & ((1 << (7 - n)) - 1);	// (!!!)

	    if (k + (3 * (n - 1)) >= len)
		goto LthrowURIerror;
	    for (j = 1; j != n; j++)
	    {
		k++;
		if (s[k] != '%')
		    goto LthrowURIerror;
		if (!isxdigit(s[k + 1]) || !isxdigit(s[k + 2]))
		    goto LthrowURIerror;
		B = (unsigned char)((ascii2hex(s[k + 1]) << 4) + ascii2hex(s[k + 2]));
		if ((B & 0xC0) != 0x80)
		    goto LthrowURIerror;
		k += 2;
		V = (V << 6) | (B & 0x3F);
	    }
	    if (V >= 0x10000)
	    {	unsigned H;
		unsigned L;

		if (V > 0x10FFFF)
		    goto LthrowURIerror;
		L = ((V - 0x10000) & 0x3FF) + 0xDC00;
		H = (((V - 0x10000) >> 10) & 0x3FF) + 0xD800;
		R[Rlen] = (dchar)H;
		R[Rlen + 1] = (dchar)L;
		Rlen += 2;
		continue;
	    }
	    C = V;
	}
	if (C < sizeof(uri_flags) && uri_flags[C] & reservedSet)
	{
	    // R += s[start .. k];	// inclusive
	    memcpy(R + Rlen, s + start, ((k + 1) - start) * sizeof(dchar));
	    Rlen += (k + 1) - start;
	}
	else
	{
	    R[Rlen] = (dchar)C;
	    Rlen++;
	}
    }
    assert(Rlen <= Rsize);	// enforce our preallocation size guarantee
#if LOG
    WPRINTF(L"Rlen = %d\n", Rlen);
#endif
  {
    Mem mem;
    GC_LOG();
    d_string d;
    d = Lstring::alloc(&mem, Rlen);
    memcpy(d_string_ptr(d), R, Rlen * sizeof(dchar));
    return d;
  }

LthrowURIerror:
#if LOG
    WPRINTF(L"LthrowURIerror\n");
#endif
    return d_string_null;
}

BUILTIN_FUNCTION2(Dglobal_, decodeURI, 1)
{
    // ECMA v3 15.1.3.1
    Value *encodedURI;
    d_string s;

#if LOG
    FuncLog funclog(L"decodeURI()");
#endif

    encodedURI = argc ? &arglist[0] : &vundefined;
    s = encodedURI->toString();
    s = URI_Decode(s, URI_Reserved | URI_Hash);
    if (s)
    {
	Vstring::putValue(ret, s);
	return NULL;
    }
    else
    {	Dobject *o = new(cc) Durierror(DTEXT("decodeURI() failure"));
	Value::copy(ret, &vundefined);
	return new(cc) Vobject(o);
    }
}

BUILTIN_FUNCTION2(Dglobal_, decodeURIComponent, 1)
{
    // ECMA v3 15.1.3.2
    Value *encodedURIComponent;
    d_string s;

#if LOG
    FuncLog funclog(L"decodeURIComponent()");
#endif
    encodedURIComponent = argc ? &arglist[0] : &vundefined;
    s = encodedURIComponent->toString();
    s = URI_Decode(s, 0);
    if (s)
    {
	Vstring::putValue(ret, s);
	return NULL;
    }
    else
    {
	Dobject *o = new(cc)
	    Durierror(DTEXT("decodeURIComponent() failure"));
	Value::copy(ret, &vundefined);
	return new(cc) Vobject(o);
    }
}

BUILTIN_FUNCTION2(Dglobal_, encodeURI, 1)
{
    // ECMA v3 15.1.3.3
    Value *uri;
    d_string s;

    uri = argc ? &arglist[0] : &vundefined;
    s = uri->toString();
    s = URI_Encode(s, URI_Reserved | URI_Hash | URI_Alpha | URI_Digit | URI_Mark);
    if (d_string_ptr(s))
    {
	Vstring::putValue(ret, s);
	return NULL;
    }
    else
    {	Dobject *o = new(cc) Durierror(DTEXT("encodeURI() failure"));
	Value::copy(ret, &vundefined);
	return new(cc) Vobject(o);
    }
}

BUILTIN_FUNCTION2(Dglobal_, encodeURIComponent, 1)
{
    // ECMA v3 15.1.3.4
    Value *uriComponent;
    d_string s;

    uriComponent = argc ? &arglist[0] : &vundefined;
    s = uriComponent->toString();
    s = URI_Encode(s, URI_Alpha | URI_Digit | URI_Mark);
    if (d_string_ptr(s))
    {
	Vstring::putValue(ret, s);
	return NULL;
    }
    else
    {	Dobject *o = new(cc) Durierror(DTEXT("encodeURIComponent() failure"));
	Value::copy(ret, &vundefined);
	return new(cc) Vobject(o);
    }
}

/* ====================== Dglobal_print ================ */

static void dglobal_print(CALL_ARGS)
{
    // Our own extension
    if (argc)
    {	unsigned i;

	for (i = 0; i < argc; i++)
	{
	    d_string s = arglist[i].toString();
	    dchar *p = d_string_ptr(s);

#if 1
#if defined UNICODE
	    WPRINTF(L"%ls", p);
#else
	    PRINTF("%s", p);
#endif
#else
	    int len = d_string_len(s);

	    for (int j = 0; j < len; j++)
	    {
#if defined UNICODE
		WPRINTF(L"%lc", p[j]);
#else
		PRINTF("%c", p[j]);
#endif
	    }
#endif
	}
    }

    Value::copy(ret, &vundefined);
}

BUILTIN_FUNCTION2(Dglobal_, print, 1)
{
    // Our own extension
    dglobal_print(cc, othis, ret, argc, arglist);
    return NULL;
}

/* ====================== Dglobal_println ================ */

BUILTIN_FUNCTION2(Dglobal_, println, 1)
{
    // Our own extension
    dglobal_print(cc, othis, ret, argc, arglist);
#if defined UNICODE
    WPRINTF(L"\n");
#else
    PRINTF("\n");
#endif
    return NULL;
}

/* ====================== Dglobal_readln ================ */

BUILTIN_FUNCTION2(Dglobal_, readln, 0)
{
    // Our own extension
    wint_t c;
    OutBuffer buf;
    dchar *s;

    for (;;)
    {
#if linux
	c = getchar();
	if (c == EOF)
	    break;
#elif _WIN32
	c = _fgetwchar();
	if (c == WEOF)
	    break;
#endif
	if (c == '\n')
	    break;
	buf.writedchar(c);
    }
    buf.writedchar(0);
    s = (dchar *)buf.data;
    buf.data = NULL;
    Vstring::putValue(ret, d_string_ctor(s));
    return NULL;
}

/* ====================== Dglobal_getenv ================ */

BUILTIN_FUNCTION2(Dglobal_, getenv, 1)
{
    // Our own extension
    Value::copy(ret, &vundefined);
    if (argc)
    {
	d_string s = arglist[0].toString();
	dchar *p = d_string_ptr(s);
	p = _wgetenv(p);
	if (p)
	{
	    Vstring::putValue(ret, p);
	}
	else
	    Value::copy(ret, &vnull);
    }
    return NULL;
}

/* ====================== Dglobal_assert ================ */

#if 0	// Now handled by AssertExp()

BUILTIN_FUNCTION(Dglobal_, assert, 1)
{
    // Our own extension
    Value::copy(ret, &vundefined);
    if (argc)
    {
	if (!arglist[0].toBoolean())
	    return RuntimeError(DTEXT("assert() failure"));
    }
    else
	return RuntimeError(DTEXT("assert() failure"));
    return NULL;
}

#endif

/* ====================== Dglobal_ScriptEngine ================ */

BUILTIN_FUNCTION2(Dglobal_, ScriptEngine, 0)
{
    Vstring::putValue(ret, TEXT_CHROMEscript);
    return NULL;
}

BUILTIN_FUNCTION2(Dglobal_, ScriptEngineBuildVersion, 0)
{
#if 1
#ifdef linux
    char *p = __DATE__;
#else
    char *p = __TIMESTAMP__;
#endif
    d_string s;

    s = Dstring::dup(pthis, p);
    Vstring::putValue(ret, s);
#else
    Vnumber::putValue(ret, BUILD_VERSION);
#endif
    return NULL;
}

BUILTIN_FUNCTION2(Dglobal_, ScriptEngineMajorVersion, 0)
{
    Vnumber::putValue(ret, MAJOR_VERSION);
    return NULL;
}

BUILTIN_FUNCTION2(Dglobal_, ScriptEngineMinorVersion, 0)
{
    Vnumber::putValue(ret, MINOR_VERSION);
    return NULL;
}

/* ====================== Dglobal =========================== */

Dglobal::Dglobal(int argc, char *argv[])
    : Dobject(Dobject::getPrototype())	// Dglobal.prototype is implementation-dependent
{
    //WPRINTF(L"Dglobal::Dglobal(%x)\n", this);
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);

    uri_init();

    Dobject *f = Dfunction::getPrototype();

    // Using new(m) instead of new(this) saves the NULL check for this
    Mem *m = this;

    classname = TEXT_global;

    // ECMA 15.1
    // Add in built-in objects which have attribute { DontEnum }

    // Value properties

    Put(TEXT_NaN, Port::nan, DontEnum);
    Put(TEXT_Infinity, Port::infinity, DontEnum);

    #define X(name,len) { &TEXT_##name, #name, Dglobal_##name, len }
    static NativeFunctionData nfd[] =
    {
    // Function properties
    X(eval, 1),
    X(parseInt, 2),
    X(parseFloat, 1),
    X(escape, 1),
    X(unescape, 1),
    X(isNaN, 1),
    X(isFinite, 1),
    X(decodeURI, 1),
    X(decodeURIComponent, 1),
    X(encodeURI, 1),
    X(encodeURIComponent, 1),

    // Dscript unique function properties
    X(print, 1),
    X(println, 1),
    X(readln, 0),
    X(getenv, 1),

    // Jscript compatible extensions
    X(ScriptEngine, 0),
    X(ScriptEngineBuildVersion, 0),
    X(ScriptEngineMajorVersion, 0),
    X(ScriptEngineMinorVersion, 0),
    };

    DnativeFunction::init(this, nfd, sizeof(nfd) / sizeof(nfd[0]), DontEnum);

    // Now handled by AssertExp()
    // Put(TEXT_assert, tc->Dglobal_assert(), DontEnum);

    // Constructor properties

    Put(TEXT_Object,         tc->Dobject_constructor, DontEnum);
    Put(TEXT_Function,       tc->Dfunction_constructor, DontEnum);
    Put(TEXT_Array,          tc->Darray_constructor, DontEnum);
    Put(TEXT_String,         tc->Dstring_constructor, DontEnum);
    Put(TEXT_Boolean,        tc->Dboolean_constructor, DontEnum);
    Put(TEXT_Number,         tc->Dnumber_constructor, DontEnum);
    Put(TEXT_Date,           tc->Ddate_constructor, DontEnum);
    Put(TEXT_RegExp,         tc->Dregexp_constructor, DontEnum);
    Put(TEXT_Error,          tc->Derror_constructor, DontEnum);
    Put(TEXT_EvalError,      tc->Devalerror_constructor, DontEnum);
    Put(TEXT_RangeError,     tc->Drangeerror_constructor, DontEnum);
    Put(TEXT_ReferenceError, tc->Dreferenceerror_constructor, DontEnum);
    Put(TEXT_SyntaxError,    tc->Dsyntaxerror_constructor, DontEnum);
    Put(TEXT_TypeError,      tc->Dtypeerror_constructor, DontEnum);
    Put(TEXT_URIError,       tc->Durierror_constructor, DontEnum);

    // Other properties

    assert(tc->Dmath_object);
    Put(TEXT_Math, tc->Dmath_object, DontEnum);

    // Build an "arguments" property out of argc and argv,
    // and add it to the global object.
    Darray *arguments;

    GC_LOG();
    arguments = new(m) Darray();
    Put(TEXT_arguments, arguments, DontDelete);
    arguments->length = argc;
    for (int i = 0; i < argc; i++)
    {
	GC_LOG();
	arguments->Put(i, Dstring::dup(m, argv[i]), DontEnum);
    }
    arguments->Put(TEXT_callee, &vnull, DontEnum);

    // Add in GetObject and ActiveXObject
    Dcomobject_addCOM(this);

    Denumerator_add(this);
    Dvbarray_add(this);
}

