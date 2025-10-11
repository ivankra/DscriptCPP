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
#include <assert.h>
#if !defined linux
#  include <windows.h>
#endif

#include "mutex.h"
#include "mem.h"

Mutex::Mutex()
{
#if defined linux
    pthread_mutex_init(&data, 0);
#else
    InitializeCriticalSection((CRITICAL_SECTION *)data);
#endif
}

Mutex::~Mutex()
{
#if defined linux
    pthread_mutex_destroy(&data);
#else
    DeleteCriticalSection((CRITICAL_SECTION *)data);
#endif
}

void Mutex::acquire()
{
#if defined linux
    pthread_mutex_lock(&data);
#else
    assert(sizeof(data) == sizeof(CRITICAL_SECTION));
    EnterCriticalSection((CRITICAL_SECTION *)data);
#endif
}

void Mutex::release()
{
#if defined linux
    pthread_mutex_unlock(&data);
#else
    LeaveCriticalSection((CRITICAL_SECTION *)data);
#endif
}

void Mutex::error()
{
    assert(0);
}

