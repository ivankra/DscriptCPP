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

// OS specific routines

void *os_mem_map(unsigned nbytes);
int os_mem_commit(void *base, unsigned offset, unsigned nbytes);
int os_mem_decommit(void *base, unsigned offset, unsigned nbytes);
int os_mem_unmap(void *base, unsigned nbytes);


// Threading

#if defined linux
#include <pthread.h>
#else
typedef long pthread_t;
pthread_t pthread_self();
#endif
