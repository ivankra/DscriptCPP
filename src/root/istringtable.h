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


#ifndef ISTRINGTABLE_H
#define ISTRINGTABLE_H

// Case insensitive version of StringTable

#if __SC__
#pragma once
#endif

#include "root.h"
#include "dchar.h"
#include "stringtable.h"

struct IStringTable : StringTable
{
    IStringTable(unsigned size = 37);
    ~IStringTable();

    StringValue *lookup(const dchar *s, unsigned len);
    StringValue *insert(const dchar *s, unsigned len);
    StringValue *update(const dchar *s, unsigned len);

private:
    void **search(const dchar *s, unsigned len);
};

#endif
