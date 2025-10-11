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

// lstring.c

#include <stdio.h>
#include <stdlib.h>

#include "gc.h"
#include "dchar.h"
#include "mem.h"
#include "lstring.h"
#include "printf.h"

Lstring Lstring::zero = LSTRING("", 0);

Lstring *Lstring::ctor(const dchar *p, unsigned length)
{
    Lstring *s;

    //WPRINTF(L"Lstring::ctor('%s', %d)\n", p, length);
    s = alloc(length);
    if(s)
    {
	memcpy(s->string, p, length * sizeof(dchar));
    }
    return s;
}

Lstring *Lstring::alloc(unsigned length)
{
    Lstring *s;

    s = (Lstring *)mem.malloc_atomic(size(length));
    if(s)
    {
	s->length = length;
	s->string[length] = 0;
    }
    return s;
}

Lstring *Lstring::alloc(Mem *mem, unsigned length)
{
    Lstring *s;

    s = (Lstring *)mem->malloc_atomic(size(length));
    if(s)
    {
	s->length = length;
	s->string[length] = 0;
    }
    return s;
}

Lstring *Lstring::append(const Lstring *s)
{
    Lstring *t;

    if (!s->length)
    {
	return this;
    }

    t = alloc(length + s->length);

    if(t)
    {
	memcpy(t->string, string, length * sizeof(dchar));
	memcpy(t->string + length, s->string, s->length * sizeof(dchar));
    }
    return t;
}

Lstring *Lstring::substring(int start, int end)
{
    Lstring *t;

    if (start == end)
    {
	return &zero;
    }

    t = alloc(end - start);

    if(t)
    {
	memcpy(t->string, string + start, (end - start) * sizeof(dchar));
    }
    return t;
}
