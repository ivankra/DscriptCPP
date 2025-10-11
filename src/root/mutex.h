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


#ifndef MUTEX_H
#define MUTEX_H

#if defined linux
#include <pthread.h>
#endif

struct Mutex
{
#if defined linux
    pthread_mutex_t data;
#else
    char data[24];		// must match sizeof(CRITICAL_SECTION)
#endif

    Mutex();
    ~Mutex();

    void acquire();
    void release();
    void error();
};

#endif
