/* Digital Mars DMDScript source code.
 * Copyright (c) 2000-2002 by Chromium Communications
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

#include <objbase.h>

#include "port.h"
#include "root.h"
#include "dscript.h"
#include "value.h"
#include "dobject.h"
#include "text.h"
#include "dcomobject.h"

/* Some error codes:
 *	E_NOINTERFACE	0x80004002
 */


/* ===================== Dvbarray_constructor ==================== */

struct Dvbarray_constructor : Dfunction
{
    Dvbarray_constructor(ThreadContext *tc);

    void *Construct(CONSTRUCT_ARGS);
    void *Call(CALL_ARGS);
};

Dvbarray_constructor::Dvbarray_constructor(ThreadContext *tc)
    : Dfunction(1, tc->Dfunction_prototype)
{
}

void *Dvbarray_constructor::Call(CALL_ARGS)
{
    return Construct(cc, ret, argc, arglist);
}

void *Dvbarray_constructor::Construct(CONSTRUCT_ARGS)
{
    Dvbarray *o;
    Value *m;

    m = (argc) ? &arglist[0] : &vundefined;
    o = new Dvbarray(m);
    if (V_VT(&o->variant) != (VT_VARIANT | VT_ARRAY))
    {	ErrInfo errinfo;
	Value::copy(ret, &vundefined);
	return RuntimeError(&errinfo, ERR_VBARRAY_EXPECTED, m->getType());
    }

    Vobject::putValue(ret, o);
    return NULL;
}

/* ===================== Dvbarray_prototype_dimensions =============== */

/* MSDN says:
	"Returns the number of dimensions in a VBArray."
 */

BUILTIN_FUNCTION(Dvbarray_prototype_, dimensions, 0)
{
    //PRINTF("VBArray.prototype.dimensions()\n");
    Value::copy(ret, &vundefined);
    if (othis->isDvbarray())
    {	Dvbarray *e = (Dvbarray *)othis;
	int dims;

	if (V_VT(&e->variant) == (VT_VARIANT | VT_ARRAY))
	{
	    dims = SafeArrayGetDim(V_ARRAY(&e->variant));
	    Vnumber::putValue(ret, dims);
	}
    }
    return NULL;
}


/* ===================== Dvbarray_prototype_getItem =============== */

/* MSDN says:
	"Returns the item at the specified location.

	safeArray.getItem(dimension1[, dimension2, ...], dimensionn)

	dimension1, ..., dimensionn: Specifies the exact location of the
		desired element of the VBArray.
		n equals the number of dimensions in the VBArray."
 */

BUILTIN_FUNCTION(Dvbarray_prototype_, getItem, 0)
{
    //PRINTF("VBArray.prototype.getItem()\n");
    Value::copy(ret, &vundefined);
    if (othis->isDvbarray())
    {	Dvbarray *e = (Dvbarray *)othis;
	SAFEARRAY *safearray;

	if (V_VT(&e->variant) != (VT_VARIANT | VT_ARRAY))
	    goto Lerr;
	safearray = V_ARRAY(&e->variant);
	if (safearray->cDims != argc)
	    goto Lerr;

	long *indices = (long *)alloca(argc * sizeof(long));
	assert(indices);

	for (unsigned i = 0; i < argc; i++)
	{   long l;

	    l = (&arglist[i])->toInt32();
	    indices[i] = l;
	}

	HRESULT hResult;
	VARIANT var;

	VariantInit(&var);
	hResult = SafeArrayGetElement(safearray, indices, &var);
	if (FAILED(hResult))
	{   VariantClear(&var);
	    goto Lerr;
	}
	VariantToValue(ret, &var, NULL, NULL);
	VariantClear(&var);
    }
    return NULL;

Lerr:
    return NULL;
}


/* ===================== Dvbarray_prototype_lbound =============== */

/* MSDN says:
	"Returns the lowest index value used in the specified dimension of a VBArray.

	safeArray.lbound(dimension)

	dimension: Optional. The dimension of the VBArray for which the lower bound
	index is wanted. If omitted, lbound behaves as if a 1 was passed.

	If the VBArray is empty, the lbound method returns undefined.
	If dimension is greater than the number of dimensions in the VBArray,
	or is negative, the method generates a "Subscript out of range" error."
 */

