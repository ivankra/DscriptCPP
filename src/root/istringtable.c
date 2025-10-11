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
#include "istringtable.h"

IStringTable::IStringTable(unsigned size)
    : StringTable(size)
{
}

IStringTable::~IStringTable()
{
}

void **IStringTable::search(const dchar *s, unsigned len)
{
    unsigned hash;
    unsigned u;
    int cmp;
    StringEntry **se;

    //printf("StringTable::search(%p,%d)\n",s,len);
    hash = Dchar::icalcHash(s,len);
    u = hash % tabledim;
    se = (StringEntry **)&table[u];
    //printf("\thash = %d, u = %d\n",hash,u);
    while (*se)
    {
	cmp = (*se)->hash - hash;
	if (cmp == 0)
	{
	    cmp = (*se)->value.lstring.len() - len;
	    if (cmp == 0)
	    {
		cmp = Dchar::memicmp(s,(*se)->value.lstring.toDchars(),len);
		if (cmp == 0)
		    break;
	    }
	}
	if (cmp < 0)
	    se = &(*se)->left;
	else
	    se = &(*se)->right;
    }
    //printf("\treturn %p\n",se);
    return (void **)se;
}

StringValue *IStringTable::lookup(const dchar *s, unsigned len)
{   StringEntry *se;

    se = *(StringEntry **)search(s,len);
    if (se)
	return &se->value;
    else
	return NULL;
}

StringValue *IStringTable::insert(const dchar *s, unsigned len)
{   StringEntry **pse;
    StringEntry *se;

    pse = (StringEntry **)search(s,len);
    se = *pse;
    if (se)
	return NULL;		// error: already in table
    else
    {
	se = StringEntry::alloc(s, len);
	se->hash = Dchar::icalcHash(s,len);
	*pse = se;
    }
    return &se->value;
}


StringValue *IStringTable::update(const dchar *s, unsigned len)
{   StringEntry **pse;
    StringEntry *se;

    pse = (StringEntry **)search(s,len);
    se = *pse;
    if (!se)			// not in table: so create new entry
    {
	se = StringEntry::alloc(s, len);
	se->hash = Dchar::icalcHash(s,len);
	*pse = se;
    }
    return &se->value;
}



