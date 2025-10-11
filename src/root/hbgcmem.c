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

#if defined linux
#include <unistd.h>
#endif

#include "mutex.h"
#include "mem.h"
#include "gc.h"
#include "printf.h"

/* This implementation of the storage allocator uses the standard C allocation package.
 */

Mem mem;

Mutex gc_mutex;

void Mem::init()
{
}

char *Mem::strdup(const char *s)
{
    unsigned len;
    char *p;

    if (s)
    {
	len = strlen(s);
	gc_mutex.acquire();
	p = (char *)GC_MALLOC_ATOMIC(len + 1);
	gc_mutex.release();
	if (p)
	{
	    memcpy(p, s, len + 1);
	    return p;
	}
	error();
    }
    return NULL;
}

void *Mem::malloc(size_t size)
{   void *p;

    gc_mutex.acquire();
    p = GC_MALLOC(size);
    gc_mutex.release();
    if (!p)
	error();
    return p;
}

void *Mem::malloc_uncollectable(size_t size)
{   void *p;

    gc_mutex.acquire();
    p = GC_MALLOC_UNCOLLECTABLE(size);
    gc_mutex.release();
    if (!p)
	error();
    return p;
}

void *Mem::calloc(size_t size, size_t n)
{   void *p;

	gc_mutex.acquire();
	p = GC_MALLOC(size * n);
	gc_mutex.release();
	if (!p)
	    error();
    return p;
}

void *Mem::realloc(void *p, size_t size)
{
    if (!p)
    {
	gc_mutex.acquire();
	p = GC_MALLOC(size);
	gc_mutex.release();
	if (!p)
	    error();
    }
    else
    {
	gc_mutex.acquire();
	p = GC_REALLOC(p, size);
	gc_mutex.release();
	if (!p)
	    error();
    }
    return p;
}

void Mem::free(void *p)
{
    if (p)
    {
	gc_mutex.acquire();
	GC_FREE(p);
	gc_mutex.release();
    }
}

void Mem::free_uncollectable(void *p)
{
    free(p);
}

void *Mem::mallocdup(void *o, size_t size)
{   void *p;

    gc_mutex.acquire();
    p = GC_MALLOC(size);
    gc_mutex.release();
    if (!p)
	error();
    else
	memcpy(p,o,size);
    return p;
}

void Mem::error()
{
    assert(0);
    printf("Error: out of memory\n");
    exit(EXIT_FAILURE);
}

void Mem::fullcollect()
{
    gc_mutex.acquire();
    GC_gcollect();
    gc_mutex.release();
}


void Mem::mark(void *pointer)
{
    (void) pointer;			// for VC /W4 compatibility
}


void Mem::addroots(char* pStart, char* pEnd)
{
    gc_mutex.acquire();
    GC_add_roots(pStart, pEnd);
    gc_mutex.release();
}


void Mem::removeroots(char* pStart)
{
    (void) pStart;
}


void Mem::setFinalizer(void* pObj, FINALIZERPROC pFn, void* pClientData)
{
    gc_mutex.acquire();
    GC_register_finalizer(pObj, (GC_finalization_proc)pFn, pClientData, NULL, NULL);
    gc_mutex.release();
}


void Mem::setStackBottom(void *stackbottom)
{
#if defined _MSC_VER
    (void) stackbottom;
#endif
#if defined linux
    // Without this, the GC will call GC_linux_stack_bottom() which will
    // produce a stack bottom that GP faults.
    // Linux unhelpfully doesn't tell us what the bottom of the stack is.
    // But we need it to scan the stack properly for garbage collection.
    // Therefore, on every entry point, call this function with an address
    // of one of its parameters, and we'll kludge a stack bottom.

    static int main_pid = 0;		// pid of main thread

    if (GC_stackbottom == 0)
    {
	main_pid = getpid();
	GC_stackbottom = (char *)stackbottom;
    }
    else if (GC_stackbottom < (char *)stackbottom && main_pid == getpid())
    {
	GC_stackbottom = (char *)stackbottom;
    }
    else
    {
	WPRINTF(L"setStackBottom(): non-main thread x%x\n", getpid());
    }
#endif
}


Mem *Mem::getThreadMem()
{
    return &mem;
}


/* =================================================== */

//
// 
//
#if 0

void * operator new(size_t m_size)
{   void *p;

    gc_mutex.acquire();
    p = GC_MALLOC(m_size);
    gc_mutex.release();
    //printf("operator new(%d) = %p\n", m_size, p);
    if (!p)
	mem.error();
    return p;
}

void operator delete(void *p)
{
//    printf("operator delete(%p)\n", p);
    gc_mutex.acquire();
    GC_FREE(p);
    gc_mutex.release();
}

#elif !defined linux
void * operator new(size_t m_size)
{
    assert("Unexpected call to global operator new!" == NULL);
    return NULL;
}

void operator delete(void *p)
{
    assert("Unexpected call to global operator delete!" == NULL);
}

#  if defined NEW_ARRAY
void* operator new[](size_t size)
{
    return operator new(size);
}

void operator delete[](void*pv);
{
    operator delete(pv);
}
#  endif // NEW_ARRAY
#endif

void * Mem::operator new(size_t m_size)
{   void *p;

    gc_mutex.acquire();
    p = GC_MALLOC(m_size);
    gc_mutex.release();
    //printf("Mem::operator new(%d) = %p\n", m_size, p);
    if (!p)
	mem.error();
    return p;
}

void Mem::operator delete(void *p)
{
//    printf("Mem::operator delete(%p)\n", p);
    gc_mutex.acquire();
    GC_FREE(p);
    gc_mutex.release();
}



