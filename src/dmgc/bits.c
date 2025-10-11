
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

#include <assert.h>
#include <stdlib.h>

#include "bits.h"

GCBits::GCBits()
{
    data = NULL;
    nwords = 0;
    nbits = 0;
}

GCBits::~GCBits()
{
    if (data)
	::free(data);
    data = NULL;
}

void GCBits::invariant()
{
    if (data)
    {
	assert(nwords * sizeof(*data) * 8 >= nbits);
    }
}

void GCBits::alloc(unsigned nbits)
{
    this->nbits = nbits;
    nwords = (nbits + (BITS_PER_WORD - 1)) >> BITS_SHIFT;
    data = (unsigned *)::calloc(nwords + 2, sizeof(unsigned));
    assert(data);
}
