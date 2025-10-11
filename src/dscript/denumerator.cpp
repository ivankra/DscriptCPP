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

#include <objbase.h>

#include "port.h"
#include "root.h"
#include "dscript.h"
#include "value.h"
#include "dobject.h"
#include "text.h"
#include "dcomobject.h"

#define LOG 0

/* Some error codes:
 *	E_NOINTERFACE	0x80004002
 */


/* ===================== Denumerator_constructor ==================== */

struct Denumerator_constructor : Dfunction
{
    Denumerator_constructor(ThreadContext *tc);

    void *Construct(CONSTRUCT_ARGS);
    void *Call(CALL_ARGS);
};

Denumerator_constructor::Denumerator_constructor(ThreadContext *tc)
    : Dfunction(1, tc->Dfunction_prototype)
{
}

void *Denumerator_constructor::Call(CALL_ARGS)
{
    return Construct(cc, ret, argc, arglist);
}

void *Denumerator_constructor::Construct(CONSTRUCT_ARGS)
{
    Dobject *o;
    Value *m;

    m = (argc) ? &arglist[0] : &vundefined;
    if (m->isPrimitive() ||
	!m->toObject()->isDcomobject())
    {	ErrInfo errinfo;
	return RuntimeError(&errinfo, ERR_NOT_COLLECTION);
    }
    o = new Denumerator(m);
    Vobject::putValue(ret, o);
    return NULL;
}

/* ===================== Denumerator_prototype_item =============== */

/* MSDN says:
	"The item method returns the current item.
	 If the collection is empty or the current item is undefined,
	 it returns undefined."
 */

BUILTIN_FUNCTION(Denumerator_prototype_, item, 0)
{
    //PRINTF("Enumerator.prototype.item()\n");
    Value::copy(ret, &vundefined);
    if (othis->isDenumerator())
    {	Denumerator *e = (Denumerator *)othis;

	if (!e->isItemCurrent)
	    e->getNextItem();
	Value::copy(ret, &e->currentItem);
    }
    return NULL;
}

/* ===================== Denumerator_prototype_atEnd =============== */

/* MSDN says:
	"The atEnd method returns true if the current item is the last one
	 in the collection, the collection is empty, or the current item is
	 undefined. Otherwise, it returns false."

   MSDN is wrong, because its own examples wouldn't work. More correctly,
   atEnd() tests if moveNext() has gone off the end of the array.
 */

BUILTIN_FUNCTION(Denumerator_prototype_, atEnd, 0)
{   int result = TRUE;

    //WPRINTF(L"Enumerator.prototype.atEnd()\n");
    if (othis->isDenumerator())
    {	Denumerator *e = (Denumerator *)othis;

	if (!e->isItemCurrent)
	    e->getNextItem();
	if (!(&e->currentItem)->isUndefined())
	    result = FALSE;
    }
    Vboolean::putValue(ret, result);
    return NULL;
}

/* ===================== Denumerator_prototype_moveFirst =============== */

/* MSDN says:
	"Resets the current item in the collection to the first item.
	 If there are no items in the collection,
	 the current item is set to undefined."
 */

BUILTIN_FUNCTION(Denumerator_prototype_, moveFirst, 0)
{
    //WPRINTF(L"Enumerator.prototype.moveFirst()\n");
    if (othis->isDenumerator())
    {	Denumerator *e = (Denumerator *)othis;


	if (e->mEnum)
	    e->mEnum->Reset();
	e->getNextItem();
    }
    Value::copy(ret, &vundefined);
    return NULL;
}

/* ===================== Denumerator_prototype_moveNext =============== */

/* MSDN says:
	"Moves the current item to the next item in the collection.
	 If the enumerator is at the end of the collection or the
	 collection is empty, the current item is set to undefined."
 */

BUILTIN_FUNCTION(Denumerator_prototype_, moveNext, 0)
{
    //WPRINTF(L"Enumerator.prototype.moveNext()\n");
    if (othis->isDenumerator())
    {	Denumerator *e = (Denumerator *)othis;

	e->getNextItem();
    }
    Value::copy(ret, &vundefined);
    return NULL;
}

/* ===================== Denumerator_prototype ==================== */

struct Denumerator_prototype : Denumerator
{
    Denumerator_prototype(ThreadContext *tc);
};

Denumerator_prototype::Denumerator_prototype(ThreadContext *tc)
    : Denumerator(tc->Dobject_prototype)
{
    Dobject *f = tc->Dfunction_prototype;

    Put(TEXT_constructor, tc->Denumerator_constructor, DontEnum);
    Put(TEXT_atEnd, new(this) Denumerator_prototype_atEnd(f), DontEnum);
    Put(TEXT_item, new(this) Denumerator_prototype_item(f), DontEnum);
    Put(TEXT_moveFirst, new(this) Denumerator_prototype_moveFirst(f), DontEnum);
    Put(TEXT_moveNext, new(this) Denumerator_prototype_moveNext(f), DontEnum);
}

