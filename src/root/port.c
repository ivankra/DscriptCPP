/* Digital Mars DMDScript source code.
 * Copyright (c) 2000-2001 by Chromium Communications
 * Copyright (c) 2004-2007 by Digital Mars
 * All Rights Reserved, written by Walter Bright
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

#include "port.h"
#include <stdio.h>

#if defined __DMC__
#include <math.h>
#include <float.h>
#include <fp.h>
#include <time.h>
#include <stdlib.h>

double Port::nan = NAN;
double Port::infinity = INFINITY;
double Port::dbl_max = DBL_MAX;
double Port::dbl_min = DBL_MIN;

int Port::isnan(double r)
{
    return ::isnan(r);
}

int Port::isfinite(double r)
{
    return ::isfinite(r);
}

int Port::isinfinity(double r)
{
    return (::fpclassify(r) == FP_INFINITE);
}

int Port::signbit(double r)
{
    return ::signbit(r);
}

double Port::floor(double d)
{
    return ::floor(d);
}

double Port::round(double d)
{
    if (!Port::isnan(d))
    {
	d = copysign(floor(d + .5), d);
    }
    return d;
}

double Port::exp(double x)
{
    return ::exp(x);
}

double Port::pow(double x, double y)
{
    return ::pow(x, y);
}

double Port::atan2(double x, double y)
{
    return ::atan2(x, y);
}

unsigned long long Port::strtoull(const char *p, char **pend, int base)
{
    return ::strtoull(p, pend, base);
}

char *Port::ull_to_string(char *buffer, ulonglong ull)
{
    sprintf(buffer, "%llu", ull);
    return buffer;
}

wchar_t *Port::ull_to_string(wchar_t *buffer, ulonglong ull)
{
    _swprintf(buffer, L"%llu", ull);
    return buffer;
}

double Port::ull_to_double(ulonglong ull)
{
    return (double) ull;
}

#if defined UNICODE
void Port::list_separator(unsigned long lcid, wchar_t buffer[5])
{
    buffer[0] = ',';
    buffer[1] = 0;
}
#else
void Port::list_separator(unsigned long lcid, char buffer[5])
{
    buffer[0] = ',';
    buffer[1] = 0;
}
#endif

char *Port::itoa(int val, char *s, int rad)
{
    return _itoa(val, s, rad);
}

wchar_t *Port::itow(int val, wchar_t *s, int rad)
{
#if 0
    int i, neg;
    wchar_t *p;
    wchar_t *q;

    /* a few early bailouts */
    if(!s)
    {
	return (wchar_t*)0;
    }

    if((rad < 2) || (rad > 36))
    {
	*s = 0;
	return s;
    }

    if(val == 0)
    {
	s[0] = '0';
	s[1] = '\0';
	return s;
    }

    neg = 0;
    if (val < 0 && rad == 10)	// only provides a negative value for base 10
    {
	neg = 1;
	val = -val;
    }

    /*
    * write the digits from high to low initially, we'll reverse the string at
    * the end.
    */
    p = s;
    while (val)
    {
	i = (unsigned)val % rad;
	if(i > 9)
	{
	    i += 7;
	}
	*p++ = '0' + i;
	val = (unsigned)val / rad;
    }

    if (neg)
    {
	*p++ = '-';
    }

    *p = '\0';

    /* OK, time to reverse */
    q = s;
    while (--p > q)
    {	wchar_t c;

	c = *q;
	*q = *p;
	*p = c;
	q++;
    }

    return s;
#else
    return ::_itow(val, s, rad);
#endif
}

#endif // __DMC__

#if _MSC_VER

// Disable useless warnings about unreferenced functions
#pragma warning (disable : 4514)

#include <math.h>
#include <float.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

static unsigned long nanarray[2] = {0,0x7FF80000 };
double Port::nan = (*(double *)nanarray);

//static unsigned long infinityarray[2] = {0,0x7FF00000 };
static double zero = 0;
double Port::infinity = 1 / zero;

double Port::dbl_max = DBL_MAX;
double Port::dbl_min = DBL_MIN;

#define PI		3.14159265358979323846

