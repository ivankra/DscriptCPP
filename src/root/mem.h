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

#ifndef MEM_H
#define MEM_H

typedef unsigned size_t;

typedef void (*FINALIZERPROC)(void* pObj, void* pClientData);

struct GC;			// thread specific allocator

struct Mem
{
    GC *gc;			// pointer to our thread specific allocator
    Mem() { gc = NULL; }

    void init();

    // Derive from Mem to get these storage allocators instead of global new/delete
    void * operator new(size_t m_size);
    void * operator new(size_t m_size, Mem *mem);
    void * operator new(size_t m_size, GC *gc);
    void operator delete(void *p);

    void * operator new[](size_t m_size);
    void operator delete[](void *p);

    char *strdup(const char *s);
    void *malloc(size_t size);
    void *malloc_atomic(size_t size);	// no pointers in allocated block
    void *malloc_uncollectable(size_t size);
    void *calloc(size_t size, size_t n);
    void *realloc(void *p, size_t size);
    void free(void *p);
    void free_uncollectable(void *p);
    void *mallocdup(void *o, size_t size);
    void error();
    void check(void *p);	// validate pointer
    void fullcollect();		// do full garbage collection
    void fullcollectNoStack();	// do full garbage collection, no scan stack
    void mark(void *pointer);
    void addroots(char* pStart, char* pEnd);
    void removeroots(char* pStart);
    void setFinalizer(void* pObj, FINALIZERPROC pFn, void* pClientData);
    void setStackBottom(void *bottom);
    GC *getThreadGC();		// get apartment allocator for this thread
};

extern Mem mem;

#endif