/* ===================== Denumerator ==================== */

Denumerator::Denumerator(Value *m)
    : Dobject(Denumerator::getPrototype())
{
#if LOG
    FuncLog funclog(L"Denumerator()");
    WPRINTF(L"\tm = %x, '%s'\n", m, m ? d_string_ptr(m->toString()) : L"");
    if (m)
	WPRINTF(L"\tm = '%s'\n", d_string_ptr(m->getTypeof()));
#endif
    classname = TEXT_Enumerator;
    isLambda = 2;
    vc = m;
    mEnum = NULL;
    isItemCurrent = 0;
    Value::copy(&currentItem, &vundefined);
    if (!vc->isPrimitive())
    {	Dobject *o = vc->toObject();
	IDispatch *id = o->toComObject();

#if LOG
	WPRINTF(L"\to = %x, id = %x\n", o, id);
#endif
	if (id)
	{
	    // Many thanks to Pat Nelson for figuring this out
	    HRESULT hResult = E_UNEXPECTED;
	    VARIANT Result;

	    VariantInit(&Result);
	    DISPPARAMS DispParamsEmpty;
	    memset(&DispParamsEmpty, 0, sizeof(DispParamsEmpty));

	    //get prop DISPID_NEWENUM
	    hResult = id->Invoke(
		DISPID_NEWENUM, GUID_NULL, LOCALE_SYSTEM_DEFAULT,
		DISPATCH_PROPERTYGET|DISPATCH_METHOD, &DispParamsEmpty,
		&Result, NULL, NULL);
	    if (SUCCEEDED(hResult) && V_VT(&Result) == VT_UNKNOWN)
	    {
		IUnknown* pdisp = V_UNKNOWN(&Result);

		if (pdisp)
		{
		    //get IEnumVARIANT*
		    hResult = pdisp->QueryInterface(IID_IEnumVARIANT, (void**)&mEnum);
		    if (SUCCEEDED(hResult))
		    {
#if LOG
			WPRINTF(L"\tAble to get IID_IEnumVARIANT interface\n");
#endif
			if (mEnum)
			    mEnum->Reset();
		    }
		    else
		    {
#if LOG
			WPRINTF(L"\tFailed to get IID_IEnumVARIANT interface\n");
#endif
		    }
		}
		VariantClear(&Result);
	    }
	    else
	    {
#if LOG
		WPRINTF(L"\tid->Invoke() failed, hr = x%x\n", hResult);
#endif
	    }
	}
    }
}

Denumerator::Denumerator(Dobject *prototype)
    : Dobject(prototype)
{
    classname = TEXT_Enumerator;
    isLambda = 2;
    vc = NULL;
    mEnum = NULL;
    isItemCurrent = 0;
    Value::copy(&currentItem, &vundefined);
}

Dfunction *Denumerator::getConstructor()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Denumerator_constructor;
}

Dobject *Denumerator::getPrototype()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Denumerator_prototype;
}

void Denumerator_init(ThreadContext *tc)
{
    GC_LOG();
    tc->Denumerator_constructor = new(&tc->mem) Denumerator_constructor(tc);
    GC_LOG();
    tc->Denumerator_prototype = new Denumerator_prototype(tc);

    tc->Denumerator_constructor->Put(TEXT_prototype, tc->Denumerator_prototype, DontEnum | DontDelete | ReadOnly);
}

void Denumerator_add(Dobject *o)
{
    // o is really the global object.

    o->Put(TEXT_Enumerator, Denumerator::getConstructor(), DontEnum);
}


void * Denumerator::operator new(size_t m_size)
{
    void *p = Dobject::operator new(m_size);

    if (p)
        mem.setFinalizer(p, DoFinalize, NULL);
    return p;
}

void Denumerator::finalizer()
{
    //WPRINTF(L"Denumerator::finalizer(mEnum = %x)\n", mEnum);
    if (mEnum) {
	mEnum->Release();
        mEnum = NULL;
    }
}

void Denumerator::getNextItem()
{
    HRESULT hr;
    ULONG count = 0;
    VARIANT vfirst;

    isItemCurrent = 0;
    Value::copy(&currentItem, &vundefined);
    if (mEnum)
    {
	VariantInit(&vfirst);
	hr = mEnum->Next(1, &vfirst, &count);
	if (SUCCEEDED(hr) && count == 1)
	{
	    VariantToValue(&currentItem, &vfirst, d_string_ctor(L""), NULL);
	    VariantClear(&vfirst);
	    isItemCurrent = 1;
	}
    }
}

