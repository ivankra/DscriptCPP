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


#ifndef PORT_H
#define PORT_H

// Portable wrapper around compiler/system specific things.
// The idea is to minimize #ifdef's in the app code.

#ifndef TYPEDEFS
#define TYPEDEFS

#include <wchar.h>

#if _MSC_VER
typedef __int64 longlong;
typedef unsigned __int64 ulonglong;
#else
typedef long long longlong;
typedef unsigned long long ulonglong;
#endif

#endif

typedef double d_time;

struct Port
{
    static double nan;
    static double infinity;
    static double dbl_max;
    static double dbl_min;

#undef isnan
#undef isfinite
#undef signbit

#if _MSC_VER
    static int Port::isnan(double r)
    {
	unsigned long *p = (unsigned long *)&r;
	return (p[1] & 0x7FF00000) == 0x7FF00000 && (p[1] & 0xFFFFF || p[0]);
    }
#else
    static int isnan(double);
#endif

    static int isfinite(double);
    static int isinfinity(double);
    static int signbit(double);

    static double floor(double);
    static double round(double);	// round to nearest whole number
    static double exp(double x);
    static double pow(double x, double y);
    static double atan2(double x, double y);

    static ulonglong strtoull(const char *p, char **pend, int base);

    static char *ull_to_string(char *buffer, ulonglong ull);
    static wchar_t *ull_to_string(wchar_t *buffer, ulonglong ull);

    // Convert ulonglong to double
    static double ull_to_double(ulonglong ull);

    // Get locale-dependent list separator
#if defined UNICODE
    static void list_separator(unsigned long lcid, wchar_t buffer[5]);
#else
    static void list_separator(unsigned long lcid, char buffer[5]);
#endif

    static char *itoa(int val, char *s, int rad);
    static wchar_t *itow(int val, wchar_t *s, int rad);

    // Read high frequency timer
    static ulonglong read_timer();
};

#endif
