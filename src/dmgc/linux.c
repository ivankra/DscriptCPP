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

#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>


/*************************************
 * This is all necessary to get fd initialized at startup.
 */

#define FDMAP 0

#if FDMAP
#include <fcntl.h>

struct OS_INIT
{
    static int fd;

    OS_INIT();
};

OS_INIT os_init;

int OS_INIT::fd = 0;

OS_INIT::OS_INIT()
{
    fd = open("/dev/zero", O_RDONLY);
}
#endif

/***********************************
 * Map memory.
 */

void *os_mem_map(unsigned nbytes)
{   void *p;

    errno = 0;
#if FDMAP
    p = mmap(NULL, nbytes, PROT_READ | PROT_WRITE, MAP_PRIVATE, OS_INIT::fd, 0);
#else
    p = mmap(NULL, nbytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
#endif
    return (p == MAP_FAILED) ? NULL : p;
}

/***********************************
 * Commit memory.
 * Returns:
 *	0	success
 *	!=0	failure
 */

int os_mem_commit(void *base, unsigned offset, unsigned nbytes)
{
    return 0;
}


/***********************************
 * Decommit memory.
 * Returns:
 *	0	success
 *	!=0	failure
 */

int os_mem_decommit(void *base, unsigned offset, unsigned nbytes)
{
    return 0;
}

/***********************************
 * Unmap memory allocated with os_mem_map().
 * Returns:
 *	0	success
 *	!=0	failure
 */

int os_mem_unmap(void *base, unsigned nbytes)
{
    return munmap(base, nbytes);
}




