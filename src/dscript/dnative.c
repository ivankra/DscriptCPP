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
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if _MSC_VER
#include <malloc.h>
#endif

#include "root.h"
#include "dscript.h"
#include "value.h"
#include "dobject.h"
#include "statement.h"
#include "parse.h"
#include "scope.h"
#include "text.h"


/******************* DnativeFunction ****************************/


DnativeFunction::DnativeFunction(PCall func, char *name, d_uint32 length)
    : Dfunction(length)
{
    this->name = name;
    pcall = func;
}

DnativeFunction::DnativeFunction(PCall func, char *name, d_uint32 length, Dobject *o)
    : Dfunction(length, o)
{
    this->name = name;
    pcall = func;
}

void *DnativeFunction::Call(CALL_ARGS)
{
    return (*pcall)(this, cc, othis, ret, argc, arglist);
}

/*********************************
 * Initalize table of native functions designed
 * to go in as properties of o.
 */

void DnativeFunction::init(Dobject *o, NativeFunctionData *nfd, int nfd_dim, unsigned attributes)
{
    Dobject *f = Dfunction::getPrototype();

    // Using new(m) instead of new(this) saves the NULL check for this
    Mem *m = o;

    for (int i = 0; i < nfd_dim; i++)
    {
	o->Put(	*nfd->string,
		new(m) DnativeFunction(nfd->pcall, nfd->name, nfd->length, f),
		attributes);
	nfd++;
    }
}

