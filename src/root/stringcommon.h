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

#ifndef STRINGCOMMON_H
#define STRINGCOMMON_H

#if __SC__
#pragma once
#endif

#include "root.h"
#include "dchar.h"
#include "lstring.h"

struct StringValue
{
    union
    {	int intvalue;
	void *ptrvalue;
	dchar *string;
    };
    Lstring lstring;
};

struct StringEntry
{
    StringEntry *left;
    StringEntry *right;
    unsigned hash;

    StringValue value;

    static StringEntry *alloc(const dchar *s, unsigned len);
};

#endif // STRINGCOMMON_H
