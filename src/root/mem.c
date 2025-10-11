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
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "mem.h"

/* This implementation of the storage allocator uses the standard C allocation package.
 */

Mem mem;

#ifdef DEBUG
#define MEM_DEBUG       1               // enable simple memory allocation debugging
#endif

#if MEM_DEBUG

// Add simple debugging features:
// o    Under and overrun detection
// o    Valid pointer detection
// o    Stomping on uninitialized data
// o    Stomping on free'd data

struct Mem_debug        // The size of this must be a multiple of 8 bytes for
                        // alignment of double's
{
    size_t size;
    int start_sentinal;

#define START_SENTINAL  0xabcd5432
#define END_SENTINAL    0x7328efbc
};

#endif


void Mem::init()
{
}

char *Mem::strdup(const char *s)
{
    char *p;

    if (s)
    {
#if MEM_DEBUG
        size_t len;

        len = strlen(s);
        p = (char *)malloc(len + 1);
        if (p)
        {
            memcpy(p, s, len + 1);
            return p;
        }
#else
        p = ::strdup(s);
        if (p)
            return p;
#endif
        error();
    }
    return NULL;
}

void *Mem::malloc(size_t size)
{   void *p;

#if MEM_DEBUG
    unsigned nbytes = sizeof(Mem_debug) + size + sizeof(size_t);
    Mem_debug *m;

    m = (Mem_debug *)::malloc(nbytes);
    if (m)
    {
        m->size = size;
        m->start_sentinal = START_SENTINAL;
        p = (char *)(m + 1);
        memset(p, 0x55, size);
        *(size_t *)((char *)p + size) = END_SENTINAL;
    }
    else
    {   error();
        p = NULL;
    }
#else
    p = ::malloc(size);
    if (!p)
        error();
#endif
    return p;
}

void *Mem::malloc_uncollectable(size_t size)
{
    return malloc(size);
}

void *Mem::calloc(size_t size, size_t n)
{   void *p;

#if MEM_DEBUG
    p = (char *)malloc(size * n);
    if (p)
        memset(p, 0, size * n);
    else
        error();
#else
    p = ::calloc(size, n);
    if (!p)
        error();
#endif
    return p;
}

void *Mem::realloc(void *p, size_t size)
{
#if MEM_DEBUG
    if (!p)
    {
        p = malloc(size);
        if (!p)
            error();
    }
    else
    {
        void *newp;
        Mem_debug *m;

        m = (Mem_debug *)p - 1;
        newp = malloc(size);
        if (!newp)
        {   error();
            p = NULL;
        }
        else
        {   size_t nbytes;

            nbytes = size;
            if (m->size < nbytes)
                nbytes = m->size;
            memcpy(newp, p, nbytes);
            free(p);
            p = newp;
        }
    }
#else
    if (!p)
    {
        p = ::malloc(size);
        if (!p)
            error();
    }
    else
    {
        p = ::realloc(p, size);
        if (!p)
            error();
    }
#endif
    return p;
}

void Mem::free(void *p)
{
    if (p)
    {
#if MEM_DEBUG
        Mem_debug *m = (Mem_debug *)p - 1;

        assert(m->start_sentinal == START_SENTINAL);
        assert(*(size_t *)((char *)p + m->size) == END_SENTINAL);
        memset(m, 0xCC, sizeof(Mem_debug) + m->size + sizeof(size_t));
        ::free(m);
#else
        ::free(p);
#endif
    }
}

void Mem::free_uncollectable(void *p)
{
    free(p);
}

void *Mem::mallocdup(void *o, size_t size)
{   void *p;

#if MEM_DEBUG
    p = malloc(size);
#else
    p = ::malloc(size);
#endif
    if (!p)
        error();
    else
        memcpy(p,o,size);
    return p;
}

void Mem::error()
{
    printf("Error: out of memory\n");
    exit(EXIT_FAILURE);
}

void Mem::fullcollect()
{
}

void Mem::mark(void *pointer)
{
    (void) pointer;             // necessary for VC /W4
}

void Mem::addroots(char* pStart, char* pEnd)
{
    (void) pStart;
    (void) pEnd;
}

void Mem::removeroots(char* pStart)
{
    (void) pStart;
}

void Mem::setFinalizer(void* pObj, FINALIZERPROC pFn, void* pClientData)
{
    (void) pObj;
    (void) pFn;
    (void) pClientData;
}

void Mem::setStackBottom(void *bottom)
{
    (void) bottom;
}

/* =================================================== */

void * Mem::operator new(size_t m_size)
{
   return mem.malloc(m_size);
}

void * Mem::operator new(size_t m_size, Mem *mymem)
{
   return mem.malloc(m_size);
}

void * Mem::operator new(size_t m_size, GC *mygc)
{
   return mem.malloc(m_size);
}

void Mem::operator delete(void *p)
{
    mem.free(p);
}

void * Mem::operator new[](size_t m_size)
{
   return operator new(m_size);
}

void Mem::operator delete[](void *p)
{
    operator delete(p);
}

