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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if _MSC_VER
#include <malloc.h>
#endif

#include "mem.h"
#include "dchar.h"
#include "root.h"
#include "thread.h"
#include "mutex.h"

#include "dscript.h"

/* This is the win32 version. It suffers from the bug that
 * when the thread exits the data structure is not cleared,
 * but the memory pool it points to is free'd.
 * Thus, if a new thread comes along with the same thread id,
 * the data will look initialized, but will point to garbage.
 *
 * What needs to happen is when a thread exits, the associated
 * ThreadContext data struct is cleared.
 */

/***********************************************
 * Get ThreadContext associated with this thread.
 */

#if 1

// Multithreaded version

Mutex threadcontext_mutex;

static ThreadContext array[64];

void ThreadContext::init()
{
    ::mem.addroots((char *)array, (char *)(array + 64));
}


// Array of pointers to ThreadContext objects, one per threadid
ThreadContext *threadcontext = array;
unsigned threadcontext_allocdim = 64;
unsigned threadcontext_dim;

ThreadId cache_ti;
ThreadContext *cache_cc;

ThreadContext *ThreadContext::getThreadContext()
{
    /* This works by creating an array of ThreadContext's, one
     * for each thread. We match up by thread id.
     */

    ThreadId ti;
    ThreadContext *cc;

    //PRINTF("ThreadContext::getThreadContext()\n");

    ti = Thread::getId();
    threadcontext_mutex.acquire();

    // Used cached version if we can
    if (ti == cache_ti)
    {
	cc = cache_cc;
	//exception(L"getThreadContext(): cache x%x", ti);
    }
    else
    {
	// This does a linear search through threadcontext[].
	// A hash table might be faster if there are more
	// than a dozen threads.
	ThreadContext *ccp;
	ThreadContext *ccptop = &threadcontext[threadcontext_dim];
	for (ccp = threadcontext; ccp < ccptop; ccp++)
	{
	    cc = ccp;
	    if (cc->threadid == ti)
	    {
		//WPRINTF(L"getThreadContext(): existing x%x", ti);
		goto Lret;
	    }
	}

	// Do not allocate with garbage collector, as this must reside
	// global to all threads.

#if 1
	assert(threadcontext_dim < threadcontext_allocdim);
	cc = ccp;
	memset(cc, 0, sizeof(*cc));
	cc->threadid = ti;
#else
	// Not in array, create new one
	cc = (ThreadContext *)::calloc(1, sizeof(ThreadContext));
	assert(cc);
	cc->threadid = ti;

	if (threadcontext_dim == threadcontext_allocdim)
	{   unsigned newdim;
	    ThreadContext **newtc;

	    newdim = threadcontext_allocdim * 2 + 8;
	    newtc = (ThreadContext **)::malloc(newdim * sizeof(ThreadContext *));
	    assert(newtc);
	    if (threadcontext)
	    {	memcpy(newtc, threadcontext, threadcontext_dim * sizeof(ThreadContext *));
		::free(threadcontext);
	    }
	    threadcontext = newtc;
	    threadcontext_allocdim = newdim;
	}

	threadcontext[threadcontext_dim] = cc;
#endif
	threadcontext_dim++;
	//WPRINTF(L"getThreadContext(): new x%x\n", ti);

    Lret:
	// Cache for next time
	cache_ti = ti;
	cache_cc = cc;
    }

    threadcontext_mutex.release();
    return cc;
}

#else

// Single threaded version

static ThreadContext cc;

ThreadContext *ThreadContext::getThreadContext()
{
    //PRINTF("getThreadContext()\n");
    return &cc;
}

#endif