#if 0
int Port::isnan(double r)
{
    //return ::_isnan(r);	// very slow
    unsigned long *p = (unsigned long *)&r;
    return (p[1] & 0x7FF00000) == 0x7FF00000 && (p[1] & 0xFFFFF || p[0]);
}
#endif

int Port::isfinite(double r)
{
    return ::_finite(r);
}

int Port::isinfinity(double r)
{
    return (::_fpclass(r) & (_FPCLASS_NINF | _FPCLASS_PINF));
}

int Port::signbit(double r)
{
    return (long)(((long *)&(r))[1] & 0x80000000);
}

double Port::floor(double d)
{
    return ::floor(d);
}

double Port::round(double d)
{
    if (!Port::isnan(d))
    {
	d = _copysign(floor(d + .5), d);
    }
    return d;
}

double Port::exp(double x)
{
    return ::exp(x);
}

double Port::pow(double x, double y)
{
    // Check for nan *first*, because other comparison operators produce
    // incorrect answers for nan's
    if (::_isnan(y))
	return Port::nan;

    if (y == 0)
	return 1;		// even if x is NAN
    return ::pow(x, y);
}

double Port::atan2(double x, double y)
{   double result;
    static double zero = 0.0;

    if (isinfinity(x) && isinfinity(y))
    {
	result = signbit(y) ? 3 * PI / 4 : PI / 4;
	if (signbit(x))
	{
	    result = -result;
	}
    }
    else if (x == 0 && signbit(x) && ::_finite(y) && !signbit(y))
    {
	//printf("0: x%08lx, %08lx\n", ((long *)&x)[1],((long *)&x)[0]);
	result = -zero;			// necessary to get -0, not +0
    }
    else
    {
	//printf("x%08lx, %08lx\n", ((long *)&x)[1],((long *)&x)[0]);
	result = ::atan2(x, y);
    }
    return result;
}

unsigned _int64 Port::strtoull(const char *p, char **pend, int base)
{
    unsigned _int64 number = 0;
    int c;
    int error;
    #define ULLONG_MAX ((unsigned _int64)~0I64)

    while (isspace(*p))		/* skip leading white space	*/
	p++;
    if (*p == '+')
	p++;
    switch (base)
    {   case 0:
	    base = 10;		/* assume decimal base		*/
	    if (*p == '0')
	    {   base = 8;	/* could be octal		*/
		    p++;
		    switch (*p)
		    {   case 'x':
			case 'X':
			    base = 16;	/* hex			*/
			    p++;
			    break;
#if BINARY
			case 'b':
			case 'B':
			    base = 2;	/* binary		*/
			    p++;
			    break;
#endif // BINARY
		    }
	    }
	    break;
	case 16:			/* skip over '0x' and '0X'	*/
	    if (*p == '0' && (p[1] == 'x' || p[1] == 'X'))
		    p += 2;
	    break;
#if BINARY
	case 2:			/* skip over '0b' and '0B'	*/
	    if (*p == '0' && (p[1] == 'b' || p[1] == 'B'))
		    p += 2;
	    break;
#endif
    }
    error = 0;
    for (;;)
    {   c = *p;
	if (('0' <= (c) && (c) <= '9'))
		c -= '0';
	else if (('A' <= (c & ~0x20) && (c & ~0x20) <= 'Z'))
		c = (c & ~0x20) - ('A' - 10);
	else			/* unrecognized character	*/
		break;
	if (c >= base)		/* not in number base		*/
		break;
	if ((ULLONG_MAX - c) / base < number)
		error = 1;
	number = number * base + c;
	p++;
    }
    if (pend)
	*pend = (char *)p;
    if (error)
    {   number = ULLONG_MAX;
	errno = ERANGE;
    }
    return number;
}

char *Port::ull_to_string(char *buffer, ulonglong ull)
{
    _ui64toa(ull, buffer, 10);
    return buffer;
}

wchar_t *Port::ull_to_string(wchar_t *buffer, ulonglong ull)
{
    _ui64tow(ull, buffer, 10);
    return buffer;
}