BUILTIN_FUNCTION(Dvbarray_prototype_, lbound, 1)
{
    //PRINTF("VBArray.prototype.lbound()\n");
    Value::copy(ret, &vundefined);
    if (othis->isDvbarray())
    {	Dvbarray *e = (Dvbarray *)othis;
	long lbound;
	int dim;
	SAFEARRAY *safearray;
	HRESULT hResult;

	if (argc >= 1)
	{
	    dim = (&arglist[0])->toInt32();
	    if (dim < 0)
		goto Lerr;
	}
	else
	    dim = 1;

	if (V_VT(&e->variant) == (VT_VARIANT | VT_ARRAY))
	{
	    safearray = V_ARRAY(&e->variant);

	    if (dim > (int)SafeArrayGetDim(safearray))
		goto Lerr;

	    hResult = SafeArrayGetLBound(safearray, dim, &lbound);
	    if (!FAILED(hResult))
		Vnumber::putValue(ret, lbound);
	}
    }
    return NULL;

Lerr:
    ErrInfo errinfo;
    return RuntimeError(&errinfo, ERR_VBARRAY_SUBSCRIPT);
}


/* ===================== Dvbarray_prototype_toArray =============== */

/* MSDN says:
	"Returns a standard JScript array converted from a VBArray.

	safeArray.toArray( ) 

	The conversion translates the multidimensional VBArray into a single
	dimensional JScript array. Each successive dimension is appended to
	the end of the previous one. For example, a VBArray with three dimensions
	and three elements in each dimension is converted into a JScript array
	as follows:
	Suppose the VBArray contains: (1, 2, 3), (4, 5, 6), (7, 8, 9).
	After translation, the JScript array contains: 1, 2, 3, 4, 5, 6, 7, 8, 9. 

	There is currently no way to convert a JScript array into a VBArray."
 */

BUILTIN_FUNCTION(Dvbarray_prototype_, toArray, 0)
{
    //PRINTF("VBArray.prototype.toArray()\n");
    Value::copy(ret, &vundefined);
    if (othis->isDvbarray())
    {	Dvbarray *e = (Dvbarray *)othis;
	unsigned nitems;
	unsigned dims;
	unsigned i;
	unsigned u;
	HRESULT hResult;
	Value v;
	long *indices;
	long *lbounds;
	long *ubounds;
	SAFEARRAY *safearray;

	if (V_VT(&e->variant) != (VT_VARIANT | VT_ARRAY))
	    goto Lerr;
	safearray = V_ARRAY(&e->variant);

	// Get all the indices
	dims = SafeArrayGetDim(safearray);
	indices = (long *)alloca(dims * sizeof(long));
	assert(indices);
	lbounds = (long *)alloca(dims * sizeof(long));
	assert(lbounds);
	ubounds = (long *)alloca(dims * sizeof(long));
	assert(ubounds);

	for (i = 1; i <= dims; i++)
	{
	    hResult = SafeArrayGetLBound(safearray, i, &lbounds[i - 1]);
	    if (FAILED(hResult))
		goto Lerr;

	    hResult = SafeArrayGetUBound(safearray, i, &ubounds[i - 1]);
	    if (FAILED(hResult))
		goto Lerr;
	}

	// Count the number of elements in the safe array.
	nitems = 0;
	for (i = 0; i < dims; i++)
	{   long range;

	    range = (ubounds[i] - lbounds[i]) + 1;
	    if (i == 0)
		nitems = range;
	    else
		nitems *= range;
	}

	// Create the array
	Darray *a = new Darray();
	a->Put(TEXT_length, nitems, 0);

	// Stuff the array
	if (nitems)
	{
#if 1
	    // Accessing array data directly should be much faster
	    VARIANT *pvar;

	    hResult = SafeArrayAccessData(safearray, (void **)&pvar);
	    if (FAILED(hResult))
		goto Lerr;
	    for (u = 0; u < nitems; u++)
	    {
		VariantToValue(&v, &pvar[u], NULL, NULL);
		a->Put(u, &v, 0);
	    }
	    SafeArrayUnaccessData(safearray);
#else
	    memcpy(indices, lbounds, dims * sizeof(long));
	    for (u = 0; u < nitems; u++)
	    {
		VARIANT var;

		VariantInit(&var);
		hResult = SafeArrayGetElement(safearray, indices, &var);
		if (FAILED(hResult))
		{   VariantClear(&var);
		    goto Lerr;
		}
		VariantToValue(&v, &var, NULL, NULL);
		VariantClear(&var);
		a->Put(u, &v, 0);

		for (i = 0; i < dims; i++)
		{
		    indices[i]++;
		    if (indices[i] <= ubounds[i])
			break;
		    indices[i] = lbounds[i];
		}
	    }
#endif
	}
	Vobject::putValue(ret, a);
    }
    return NULL;

Lerr:
    //WPRINTF(L"toArray() error\n");
    return NULL;
}


/* ===================== Dvbarray_prototype_ubound =============== */

