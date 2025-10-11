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

#ifndef DCHAR_H
#define DCHAR_H

#if _MSC_VER
    // Disable useless warnings about unreferenced functions
    #pragma warning (disable : 4514)
#endif // _MSC_VER


// NOTE: All functions accepting pointer arguments must not be NULL

#define isAsciiDigit(c) ('0' <= (c) && (c) <= '9')

#if defined UNICODE

#if defined __GNUC__
#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <wctype.h>
#define _wcsdup wcsdup
extern "C" int memicmp(const void *,const void *,int);

#if STANDALONE
#define wcsicmp  wcscasecmp
#define wsprintf wprintf
#define wcsnicmp wcsncasecmp
#endif
#endif

#include <wchar.h>
#include <string.h>
#include <ctype.h>

typedef wchar_t dchar;

#if (defined linux) && (!defined SIZEOFDCHAR)
#  define SIZEOFDCHAR 4
#elif !defined SIZEOFDCHAR
#  define SIZEOFDCHAR 2
#endif

#define DTEXT(x)	L##x

#define Dchar_mbmax	1

struct Dchar
{
    static dchar *inc(dchar *p) { return p + 1; }
    static dchar *dec(dchar *pstart, dchar *p) { (void)pstart; return p - 1; }
    static int len(const dchar *p) { return wcslen(p); }
    static dchar get(dchar *p) { return *p; }
    static dchar getprev(dchar *pstart, dchar *p) { (void)pstart; return p[-1]; }
    static dchar *put(dchar *p, dchar c) { *p = c; return p + 1; }
    static int cmp(dchar *s1, dchar *s2)
    {
#if defined __DMC__
	if (!*s1 && !*s2)	// wcscmp is broken
	    return 0;
#endif
	return wcscmp(s1, s2);
    }
    static int memcmp(const dchar *s1, const dchar *s2, int nchars) { return ::memcmp(s1, s2, nchars * sizeof(dchar)); }
    static int isDigit(dchar c) { return isAsciiDigit(c); }
    static int isAlpha(dchar c) { return iswalpha(c); }
    static int isSpace(dchar c) { return c <= 0x7F ? isspace(c) : 0; }
    static int isLocaleUpper(dchar c) { return isUpper(c); }
    static int isLocaleLower(dchar c) { return isLower(c); }
#if linux
    static int isUpper(dchar c);
    static int isLower(dchar c);
    static int toLower(dchar c);
    static int toUpper(dchar c);
#else
    static int isUpper(dchar c) { return iswupper(c); }
    static int isLower(dchar c) { return iswlower(c); }
    static int toLower(dchar c) { return towlower(c); }
    static int toUpper(dchar c) { return towupper(c); }
#endif
    static int toLower(dchar *p) { return toLower(*p); }
#ifdef linux
    static dchar *dup(dchar *p) { return ::wcsdup(p); }
#else
    static dchar *dup(dchar *p) { return ::_wcsdup(p); }  // BUG: out of memory?
#endif
    static dchar *dup(char *p);
    static dchar *chr(dchar *p, unsigned c) { return wcschr(p, (dchar)c); }
    static dchar *rchr(dchar *p, unsigned c) { return wcsrchr(p, (dchar)c); }
    static dchar *memchr(dchar *p, int c, int count);
    static dchar *cpy(dchar *s1, dchar *s2) { return wcscpy(s1, s2); }
    static dchar *str(dchar *s1, dchar *s2) { return wcsstr(s1, s2); }
    static unsigned calcHash(const dchar *str, unsigned len);

    // Case insensitive versions
#ifdef linux
    static int icmp(dchar *s1, dchar *s2) { return wcscasecmp(s1, s2); }
    static int memicmp(const dchar *s1, const dchar *s2, int nchars) { return ::wcsncasecmp(s1, s2, nchars); }
#else
    static int icmp(dchar *s1, dchar *s2) { return wcsicmp(s1, s2); }
    static int memicmp(const dchar *s1, const dchar *s2, int nchars) { return ::wcsnicmp(s1, s2, nchars); }
#endif
    static unsigned icalcHash(const dchar *str, unsigned len);
};

#elif MCBS

#include <limits.h>
#include <mbstring.h>

typedef char dchar;
#define SIZEOFDCHAR 1

#define DTEXT(x)	x

#define Dchar_mbmax	MB_LEN_MAX

#elif UTF8

typedef char dchar;
#define SIZEOFDCHAR 1

#define DTEXT(x)	x

#define Dchar_mbmax	6

struct Dchar
{
    static char mblen[256];

