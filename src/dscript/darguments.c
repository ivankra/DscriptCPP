
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

#include "root.h"
#include "dscript.h"
#include "value.h"
#include "dobject.h"
#include "text.h"


// The purpose of Darguments is to implement "value sharing"
// per ECMA 10.1.8 between the activation object and the
// arguments object.
// We implement it by forwarding the property calls from the
// arguments object to the activation object.

Darguments::Darguments(Dobject *caller, Dobject *callee, Dobject *actobj,
	Array *parameters, unsigned argc, Value *arglist)

    : Dobject(Dobject::getPrototype())
{   unsigned a;

    this->actobj = actobj;
    this->nparams = parameters ? parameters->dim : 0;
    this->parameters = parameters;

    if (caller)
	Put(TEXT_caller, caller, DontEnum);
    else
	Put(TEXT_caller, &vnull, DontEnum);

    Put(TEXT_callee, callee, DontEnum);
    Put(TEXT_length, argc, DontEnum);

    for (a = 0; a < argc; a++)
    {
	Put(a, &arglist[a], DontEnum);
    }
}

/************************
 * Convert d_string to an index, if it is one.
 * Returns:
 *	TRUE	it's an index, and *index is set
 *	FALSE	it's not an index
 */

int StringToIndex(d_string name, d_uint32 *index)
{
    d_uint32 i;
    dchar *p;

    i = 0;
    for (p = d_string_ptr(name); ; p++)
    {
	switch (*p)
	{
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
		if ((i == 0 && p > d_string_ptr(name)) ||	// if leading zeros
		    i >= 0xFFFFFFFF / 10)			// or overflow
		    goto Lnotindex;
		i = i * 10 + *p - '0';
		break;

	    case 0:
		if (p > d_string_ptr(name))
		{
		    *index = i;
		    return TRUE;
		}
		goto Lnotindex;

	    default:
	    Lnotindex:
		return FALSE;
	}
    }
}

Value *Darguments::Get(d_string PropertyName)
{
    d_uint32 index;

    return (StringToIndex(PropertyName, &index) && index < nparams)
	? actobj->Get(index)
	: Dobject::Get(PropertyName);
}

Value *Darguments::Get(d_uint32 index)
{
    return (index < nparams)
	? actobj->Get(index)
	: Dobject::Get(index);
}

Value *Darguments::Put(d_string PropertyName, Value *value, unsigned attributes)
{
    d_uint32 index;

    if (StringToIndex(PropertyName, &index) && index < nparams)
	return actobj->Put(PropertyName, value, attributes);
    else
	return Dobject::Put(PropertyName, value, attributes);
}

Value *Darguments::Put(d_string PropertyName, Dobject *o, unsigned attributes)
{
    d_uint32 index;

    if (StringToIndex(PropertyName, &index) && index < nparams)
	return actobj->Put(PropertyName, o, attributes);
    else
	return Dobject::Put(PropertyName, o, attributes);
}

Value *Darguments::Put(d_string PropertyName, d_number n, unsigned attributes)
{
    d_uint32 index;

    if (StringToIndex(PropertyName, &index) && index < nparams)
	return actobj->Put(PropertyName, n, attributes);
    else
	return Dobject::Put(PropertyName, n, attributes);
}

Value *Darguments::Put(d_uint32 index, Value *value, unsigned attributes)
{
    if (index < nparams)
	return actobj->Put(index, value, attributes);
    else
	return Dobject::Put(index, value, attributes);
}

int Darguments::CanPut(d_string PropertyName)
{
    d_uint32 index;

    return (StringToIndex(PropertyName, &index) && index < nparams)
	? actobj->CanPut(PropertyName)
	: Dobject::CanPut(PropertyName);
}

int Darguments::HasProperty(d_string PropertyName)
{
    d_uint32 index;

    return (StringToIndex(PropertyName, &index) && index < nparams)
	? actobj->HasProperty(PropertyName)
	: Dobject::HasProperty(PropertyName);
}

int Darguments::Delete(d_string PropertyName)
{
    d_uint32 index;

    return (StringToIndex(PropertyName, &index) && index < nparams)
	? actobj->Delete(PropertyName)
	: Dobject::Delete(PropertyName);
}


