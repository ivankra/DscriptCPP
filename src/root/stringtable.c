/* Digital Mars DMDScript source code.
 * Copyright (c) 2000-2002 by Chromium Communications
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

#include "printf.h"
#include "root.h"
#include "mem.h"
#include "dchar.h"
#include "lstring.h"
#include "stringtable.h"
#include "gc.h"

#define LOG 0

#if !USE_GC_ALLOC

//#define CHUNK_SIZE	(0x4000 - 128)
#define CHUNK_SIZE	(4096)

void *StringTable::mycalloc(unsigned size)
{   void *p;

#if LOG
    WPRINTF(L"StringTable::mycalloc(size = %d)\n", size);
#endif
    assert(size <= CHUNK_SIZE);
    if (size > bytesleft)
    {
	if (pbase)
	    *(void **)pbase = alloclist;
	alloclist = pbase;

	// Allocate new chunk with C allocator
	pbase = ::calloc(CHUNK_SIZE, 1);
	if (!pbase)
	{
	    WPRINTF(L"StringTable::mycalloc()\n");
	    assert(0);
	}
	pmark = (char *)pbase;
	bytesleft = CHUNK_SIZE;
    }

    p = pmark;
    pmark += size;
    bytesleft -= size;
#if LOG
    WPRINTF(L"\tp = x%x\n", p);
#endif
    return p;
}

#endif

StringTable::StringTable(unsigned size)
{
#if LOG
    WPRINTF(L"StringTable::StringTable(size = %d)\n", size);
    WPRINTF(L"\tsizeof(StringEntry) = %d, Lstring::size(0) = %d\n", sizeof(StringEntry), Lstring::size(0));
#endif
    tabledim = size;
    count = 0;

#if USE_GC_ALLOC
    GC_LOG();
    table = (void **)mem.calloc(size, sizeof(void *));
#else
    pbase = NULL;
    pmark = NULL;
    bytesleft = 0;
    alloclist = NULL;
    table = (void **)mycalloc(size * sizeof(void *));
#endif
}

StringTable::~StringTable()
{
#if LOG
    WPRINTF(L"~StringTable(%x)\n", this);
#endif

#if USE_GC_ALLOC
    unsigned i;

    // Zero out dangling pointers to help garbage collector.
    // Should zero out StringEntry's too.
    for (i = 0; i < tabledim; i++)
    {
	if (table[i])
	{
	    ((StringEntry *)table[i])->rzero();
	    table[i] = NULL;
	}
    }

    mem.free(table);
    table = NULL;
#else
    void *pnext = NULL;

    for (void *p = alloclist; p; p = pnext)
    {
	pnext = *(void **)p;
	::free(p);
    }

    if (pbase)
	::free(pbase);
#endif
}

/*********************************
 * Zero's out tree structure for better GC.
 */

void StringEntry::rzero()
{   StringEntry *se;
    StringEntry *tmp;

#if LOG
    WPRINTF(L"StringEntry::rzero()\n");
#endif
    se = this;
    for (;;)
    {
	tmp = se->left;
	if (tmp)
	{   se->left = NULL;
	    tmp->rzero();
	}
	tmp = se->right;
	if (!tmp)
	    break;
	se->right = NULL;
	se = tmp;
    }
}

StringEntry *StringTable::alloc(const dchar *s, unsigned len)
{
    StringEntry *se;

#if __GNUC__
#if 0
    WPRINTF(L"StringTable::alloc(");
    for (unsigned i = 0; i < len; i++)
	WPRINTF(L"%c", s[i]);
    WPRINTF(L", %d)\n", len);
#endif
#if USE_GC_ALLOC
    se = (StringEntry *) mem.malloc(sizeof(StringEntry) - 4 + Lstring::size(len));
#else
    se = (StringEntry *) mycalloc(sizeof(StringEntry) - 4 + Lstring::size(len));
#endif
#else
    //WPRINTF(L"StringTable::alloc(%.*s, %d)\n", s, len, len);
    //PRINTF("StringEntry = %d, Lstring = %d, size(%d) = %d\n", sizeof(StringEntry), sizeof(Lstring), len, Lstring::size(len));
#if USE_GC_ALLOC
    se = (StringEntry *) mem.malloc(sizeof(StringEntry) - sizeof(Lstring) + Lstring::size(len));
#else
    se = (StringEntry *) mycalloc(sizeof(StringEntry) - sizeof(Lstring) + Lstring::size(len));
#endif
#endif
    se->left = NULL;
    se->right = NULL;
#if ADD_DATA_PTR
    se->value.ptrvalue = NULL;
#endif
    se->value.lstring.length = len;
    se->hash = Dchar::CALCHASH(s,len);
    memcpy(se->value.lstring.string, s, len * sizeof(dchar));
    se->value.lstring.string[len] = 0;
    //message(DTEXT("String allocated: "));
    //for(int x=0; x<len; x++) message(DTEXT("%c"),s[x]); message(DTEXT("\n"));
    return se;
}

void **StringTable::search(const dchar *s, unsigned len)
{
    unsigned hash;
    unsigned u;
    int cmp;
    StringEntry **se;

    //wprintf(DTEXT("StringTable::search(this=%p, %p, %d)\n"), this, s,len);
    //for(int x=0; x<len; x++) wprintf(L"%c",s[x]); wprintf(L"\n");
    hash = Dchar::CALCHASH(s,len);
    u = hash % tabledim;
    se = (StringEntry **)&table[u];
    //wprintf(L"\ttable = %p, hash = %d, u = %d\n",table,hash,u);
    while (*se)
    {
	cmp = (*se)->hash - hash;
	if (cmp == 0)
	{
	    cmp = (*se)->value.lstring.len() - len;
	    if (cmp == 0)
	    {
		cmp = Dchar::MEMCMP(s,(*se)->value.lstring.toDchars(),len);
		if (cmp == 0)
		    break;
	    }
	}
	if (cmp < 0)
	    se = &(*se)->left;
	else
	    se = &(*se)->right;
    }
    //message(DTEXT("\treturn %p\n"),se);
    return (void **)se;
}

StringValue *StringTable::lookup(const dchar *s, unsigned len)
{   StringEntry *se;

    //message(DTEXT("StringTable::search(%p,%d) "),s,len);
    //for(int x=0; x<len; x++) message(DTEXT("%c"),s[x]); message(DTEXT("\n"));
    se = *(StringEntry **)search(s,len);
    if (se)
	return &se->value;
    else
	return NULL;
}

StringValue *StringTable::update(const dchar *s, unsigned len)
{   StringEntry **pse;
    StringEntry *se;

    pse = (StringEntry **)search(s,len);
    se = *pse;
    if (!se)			// not in table: so create new entry
    {
	GC_LOG();
	se = alloc(s, len);
	*pse = se;
	count++;
    }
    return &se->value;
}

StringValue *StringTable::insert(const dchar *s, unsigned len)
{   StringEntry **pse;
    StringEntry *se;

    pse = (StringEntry **)search(s,len);
    se = *pse;
    if (se)
	return NULL;		// error: already in table
    else
    {
	se = alloc(s, len);
	*pse = se;
	count++;
    }
    return &se->value;
}




