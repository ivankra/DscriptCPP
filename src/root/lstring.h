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

// lstring.h
// length-prefixed strings

// Written by Walter Bright

#ifndef LSTRING_H
#define LSTRING_H 1

#include "dchar.h"

struct Mem;

struct Lstring
{
    unsigned length;

#if defined __GNUC__
    dchar string[1];
#else
    // Disable warning about nonstandard extension
    #pragma warning (disable : 4200)
    dchar string[];
#if __DMC__ && __DMC__ < 0x828
    #error need 8.28 or later for string[]
#endif
#endif

    static Lstring zero;	// 0 length string

    // No constructors because we want to be able to statically
    // initialize Lstring's, and Lstrings are of variable size.

#if defined UNICODE
#  define LSTRING(p,length) { length, L##p }
#else
#  define LSTRING(p,length) { length, p }
#endif

    static Lstring *ctor(const dchar *p) { return ctor(p, Dchar::len(p)); }
    static Lstring *ctor(const dchar *p, unsigned length);
    static unsigned size(unsigned len) { return sizeof(unsigned) + (len + 1) * sizeof(dchar); }
    static Lstring *alloc(unsigned length);
    static Lstring *alloc(Mem *mem, unsigned length);
    Lstring *clone();

    unsigned len() { return length; }

    dchar *toDchars() { return string; }

    unsigned hash() { return Dchar::calcHash(string, length); }
    unsigned ihash() { return Dchar::icalcHash(string, length); }

    static int cmp(const Lstring *s1, const Lstring *s2)
    {
	int c = s2->length - s1->length;
	return c ? c : Dchar::memcmp(s1->string, s2->string, s1->length);
    }

    static int icmp(const Lstring *s1, const Lstring *s2)
    {
	int c = s2->length - s1->length;
	return c ? c : Dchar::memicmp(s1->string, s2->string, s1->length);
    }

    Lstring *append(const Lstring *s);
    Lstring *substring(int start, int end);
};

#endif
