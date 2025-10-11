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

#include "thread.h"

#if linux

#include <pthread.h>

ThreadId Thread::getId()
{
    return pthread_self();
}

#elif _WIN32

#include <windows.h>

ThreadId Thread::getId()
{
    return (ThreadId) GetCurrentThreadId();
}

#endif