/* MSDN says:
	"Returns the highest index value used in the specified dimension
	of the VBArray.

	safeArray.ubound(dimension) 

	dimension: Optional. The dimension of the VBArray for which the higher
	bound index is wanted. If omitted, ubound behaves as if a 1 was passed. 

	If the VBArray is empty, the ubound method returns undefined.
	If dim is greater than the number of dimensions in the VBArray,
	or is negative, the method generates a "Subscript out of range" error."
 */

BUILTIN_FUNCTION(Dvbarray_prototype_, ubound, 1)
{
    //PRINTF("VBArray.prototype.ubound()\n");
    Value::copy(ret, &vundefined);
    if (othis->isDvbarray())
    {	Dvbarray *e = (Dvbarray *)othis;
	long ubound;
	int dim;
	SAFEARRAY *safearray;
	HRESULT hResult;

	if (argc >= 1)
	{
	    dim = (&arglist[0])->toInt32();
	    if (dim < 0)
		goto Lerr;
	}
	else
	    dim = 1;

	if (V_VT(&e->variant) == (VT_VARIANT | VT_ARRAY))
	{
	    safearray = V_ARRAY(&e->variant);

	    if (dim > (int)SafeArrayGetDim(safearray))
		goto Lerr;

	    hResult = SafeArrayGetUBound(safearray, dim, &ubound);
	    if (!FAILED(hResult))
		Vnumber::putValue(ret, ubound);
	}
    }
    return NULL;

Lerr:
    ErrInfo errinfo;
    return RuntimeError(&errinfo, ERR_VBARRAY_SUBSCRIPT);
}


/* ===================== Dvbarray_prototype ==================== */

struct Dvbarray_prototype : Dvbarray
{
    Dvbarray_prototype(ThreadContext *tc);
};

Dvbarray_prototype::Dvbarray_prototype(ThreadContext *tc)
    : Dvbarray(tc->Dobject_prototype)
{
    Dobject *f = tc->Dfunction_prototype;

    Put(TEXT_constructor, tc->Dvbarray_constructor, DontEnum);
    Put(TEXT_dimensions, new(this) Dvbarray_prototype_dimensions(f), DontEnum);
    Put(TEXT_getItem, new(this) Dvbarray_prototype_getItem(f), DontEnum);
    Put(TEXT_lbound, new(this) Dvbarray_prototype_lbound(f), DontEnum);
    Put(TEXT_toArray, new(this) Dvbarray_prototype_toArray(f), DontEnum);
    Put(TEXT_ubound, new(this) Dvbarray_prototype_ubound(f), DontEnum);
}

/* ===================== Dvbarray ==================== */

Dvbarray::Dvbarray(Value *m)
    : Dvariant(Dvbarray::getPrototype(), NULL)
{
    classname = TEXT_VBArray;
    isLambda = 3;
    VariantInit(&variant);

    if (!m->isPrimitive())
    {
	Dobject *o = m->toObject();
	if (o->isDvariant() || o->isDvbarray())
	{   Dvariant *dv = (Dvariant *)o;
	    HRESULT hResult;

	    if (V_VT(&dv->variant) == (VT_VARIANT | VT_ARRAY))
	    {
		hResult = VariantCopy(&variant, &dv->variant);
		if (FAILED(hResult))
		{
		    //WPRINTF(L"VariantCopy failed, hResult = %x\n", hResult);
		}
	    }
	}
    }
}

Dvbarray::Dvbarray(VARIANT *v)
    : Dvariant(Dvbarray::getPrototype(), v)
{
    //WPRINTF(L"Dvbarray::Dvbarray(VARIANT %p)\n", v);
    classname = TEXT_VBArray;
}

Dvbarray::Dvbarray(Dobject *prototype)
    : Dvariant(prototype, NULL)
{
    classname = TEXT_VBArray;
}

d_string Dvbarray::getTypeof()
{
    return TEXT_unknown;
}

Dfunction *Dvbarray::getConstructor()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Dvbarray_constructor;
}

Dobject *Dvbarray::getPrototype()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Dvbarray_prototype;
}

void Dvbarray_init(ThreadContext *tc)
{
    GC_LOG();
    tc->Dvbarray_constructor = new(&tc->mem) Dvbarray_constructor(tc);
    GC_LOG();
    tc->Dvbarray_prototype = new Dvbarray_prototype(tc);

    tc->Dvbarray_constructor->Put(TEXT_prototype, tc->Dvbarray_prototype, DontEnum | DontDelete | ReadOnly);
}

void Dvbarray_add(Dobject *o)
{
    // o is really the global object.

    o->Put(TEXT_VBArray, Dvbarray::getConstructor(), DontEnum);
}

Dobject *Dvbarray_ctor(Value *m)
{
    return new Dvbarray(m);
}