double Port::ull_to_double(ulonglong ull)
{   double d;

    if ((__int64) ull < 0)
    {
	// MSVC doesn't implement the conversion
	d = (double) (__int64)(ull -  0x8000000000000000i64);
	d += (double)(signed __int64)(0x7FFFFFFFFFFFFFFFi64) + 1.0;
    }
    else
	d = (double)(__int64)ull;
    return d;
}

#if defined UNICODE
void Port::list_separator(unsigned long lcid, wchar_t buffer[5])
{
    if (lcid == 0 ||
        GetLocaleInfo(lcid, LOCALE_SLIST, buffer, 5) == 0)
    {	buffer[0] = ',';
	buffer[1] = 0;
    }
}
#else
void Port::list_separator(unsigned long lcid, char buffer[5])
{
    buffer[0] = ',';
    buffer[1] = 0;
}
#endif


char *Port::itoa(int val, char *s, int rad)
{
    return _itoa(val, s, rad);
}

wchar_t *Port::itow(int val, wchar_t *s, int rad)
{
    return _itow(val, s, rad);
}


#endif // _MSC_VER

// --------------------------------------------------------
//
// Linux Version
//
// --------------------------------------------------------
#if defined linux

#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "wchar.h"

#if defined _WIN32
#include <windows.h>	// CHILICOM
#endif

#include "printf.h"	// for SWPRINTF

static unsigned long nanarray[2] = {0,0x7FF80000 };
double Port::nan = (*(double *)nanarray);

static double zero = 0;
double Port::infinity = 1 / zero;
double Port::dbl_max = 1.7976931348623157e308;
double Port::dbl_min = 5e-324;

#undef isnan
int Port::isnan(double r)
{
    return ::isnan(r);
}

#undef isfinite
int Port::isfinite(double r)
{
    return ::finite(r);
}

#undef isinf
int Port::isinfinity(double r)
{
    return ::isinf(r);
}

#undef signbit
int Port::signbit(double r)
{
    return (long)(((long *)&(r))[1] & 0x80000000);
}

double Port::floor(double d)
{
    return ::floor(d);
}

double Port::round(double d)
{
    if (!Port::isnan(d))
    {
	d = copysign(floor(d + .5), d);
    }
    return d;
}

double Port::exp(double x)
{
    if (::isinf(x))
    {
	if (signbit(x))
	    return 0;
	else
	    return Port::infinity;
    }
    return ::exp(x);
}

double Port::pow(double x, double y)
{
    // Check for nan *first*, because other comparison operators produce
    // incorrect answers for nan's
    if (::isnan(y))
	return Port::nan;

    if (y == 0)
	return 1;		// even if x is NAN
    if (::isnan(x) && y != 0)
	return Port::nan;
    if (::isinf(y))
    {
	if (fabs(x) > 1)
	{
	    if (signbit(y))
		return +0.0;
	    else
		return Port::infinity;
	}
	else if (fabs(x) == 1)
	{
	    return Port::nan;
	}
	else // < 1
	{
	    if (signbit(y))
		return Port::infinity;
	    else
		return +0.0;
	}
    }
    if (::isinf(x))
    {
	if (signbit(x))
	{   longlong i;

	    i = (longlong)y;
	    if (y > 0)
	    {
		if (i == y && i & 1)
		{
		    return -Port::infinity;
		}
		else
		{
		    return Port::infinity;
		}
	    }
	    else if (y < 0)
	    {
		if (i == y && i & 1)
		{
		    return -0.0;
		}
		else
		{
		    return +0.0;
		}
	    }
	}
	else
	{
	    if (y > 0)
		return Port::infinity;
	    else if (y < 0)
		return +0.0;
	}
    }

    if (x == 0.0)
    {
	if (signbit(x))
	{   longlong i;

	    i = (longlong)y;
	    if (y > 0)
	    {
		if (i == y && i & 1)
		{
		    return -0.0;
		}
		else
		{
		    return +0.0;
		}
	    }
	    else if (y < 0)
	    {
		if (i == y && i & 1)
		{
		    return -Port::infinity;
		}
		else
		{
		    return Port::infinity;
		}
	    }
	}
	else
	{
	    if (y > 0)
		return +0.0;
	    else if (y < 0)
		return Port::infinity;
	}
    }

    return ::pow(x, y);
}


