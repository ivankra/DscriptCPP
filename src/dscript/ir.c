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

#include "root.h"
#include "dscript.h"
#include "ir.h"
#include "statement.h"

IRstate::IRstate()
{
    current = NULL;
    locali = 1;		// leave location 0 as our "null"
    nlocals = 1;
    codebuf = new OutBuffer();
    breakTarget = NULL;
    continueTarget = NULL;
    scopeContext = NULL;
    fixups.reserve(10);
}

/**********************************
 * Allocate a block of local variables, and return an
 * index to them.
 */

unsigned IRstate::alloc(unsigned nlocals)
{
    unsigned n;

    n = locali;
    locali += nlocals;
    if (locali > this->nlocals)
	this->nlocals = locali;
    assert(n);
    return n * INDEX_FACTOR;
}

/****************************************
 * Release this block of n locals starting at local.
 */

void IRstate::release(unsigned local, unsigned n)
{
#if 0
    local /= INDEX_FACTOR;
    if (local + n == locali)
	locali = local;
#endif
}

unsigned IRstate::mark()
{
    return locali;
}

void IRstate::release(unsigned i)
{
#if 0
    locali = i;
#endif
}

#define combine(loc, opcode)	((loc << 16) | opcode)

/***************************************
 * Generate code.
 */

void IRstate::gen0(Loc loc, unsigned opcode)
{
    codebuf->write4(combine(loc, opcode));
}

void IRstate::gen1(Loc loc, unsigned opcode, unsigned arg)
{
    codebuf->reserve(2 * sizeof(unsigned));
#if 1
    // Inline ourselves for speed (compiler doesn't do a good job)
    unsigned *data = (unsigned *)(codebuf->data + codebuf->offset);
    codebuf->offset += 2 * sizeof(unsigned);
    data[0] = combine(loc, opcode);
    data[1] = arg;
#else
    codebuf->write4n(combine(loc, opcode));
    codebuf->write4n(arg);
#endif
}

void IRstate::gen2(Loc loc, unsigned opcode, unsigned arg1, unsigned arg2)
{
    codebuf->reserve(3 * sizeof(unsigned));
#if 1
    // Inline ourselves for speed (compiler doesn't do a good job)
    unsigned *data = (unsigned *)(codebuf->data + codebuf->offset);
    codebuf->offset += 3 * sizeof(unsigned);
    data[0] = combine(loc, opcode);
    data[1] = arg1;
    data[2] = arg2;
#else
    codebuf->write4n(combine(loc, opcode));
    codebuf->write4n(arg1);
    codebuf->write4n(arg2);
#endif
}

void IRstate::gen3(Loc loc, unsigned opcode, unsigned arg1, unsigned arg2, unsigned arg3)
{
    codebuf->reserve(4 * sizeof(unsigned));
#if 1
    // Inline ourselves for speed (compiler doesn't do a good job)
    unsigned *data = (unsigned *)(codebuf->data + codebuf->offset);
    codebuf->offset += 4 * sizeof(unsigned);
    data[0] = combine(loc, opcode);
    data[1] = arg1;
    data[2] = arg2;
    data[3] = arg3;
#else
    codebuf->write4n(combine(loc, opcode));
    codebuf->write4n(arg1);
    codebuf->write4n(arg2);
    codebuf->write4n(arg3);
#endif
}

void IRstate::gen4(Loc loc, unsigned opcode, unsigned arg1, unsigned arg2, unsigned arg3, unsigned arg4)
{
    codebuf->reserve(5 * sizeof(unsigned));
#if 1
    // Inline ourselves for speed (compiler doesn't do a good job)
    unsigned *data = (unsigned *)(codebuf->data + codebuf->offset);
    codebuf->offset += 5 * sizeof(unsigned);
    data[0] = combine(loc, opcode);
    data[1] = arg1;
    data[2] = arg2;
    data[3] = arg3;
    data[4] = arg4;
#else
    codebuf->write4n(combine(loc, opcode));
    codebuf->write4n(arg1);
    codebuf->write4n(arg2);
    codebuf->write4n(arg3);
    codebuf->write4n(arg4);
#endif
}

void IRstate::gen(Loc loc, unsigned opcode, unsigned argc, ...)
{
    va_list ap;

    codebuf->reserve((1 + argc) * 4);
    codebuf->write4n(combine(loc, opcode));
    va_start(ap, argc);
    for (unsigned i = 0; i < argc; i++)
    {
	codebuf->write4n(va_arg(ap, int));
    }
    va_end(ap);
}

void IRstate::pops(unsigned npops)
{
    while (npops--)
	gen0(0, IRpop);
}

/******************************
 * Get the current "instruction pointer"
 */

unsigned IRstate::getIP()
{
    if (!codebuf)
	return 0;
    return codebuf->offset / 4;
}

/******************************
 * Patch a value into the existing codebuf.
 */

void IRstate::patchJmp(unsigned index, unsigned value)
{
    ((unsigned long *)(codebuf->data))[index + 1] = value - index;
}

/*******************************
 * Add this IP to list of jump instructions to patch.
 */

void IRstate::addFixup(unsigned index)
{
    fixups.push((void *)index);
}

/*******************************
 * Go through the list of fixups and patch them.
 */

void IRstate::doFixups()
{
    unsigned i;
    unsigned index;
    unsigned value;
    Statement *s;

    for (i = 0; i < fixups.dim; i++)
    {
	index = (unsigned) fixups.data[i];
	s = ((Statement **)codebuf->data)[index + 1];
	value = s->getTarget();
	patchJmp(index, value);
    }
}
