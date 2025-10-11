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

#if defined linux
#include <wctype.h>
#endif

#include "dchar.h"
#include "mem.h"
#include "port.h"
#include "root.h"
#include "dscript.h"
#include "dobject.h"
#include "text.h"

#if 0
d_string d_string::tod_string(const dchar *p)
{   d_string s;

    s.p = (dchar *)p;
    return s;
}

int d_string::cmp(d_string s1, d_string s2)
{
    return Dchar::cmp(d_string_ptr(s1), d_string_ptr(s2));
}
#endif

d_string Dstring::dup(Mem *mem, d_string s)
{
    d_uint32 length;
    d_string d;

    length = d_string_len(s);
    d = Lstring::alloc(mem, length);
    memcpy(d_string_ptr(d), d_string_ptr(s), length * sizeof(dchar));

#ifdef DEBUG
    assert(cmp(d, s) == 0);
#endif
    return d;
}

#if !ASCIZ
d_string Dstring::dup(Mem *mem, dchar *p)
{
    d_uint32 length;
    d_string d;

    length = Dchar::len(p);
    d = Lstring::alloc(mem, length);
    memcpy(d_string_ptr(d), p, length * sizeof(dchar));

#ifdef DEBUG
    assert(Dchar::cmp(d_string_ptr(d), p) == 0);
#endif
    return d;
}
#endif

#if defined UNICODE

d_string Dstring::dup(Mem *mem, char *p)
{
    d_uint32 length;
    d_string d;
    d_uint32 i;

    length = strlen(p);
    d = Lstring::alloc(mem, length);
    for (i = 0; i < length; i++)
	d_string_ptr(d)[i] = (dchar)(p[i] & 0xFF);
    return d;
}

#endif

d_string Dstring::dup2(Mem *mem, d_string s1, d_string s2)
{
    d_uint32 length1;
    d_uint32 length2;
    d_string d;

    //WPRINTF(L"Dstring::dup2(%d '%ls', %d '%ls')\n", d_string_len(s1), d_string_ptr(s1), d_string_len(s2), d_string_ptr(s2));
    length1 = d_string_len(s1);
    length2 = d_string_len(s2);
    GC_LOG();
    d = Lstring::alloc(mem, length1 + length2);
    memcpy(d_string_ptr(d), d_string_ptr(s1), length1 * sizeof(dchar));
    memcpy(d_string_ptr(d) + length1, d_string_ptr(s2), length2 * sizeof(dchar));
    //WPRINTF(L"  result = %d '%ls'\n", d_string_len(d), d_string_ptr(d));
    return d;
}

d_string Dstring::substring(d_string string, int start, int end)
{   d_string s;

    if (start == end)
	s = TEXT_;
    else
    {	unsigned len;

	len = end - start;
	GC_LOG();
	s = alloc(len);
	memcpy(d_string_ptr(s), d_string_ptr(string) + start, len * sizeof(dchar));
    }
    return s;
}

#if !ASCIZ
d_string Dstring::substring(dchar *string, int start, int end)
{   d_string s;

    if (start == end)
	s = TEXT_;
    else
    {	unsigned len;

	len = end - start;
	GC_LOG();
	s = alloc(len);
	memcpy(d_string_ptr(s), string + start, len * sizeof(dchar));
    }
    return s;
}
#endif

int Dstring::cmp(d_string s1, d_string s2)
{
    return Dchar::cmp(d_string_ptr(s1), d_string_ptr(s2));
}

d_string Dstring::alloc(d_uint32 length)
{
    return Lstring::alloc(length);
}

d_uint32 Dstring::len(d_string s)
{
    return d_string_len(s);
}

int Dstring::isStrWhiteSpaceChar(dchar c)
{
    switch (c)
    {
	case ' ':
	case '\t':
	case 0xA0:	// <NBSP>
	case '\f':
	case '\v':
	case '\r':
	case '\n':
#if defined UNICODE
	case 0x2028:	// <LS>
	case 0x2029:	// <PS>
#endif
#if defined linux
	//case 0x2000:	// Microsoft apparently doesn't recognize this as whitespace
	case 0x2001:	// <USP> (iswspace() doesn't work well under linux)
#endif
	    return 1;

	case 0:
	    break;

	default:
	    if (c > 0xFF && iswspace(c))		// <USP>
		return 1;
	    break;
    }
    return 0;
}

d_number Dstring::toNumber(d_string string, dchar **p_endptr)
{
    // Convert StringNumericLiteral using ECMA 9.3.1
    d_number number;
    dchar *s;
    dchar *sstart;
    dchar *endptr;
    int sign = 0;
    unsigned c;
    unsigned len;

    s = d_string_ptr(string);
    len = d_string_len(string);

    // Leading whitespace
    while (Dstring::isStrWhiteSpaceChar(*s))
	s++;

    // Check for [+|-]
    sstart = s;
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
	number = sign ? -Port::infinity : Port::infinity;
	endptr = s + 8;		// 8 chars in "Infinity"
    }
    else if (*s == '0' && (s[1] == 'x' || s[1] == 'X'))
    {
	// Check for 0[x|X]HexDigit...
	endptr = NULL;			// necessary for /W4
	number = 0;
	s += 2;
	for (;;)
	{
	    c = *s;		// don't need Dchar::get(s);
	    if ('0' <= c && c <= '9')
		number = number * 16 + (c - '0');
	    else if ('a' <= c && c <= 'f')
		number = number * 16 + (c - 'a' + 10);
	    else if ('A' <= c && c <= 'F')
		number = number * 16 + (c - 'A' + 10);
	    else
	    {
		if (sign)
		    number = -number;
		endptr = s;
		break;
	    }
	    s++;		// don't need Dchar::inc(s)
	}
    }
    else
    {
#if defined UNICODE
	number = wcstod(s, &endptr);
#else
	number = strtod(s, &endptr);
#endif
	// Microsoft's wcstod() does not correctly produce a -0 for the
	// string "-1e-2000", so we handle it ourselves
	if (sign)
	    number = -number;
	if (endptr == s && s != sstart)
	    number = Port::nan;
    }

    if (p_endptr)
	*p_endptr = endptr;

    return number;
}