double Port::atan2(double x, double y)
{
    return ::atan2(x, y);
}

unsigned long long Port::strtoull(const char *p, char **pend, int base)
{
    return ::strtoull(p, pend, base);
}

char *Port::ull_to_string(char *buffer, ulonglong ull)
{
    sprintf(buffer, "%llu", ull);
    return buffer;
}

wchar_t *Port::ull_to_string(wchar_t *buffer, ulonglong ull)
{
    SWPRINTF(buffer, 20, L"%llu", ull);
    return buffer;
}

double Port::ull_to_double(ulonglong ull)
{
    return (double) ull;
}

#if defined UNICODE
void Port::list_separator(unsigned long lcid, wchar_t buffer[5])
{
#if defined _WIN32
    if (lcid == 0 ||
        GetLocaleInfo(lcid, LOCALE_SLIST, buffer, 5) == 0)
#endif
    {	buffer[0] = ',';
	buffer[1] = 0;
    }
}
#else
void Port::list_separator(unsigned long lcid, char buffer[5])
{
    buffer[0] = ',';
    buffer[1] = 0;
}
#endif


char *Port::itoa(int val, char *s, int rad)
{
    int i, neg;
    char *p;
    char *q;

    /* a few early bailouts */
    if(!s)
    {
	return (char*)0;
    }

    if((rad < 2) || (rad > 36))
    {
	*s = 0;
	return s;
    }

    if(val == 0)
    {
	s[0] = '0';
	s[1] = '\0';
	return s;
    }

    neg = 0;
    if (val < 0 && rad == 10)	// only provides a negative value for base 10
    {
	neg = 1;
	val = -val;
    }

    /*
    * write the digits from high to low initially, we'll reverse the string at
    * the end.
    */
    p = s;
    while (val)
    {
	i = (unsigned)val % rad;
	if(i > 9)
	{
	    i += 7;
	}
	*p++ = '0' + i;
	val = (unsigned)val / rad;
    }

    if (neg)
    {
	*p++ = '-';
    }

    *p = '\0';

    /* OK, time to reverse */
    q = s;
    while (--p > q)
    {	char c;

	c = *q;
	*q = *p;
	*p = c;
	q++;
    }

    return s;
}

wchar_t *Port::itow(int val, wchar_t *s, int rad)
{
    int i, neg;
    wchar_t *p;
    wchar_t *q;

    /* a few early bailouts */
    if(!s)
    {
	return (wchar_t*)0;
    }

    if((rad < 2) || (rad > 36))
    {
	*s = 0;
	return s;
    }

    if(val == 0)
    {
	s[0] = '0';
	s[1] = '\0';
	return s;
    }

    neg = 0;
    if (val < 0 && rad == 10)	// only provides a negative value for base 10
    {
	neg = 1;
	val = -val;
    }

    /*
    * write the digits from high to low initially, we'll reverse the string at
    * the end.
    */
    p = s;
    while (val)
    {
	i = (unsigned)val % rad;
	if(i > 9)
	{
	    i += 7;
	}
	*p++ = '0' + i;
	val = (unsigned)val / rad;
    }

    if (neg)
    {
	*p++ = '-';
    }

    *p = '\0';

    /* OK, time to reverse */
    q = s;
    while (--p > q)
    {	wchar_t c;

	c = *q;
	*q = *p;
	*p = c;
	q++;
    }

    return s;
}

ulonglong Port::read_timer()
{
#if 0	// This works with GCC 2.91.66, but not GCC 2.95.3
    ulonglong r;

    __asm__ __volatile__("rdtsc\n" 
	: "=A" (r) 
	: /* */ 
	: "ax", "dx"); 
    return r; 
#else
    // Alternate version if the other one doesn't compile

    #define rdtsc(low, high) \
	     __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high))

    unsigned long low, high;

    rdtsc(low, high);

    return ((ulonglong)high << 32) | low;
#endif
}


#endif // linux
