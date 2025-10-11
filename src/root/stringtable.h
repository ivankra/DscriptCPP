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


#ifndef STRINGTABLE_H
#define STRINGTABLE_H

#if __SC__
#pragma once
#endif

#include "root.h"
#include "dchar.h"
#include "lstring.h"
#include "mem.h"


#if CASE_INSENSITIVE
#define CALCHASH	icalcHash
#define CMP		icmp
#define MEMCMP		memicmp
#else
#define CALCHASH	calcHash
#define CMP		cmp
#define MEMCMP		memcmp
#endif

/* Decide to use GC mem, or our own allocator.
 */

#define USE_GC_ALLOC	1	// 1: use GC
				// 0: use custom heap

struct StringEntry;

struct StringValue
{
#if ADD_DATA_PTR
    void   *ptrvalue;
#endif
    Lstring lstring;
};

struct StringTable : Object
{
    void **table;
    unsigned count;
    unsigned tabledim;
#if USE_GC_ALLOC
    Mem mem;
#else
    void *pbase;		// base of current chunk of memory
    char *pmark;		// high water mark in pbase
    unsigned bytesleft;		// bytes left in pbase chunk
    void *alloclist;		// list of previously allocated chunks

    void *mycalloc(unsigned size);
#endif

    StringTable(unsigned size = 37);
    ~StringTable();

    StringValue *lookup(const dchar *s, unsigned len);
    StringValue *insert(const dchar *s, unsigned len);
    StringValue *update(const dchar *s, unsigned len);

private:
    void **search(const dchar *s, unsigned len);

    StringEntry *alloc(const dchar *s, unsigned len);
};

struct StringEntry
{
    StringEntry *left;
    StringEntry *right;
    unsigned hash;

    StringValue value;

    void rzero();
};

#endif
