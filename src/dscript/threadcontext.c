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
#include "printf.h"

#include "dscript.h"

#define LOG	0	// 1: log thread creation/destruction
			// 0: no logging

#if defined linux

#include <pthread.h>

extern "C"
{

// Key identifying the thread-specific data
static pthread_key_t threadcontext_key;

/* "Once" variable ensuring that the key for threadcontext_alloc will be allocated
 * exactly once.
 */
static pthread_once_t threadcontext_alloc_key_once = PTHREAD_ONCE_INIT;

/* Forward functions */
static void threadcontext_alloc_key();
static void threadcontext_alloc_destroy_tc(void * accu);


// Function to allocate the key for threadcontext_alloc thread-specific data.

static void threadcontext_alloc_key()
{
    pthread_key_create(&threadcontext_key, threadcontext_alloc_destroy_tc);
#if LOG
    WPRINTF(L"Thread %lx: allocated tc key %d\n", pthread_self(), threadcontext_key);
#endif
}

// Function to free the buffer when the thread exits.
// Called only when the thread-specific data is not NULL.

static void threadcontext_alloc_destroy_tc(void *tc)
{
#if LOG
    WPRINTF(L"Thread %x: freeing tc at %x\n", pthread_self(), tc);
#endif
    memset(tc, 0, sizeof(ThreadContext));
    ::free(tc);

    // Don't need to free the roots because the gc is going away anyway
}

}

void ThreadContext::init()
{
#if LOG
    WPRINTF(L"Thread %lx: ThreadContext::init()\n", pthread_self());
#endif
    pthread_once(&threadcontext_alloc_key_once, threadcontext_alloc_key);
}

ThreadContext *ThreadContext::getThreadContext()
{
    ThreadContext *tc;

    // Get the thread-specific data associated with the key
    tc = (ThreadContext *) pthread_getspecific(threadcontext_key);

    // It's initially NULL, meaning that we must allocate the buffer first.
    if (tc == NULL)
    {
	tc = (ThreadContext *)::calloc(1, sizeof(ThreadContext));

	// Store the buffer pointer in the thread-specific data.
	pthread_setspecific(threadcontext_key, (void *) tc);

	::mem.addroots((char *)tc, (char *)(tc + 1));
#if LOG
	WPRINTF(L"Thread %lx: allocating tc at %x\n", pthread_self(), tc);
#endif
    }
    return tc;
}


#endif
