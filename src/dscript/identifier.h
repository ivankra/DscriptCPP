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

#ifndef IDENTIFIER_H
#define IDENTIFIER_H

//#pragma once

#include "dchar.h"
#include "root.h"
#include "lstring.h"

#include "dscript.h"

#if 1

struct Identifier
{
    dchar *toDchars() { return ((Lstring *)this)->toDchars(); }
    Lstring *toLstring() { return (Lstring *)this; }
    void toBuffer(OutBuffer *buf);
    int equals(Identifier *id);
};

#else
struct Identifier : Object
{
    Lstring *string;

    Identifier(Lstring *string);
    int equals(Object *o);
    unsigned hashCode();
    int compare(Object *o);
    void print();
    dchar *toDchars() { return string->toDchars(); }
    void toBuffer(OutBuffer *buf);
};
#endif

#endif





