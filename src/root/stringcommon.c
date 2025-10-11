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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "root.h"
#include "mem.h"
#include "dchar.h"
#include "lstring.h"
#include "stringcommon.h"

StringEntry *StringEntry::alloc(const dchar *s, unsigned len)
{
    StringEntry *se;

#if __GNUC__
    se = (StringEntry *) mem.calloc(1,sizeof(StringEntry) - 4 + Lstring::size(len));
#else
    se = (StringEntry *) mem.calloc(1,sizeof(StringEntry) - sizeof(Lstring) + Lstring::size(len));
#endif
    se->value.lstring.length = len;
    se->hash = Dchar::calcHash(s,len);
    memcpy(se->value.lstring.string, s, len * sizeof(dchar));
    return se;
}
