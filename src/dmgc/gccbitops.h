/* Digital Mars DMDScript source code.
 * Copyright (c) 2002-2007 by Digital Mars
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

// Bit operations for GCC and I386

#ifndef GCCBITOPS_H
#define GCCBITOPS_H 1

inline int _inline_bsf(int w)
{   int index;

    __asm__ __volatile__
    (
	"bsfl %1, %0 \n\t"
	: "=r" (index)
	: "r" (w)
    );
    return index;
}


inline int _inline_bt(unsigned *p, int i)
{
    char result;

    __asm__ __volatile__
    (
	"btl %2,%1	\n\t"
	"setc %0	\n\t"
	:"=r" (result)
	:"m" (*p), "r" (i)
    );
    return result;
}

inline int _inline_bts(unsigned *p, int i)
{
    char result;

    __asm__ __volatile__
    (
	"btsl %2,%1	\n\t"
	"setc %0	\n\t"
	:"=r" (result)
	:"m" (*p), "r" (i)
    );
    return result;
}

inline int _inline_btr(unsigned *p, int i)
{
    char result;

    __asm__ __volatile__
    (
	"btrl %2,%1	\n\t"
	"setc %0	\n\t"
	:"=r" (result)
	:"m" (*p), "r" (i)
    );
    return result;
}

#endif