    static dchar *inc(dchar *p) { return p + mblen[*p & 0xFF]; }
    static dchar *dec(dchar *pstart, dchar *p);
    static int len(const dchar *p) { return strlen(p); }
    static int get(dchar *p);
    static int getprev(dchar *pstart, dchar *p)
	{ return *dec(pstart, p) & 0xFF; }
    static dchar *put(dchar *p, unsigned c);
    static int cmp(dchar *s1, dchar *s2) { return strcmp(s1, s2); }
    static int memcmp(const dchar *s1, const dchar *s2, int nchars) { return ::memcmp(s1, s2, nchars); }
    static int isDigit(dchar c) { return isAsciiDigit(c); }
    static int isAlpha(dchar c) { return isalpha(c); }
    static int isSpace(dchar c) { return isspace(c); }
    static int isUpper(dchar c) { return isupper(c); }
    static int isLower(dchar c) { return islower(c); }
    static int isLocaleUpper(dchar c) { return isUpper(c); }
    static int isLocaleLower(dchar c) { return isLower(c); }
    static int toLower(dchar c) { return tolower(c); }
    static int toLower(dchar *p) { return toLower(*p); }
    static int toUpper(dchar c) { return toupper(c); }
    static dchar *dup(dchar *p) { return ::strdup(p); }	// BUG: out of memory?
    static dchar *chr(dchar *p, int c) { return strchr(p, c); }
    static dchar *rchr(dchar *p, int c) { return strrchr(p, c); }
    static dchar *memchr(dchar *p, int c, int count)
	{ return (dchar *)::memchr(p, c, count); }
    static dchar *cpy(dchar *s1, dchar *s2) { return strcpy(s1, s2); }
    static dchar *str(dchar *s1, dchar *s2) { return strstr(s1, s2); }
    static unsigned calcHash(const dchar *str, unsigned len);

    // Case insensitive versions
    static int icmp(dchar *s1, dchar *s2) { return _mbsicmp(s1, s2); }
    static int memicmp(const dchar *s1, const dchar *s2, int nchars) { return ::_mbsnicmp(s1, s2, nchars); }
};

#else

#include <string.h>
#include <ctype.h>

typedef char dchar;
#define SIZEOFDCHAR 1

#define DTEXT(x)	x

#define Dchar_mbmax	1

struct Dchar
{
    static dchar *inc(dchar *p) { return p + 1; }
    static dchar *dec(dchar *pstart, dchar *p) { return p - 1; }
    static int len(const dchar *p) { return strlen(p); }
    static int get(dchar *p) { return *p & 0xFF; }
    static int getprev(dchar *pstart, dchar *p) { return p[-1] & 0xFF; }
    static dchar *put(dchar *p, unsigned c) { *p = c; return p + 1; }
    static int cmp(dchar *s1, dchar *s2) { return strcmp(s1, s2); }
    static int memcmp(const dchar *s1, const dchar *s2, int nchars) { return ::memcmp(s1, s2, nchars); }
    static int isDigit(dchar c) { return isAsciiDigit(c); }
    static int isAlpha(dchar c) { return isalpha(c); }
    static int isSpace(dchar c) { return isspace(c); }
    static int isUpper(dchar c) { return isupper(c); }
    static int isLower(dchar c) { return islower(c); }
    static int isLocaleUpper(dchar c) { return isupper(c); }
    static int isLocaleLower(dchar c) { return islower(c); }
    static int toLower(dchar c) { return tolower(c); }
    static int toLower(dchar *p) { return toLower(*p); }
    static int toUpper(dchar c) { return toupper(c); }
    static dchar *dup(dchar *p) { return ::strdup(p); }	// BUG: out of memory?
    static dchar *chr(dchar *p, int c) { return strchr(p, c); }
    static dchar *rchr(dchar *p, int c) { return strrchr(p, c); }
    static dchar *memchr(dchar *p, int c, int count)
	{ return (dchar *)::memchr(p, c, count); }
    static dchar *cpy(dchar *s1, dchar *s2) { return strcpy(s1, s2); }
    static dchar *str(dchar *s1, dchar *s2) { return strstr(s1, s2); }
    static unsigned calcHash(const dchar *str, unsigned len);

    // Case insensitive versions
#ifdef __GNUC__
    static int icmp(dchar *s1, dchar *s2) { return strcasecmp(s1, s2); }
    static int memicmp(const dchar *s1, const dchar *s2, int nchars) {  return ::strncasecmp(s1,s2,nchars); }
#else
    static int icmp(dchar *s1, dchar *s2) { return stricmp(s1, s2); }
    static int memicmp(const dchar *s1, const dchar *s2, int nchars) { return ::memicmp(s1, s2, nchars); }
#endif
    static unsigned icalcHash(const dchar *str, unsigned len);
};

#endif
#endif

