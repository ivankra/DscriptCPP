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

// Bit operations for MSC and I386

#ifndef MSCBITOPS_H
#define MSCBITOPS_H 1

inline int _inline_bsf(int w)
{   int index;

    index = 0;
    while (!(w & 1))
    {	index++;
	w >>= 1;
    }
    return index;
}

#endif
