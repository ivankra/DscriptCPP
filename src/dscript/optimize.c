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
#include <stdarg.h>
#include <assert.h>

#if _MSC_VER
#include <malloc.h>
#endif

#include "root.h"
#include "lstring.h"
#include "mem.h"

#include "dscript.h"
#include "ir.h"

void IRstate::optimize()
{
    // Determine the length of the code array

    IR *c;
    IR *c2;
    IR *code;
    unsigned length;
    unsigned i;

    code = (IR *) codebuf->data;
    for (c = code; c->op.opcode != IRend; c += IR::size(c->op.opcode))
	;
    length = c - code + 1;

    // Allocate a bit vector for the array
    Bits b;
    b.resize(length);

    // Set bit for each target of a jump
    b.set(0);				// entry point
    for (c = code; c->op.opcode != IRend; c += IR::size(c->op.opcode))
    {
	switch (c->op.opcode)
	{
	    case IRjf:
	    case IRjt:
	    case IRjfb:
	    case IRjtb:
	    case IRjmp:
	    case IRjlt:
	    case IRjle:
	    case IRjltc:
	    case IRjlec:
	    case IRtrycatch:
	    case IRtryfinally:
		//PRINTF("set %d\n", (c - code) + (c + 1)->offset);
		b.set((c - code) + (c + 1)->offset);
		break;
#if 0
		b.set((c + 1)->offset);
		break;
#endif
            default:
                break;
	}
    }

    // Allocate array of IR contents for locals
    IR **local;
    void *p1 = NULL;
    if (nlocals >= 128 || (local = (IR **)alloca(nlocals * sizeof(local[0]))) == NULL)
    {	p1 = mem.malloc(nlocals * sizeof(local[0]));
	local = (IR **)p1;
    }

    // Optimize
    for (c = code; c->op.opcode != IRend; c += IR::size(c->op.opcode))
    {
	unsigned offset = (c - code);

	if (b.test(offset))	// if target of jump
	{
	    // Reset contents of locals
	    for (i = 0; i < nlocals; i++)
		local[i] = NULL;
	}

	switch (c->op.opcode)
	{
	    case IRnop:
		break;

	    case IRnumber:
	    case IRstring:
	    case IRboolean:
		local[(c + 1)->index / INDEX_FACTOR] = c;
		break;

	    case IRadd:
	    case IRsub:
	    case IRcle:
		local[(c + 1)->index / INDEX_FACTOR] = c;
		break;

	    case IRputthis:
		local[(c + 1)->index / INDEX_FACTOR] = c;
		goto Lreset;

	    case IRputscope:
		local[(c + 1)->index / INDEX_FACTOR] = c;
		break;

	    case IRgetscope:
	    {
		d_string cs = (c + 2)->string;
		IR *cimax = NULL;
		for (i = nlocals; i--; )
		{
		    IR *ci = local[i];
		    if (ci &&
			(ci->op.opcode == IRgetscope || ci->op.opcode == IRputscope) &&
			((ci + 2)->string == cs ||
			 d_string_cmp((ci + 2)->string, cs) == 0
			))
		    {
			if (cimax)
			{
			    if (cimax < ci)
				cimax = ci;	// select most recent instruction
			}
			else
			    cimax = ci;
		    }
		}
		if (cimax)
		{
		    //PRINTF("IRgetscope -> IRmov %d, %d\n", (c + 1)->index, (cimax + 1)->index);
		    c->op.opcode = IRmov;
		    (c + 3)->op.opcode = IRnop;
		    (c + 2)->index = (cimax + 1)->index;
		    local[(c + 1)->index / INDEX_FACTOR] = cimax;
		}
		else
		    local[(c + 1)->index / INDEX_FACTOR] = c;
		break;
	    }

	    case IRnew:
		local[(c + 1)->index / INDEX_FACTOR] = c;
		goto Lreset;

	    case IRcallscope:
	    case IRputcall:
	    case IRputcalls:
	    case IRputcallscope:
	    case IRputcallv:
	    case IRcallv:
		local[(c + 1)->index / INDEX_FACTOR] = c;
		goto Lreset;

	    case IRmov:
		local[(c + 1)->index / INDEX_FACTOR] = local[(c + 2)->index / INDEX_FACTOR];
		break;

	    case IRput:
	    case IRpostincscope:
	    case IRaddassscope:
		goto Lreset;

	    case IRjf:
	    case IRjfb:
	    case IRjtb:
	    case IRjmp:
	    case IRjt:
	    case IRret:
	    case IRjlt:
	    case IRjle:
	    case IRjltc:
	    case IRjlec:
		break;

	    default:
	    Lreset:
		// Reset contents of locals
		for (i = 0; i < nlocals; i++)
		    local[i] = NULL;
		break;
	}
    }

    mem.free(p1);

//return;
    // Remove all IRnop's
    for (c = code; c->op.opcode != IRend; )
    {
	unsigned offset;
	unsigned o;
	unsigned c2off;

	if (c->op.opcode == IRnop)
	{
	    offset = (c - code);
	    for (c2 = code; c2->op.opcode != IRend; c2 += IR::size(c2->op.opcode))
	    {
		switch (c2->op.opcode)
		{
		    case IRjf:
		    case IRjt:
		    case IRjfb:
		    case IRjtb:
		    case IRjmp:
		    case IRjlt:
		    case IRjle:
		    case IRjltc:
		    case IRjlec:
		    case IRnextscope:
		    case IRtryfinally:
		    case IRtrycatch:
			c2off = c2 - code;
			o = c2off + (c2 + 1)->offset;
			if (c2off <= offset && offset < o)
			    (c2 + 1)->offset--;
			else if (c2off > offset && o <= offset)
			    (c2 + 1)->offset++;
			break;
#if 0
		    case IRtrycatch:
			o = (c2 + 1)->offset;
			if (offset < o)
			    (c2 + 1)->offset--;
			break;
#endif
		    default:
			continue;
		}
	    }

	    length--;
	    memmove(c, c + 1, (length - offset) * sizeof(unsigned));
	}
	else
	    c += IR::size(c->op.opcode);
    }
}
