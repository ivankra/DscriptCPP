/* Digital Mars DMDScript source code.
 * Copyright (c) 2001-2007 by Digital Mars
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

#include <windows.h>

#include "os.h"

/***********************************
 * Map memory.
 */

void *os_mem_map(unsigned nbytes)
{
    return VirtualAlloc(NULL, nbytes, MEM_RESERVE, PAGE_READWRITE);
}

/***********************************
 * Commit memory.
 * Returns:
 *	0	success
 *	!=0	failure
 */

int os_mem_commit(void *base, unsigned offset, unsigned nbytes)
{
    void *p;

    p = VirtualAlloc((char *)base + offset, nbytes, MEM_COMMIT, PAGE_READWRITE);
    return (p == NULL);
}


/***********************************
 * Decommit memory.
 * Returns:
 *	0	success
 *	!=0	failure
 */

int os_mem_decommit(void *base, unsigned offset, unsigned nbytes)
{
    return VirtualFree((char *)base + offset, nbytes, MEM_DECOMMIT) == 0; 
}

/***********************************
 * Unmap memory allocated with os_mem_map().
 * Memory must have already been decommitted.
 * Returns:
 *	0	success
 *	!=0	failure
 */

int os_mem_unmap(void *base, unsigned nbytes)
{
    (void)nbytes;
    return VirtualFree(base, 0, MEM_RELEASE) == 0; 
}


/********************************************
 */

pthread_t pthread_self()
{
    return (pthread_t) GetCurrentThreadId();
}
