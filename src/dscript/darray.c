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
#include <ctype.h>
#include <assert.h>

#if _MSC_VER
#include <malloc.h>
#endif

#include "port.h"
#include "root.h"
#include "dchar.h"
#include "mutex.h"

#include "dscript.h"
#include "value.h"
#include "dobject.h"
#include "text.h"
#include "program.h"

/* ===================== Darray_constructor ==================== */

struct Darray_constructor : Dfunction
{
    Darray_constructor(ThreadContext *tc);

    void *Construct(CONSTRUCT_ARGS);
    void *Call(CALL_ARGS);
};

Darray_constructor::Darray_constructor(ThreadContext *tc)
    : Dfunction(1, tc->Dfunction_prototype)
{
    name = "Array";
}

void *Darray_constructor::Call(CALL_ARGS)
{
    // ECMA 15.4.1
    return Construct(cc, ret, argc, arglist);
}

void *Darray_constructor::Construct(CONSTRUCT_ARGS)
{
    // ECMA 15.4.2
    Darray *a;

    GC_LOG();
    a = new(this) Darray();
    if (argc == 0)
    {
	a->length.number = 0;
    }
    else if (argc == 1)
    {
	Value *v = &arglist[0];
	Vnumber *n;

	if (v->isNumber())
	{
	    d_uint32 len;

	    n = (Vnumber *)v;
	    len = n->toUint32();
#if __DMC__ && __DMC__ < 0x828
    #error unsigned to double conversion not correct
#endif
	    if ((double) len != n->number)
	    {	ErrInfo errinfo;

		Value::copy(ret, &vundefined);
		errinfo.code = 5029;
		return RangeError(&errinfo, ERR_ARRAY_LEN_OUT_OF_BOUNDS, n->number);
	    }
	    else
	    {	a->length.number = len;
		if (len > 16)
		{
		    //PRINTF("setting %p dimension to %d\n", &a->proptable, len);
		    if (len > 10000)
			len = 10000;		// cap so we don't run out of memory
		    a->proptable.roots.setDim(len);
		    a->proptable.roots.zero();
		}
	    }
	}
	else
	{
	    a->length.number = 1;
	    a->Put((d_uint32) 0, v, 0);
	}
    }
    else
    {
	//if (argc > 10) WPRINTF(L"Array constructor: argc = %d\n", argc);
	if (argc > 16)
	{
	    a->proptable.roots.setDim(argc);
	    a->proptable.roots.zero();
	}
	a->length.number = argc;
	for (unsigned k = 0; k < argc; k++)
	{
	    a->Put(k, &arglist[k], 0);
	}
    }
    Value::copy(ret, &a->value);
    //PRINTF("Darray_constructor::Construct(): length = %g\n", a->length.number);
    return NULL;
}

/* ===================== Darray_prototype_toString ================= */

void array_join(Dobject *othis,
	Value *ret, unsigned argc, Value *arglist);

BUILTIN_FUNCTION(Darray_prototype_, toString, 0)
{
    //PRINTF("Darray_prototype_toString()\n");
    array_join(othis, ret, 0, NULL);
    return NULL;
}

/* ===================== Darray_prototype_toLocaleString ================= */

BUILTIN_FUNCTION(Darray_prototype_, toLocaleString, 0)
{
    // ECMA v3 15.4.4.3
    dchar *separator;
    d_string r;
    d_uint32 len;
    d_uint32 k;
    OutBuffer buf;
    Value *v;

    //PRINTF("array_join(othis = %p)\n", othis);

    if (!othis->isClass(TEXT_Array))
    {
	Value::copy(ret, &vundefined);
	ErrInfo errinfo;
	return RuntimeError(&errinfo, ERR_TLS_NOT_TRANSFERRABLE);
    }

    v = othis->Get(TEXT_length);
    len = v ? v->toUint32() : 0;

    Program *prog = cc->prog;
    if (!prog->slist)
    {
	// Determine what list separator is only once per thread
	GC_LOG();
	prog->slist = (dchar *)mem.malloc(5 * sizeof(dchar));
	Port::list_separator(prog->lcid, prog->slist);
    }
    separator = prog->slist;

    buf.reserve((len + 1) * sizeof(dchar));
    for (k = 0; k != len; k++)
    {
	if (k)
	    buf.writedstring(separator);
	v = othis->Get(k);
	if (v && !v->isUndefinedOrNull())
	{   Dobject *othis;

	    othis = v->toObject();
	    v = othis->Get(TEXT_toLocaleString, d_string_hash(TEXT_toLocaleString));
	    if (v && !v->isPrimitive())	// if it's an Object
	    {   void *a;
		Dobject *o;
		Value ret;

		o = v->object;
		Value::copy(&ret, &vundefined);
		a = o->Call(cc, othis, &ret, 0, NULL);
		if (a)			// if exception was thrown
		    return a;
		v = &ret;
		buf.writedstring(d_string_ptr(v->toString()));
	    }
	}
    }
    buf.writedchar(0);
    r = d_string_ctor((dchar *)buf.data);
    buf.data = NULL;

    Vstring::putValue(ret, r);
    return NULL;
}

/* ===================== Darray_prototype_concat ================= */

BUILTIN_FUNCTION(Darray_prototype_, concat, 1)
{
    // ECMA v3 15.4.4.4
    Darray *A;
    Darray *E;
    Value *v;
    d_uint32 k;
    d_uint32 n;
    d_uint32 a;

    GC_LOG();
    A = new(cc) Darray();
    n = 0;
    v = &othis->value;
    for (a = 0; ; a++)
    {
	if (!v->isPrimitive() && v->object->isDarray())
	{   d_uint32 len;

	    E = (Darray *)v->object;
	    len = (d_uint32) E->length.number;
	    for (k = 0; k != len; k++)
	    {
		v = E->Get(k);
		if (v)
		    A->Put(n, v, 0);
		n++;
	    }
	}
	else
	{
	    A->Put(n, v, 0);
	    n++;
	}
	if (a == argc)
	    break;
	v = &arglist[a];
    }

    A->Put(TEXT_length, n, DontDelete | DontEnum);
    Value::copy(ret, &A->value);
    return NULL;
}

/* ===================== Darray_prototype_join ================= */

BUILTIN_FUNCTION(Darray_prototype_, join, 1)
{
    array_join(othis, ret, argc, arglist);
    return NULL;
}

void array_join(Dobject *othis,
	Value *ret, unsigned argc, Value *arglist)
{
    // ECMA 15.4.4.3
    d_string separator;
    d_string r;
    d_uint32 len;
    d_uint32 k;
    OutBuffer buf;
    Value *v;

    //PRINTF("array_join(othis = %p)\n", othis);
    v = othis->Get(TEXT_length);
    len = v ? v->toUint32() : 0;
    if (argc == 0)
	separator = TEXT_comma;
    else
	separator = arglist[0].toString();

    buf.reserve((len + 1) * sizeof(dchar));
    for (k = 0; k != len; k++)
    {
	if (k)
	    buf.writedstring(d_string_ptr(separator));
	v = othis->Get(k);
	if (v && !v->isUndefinedOrNull())
	    buf.writedstring(d_string_ptr(v->toString()));
    }
    buf.writedchar(0);
    r = d_string_ctor((dchar *)buf.data);
    buf.data = NULL;

    Vstring::putValue(ret, r);
}

/* ===================== Darray_prototype_toSource ================= */

BUILTIN_FUNCTION(Darray_prototype_, toSource, 0)
{
    dchar *separator;
    d_string r;
    d_uint32 len;
    d_uint32 k;
    OutBuffer buf;
    Value *v;

    v = othis->Get(TEXT_length);
    len = v ? v->toUint32() : 0;
    separator = DTEXT(",");

    buf.reserve((len + 1) * sizeof(dchar));
    buf.writedchar('[');
    for (k = 0; k != len; k++)
    {
	if (k)
	    buf.writedstring(separator);
	v = othis->Get(k);
	if (v && !v->isUndefinedOrNull())
	    buf.writedstring(d_string_ptr(v->toSource()));
    }
    buf.writedchar(']');
    buf.writedchar(0);
    r = d_string_ctor((dchar *)buf.data);
    buf.data = NULL;

    Vstring::putValue(ret, r);
    return NULL;
}


/* ===================== Darray_prototype_pop ================= */

BUILTIN_FUNCTION(Darray_prototype_, pop, 0)
{
    // ECMA v3 15.4.4.6
    Value *v;
    d_uint32 u;

    // If othis is a Darray, then we can optimize this significantly
    v = othis->Get(TEXT_length);
    if (!v)
	v = &vundefined;
    u = v->toUint32();
    if (u == 0)
    {
	othis->Put(TEXT_length, 0.0, DontDelete | DontEnum);
	Value::copy(ret, &vundefined);
    }
    else
    {
	v = othis->Get(u - 1);
	if (!v)
	    v = &vundefined;
	Value::copy(ret, v);
	othis->Delete(u - 1);
	othis->Put(TEXT_length, u - 1, DontDelete | DontEnum);
    }
    return NULL;
}

/* ===================== Darray_prototype_push ================= */

BUILTIN_FUNCTION(Darray_prototype_, push, 1)
{
    // ECMA v3 15.4.4.7
    Value *v;
    d_uint32 u;
    d_uint32 a;

    // If othis is a Darray, then we can optimize this significantly
    v = othis->Get(TEXT_length);
    if (!v)
	v = &vundefined;
    u = v->toUint32();
    for (a = 0; a < argc; a++)
    {
	othis->Put(u + a, &arglist[a], 0);
    }
    othis->Put(TEXT_length, u + a, DontDelete | DontEnum);
    Vnumber::putValue(ret, u + a);
    return NULL;
}

/* ===================== Darray_prototype_reverse ================= */

BUILTIN_FUNCTION(Darray_prototype_, reverse, 0)
{
    // ECMA 15.4.4.4
    d_uint32 a;
    d_uint32 b;
    Value *va;
    Value *vb;
    Value *v;
    d_uint32 pivot;
    d_uint32 len;
    Value tmp;

    v = othis->Get(TEXT_length);
    len = v ? v->toUint32() : 0;
    pivot = len / 2;
    for (a = 0; a != pivot; a++)
    {
	b = len - a - 1;
	//PRINTF("a = %d, b = %d\n", a, b);
	va = othis->Get(a);
	if (va)
	    Value::copy(&tmp, va);
	vb = othis->Get(b);
	if (vb)
	    othis->Put(a, vb, 0);
	else
	    othis->Delete(a);

	if (va)
	    othis->Put(b, &tmp, 0);
	else
	    othis->Delete(b);
    }
    Value::copy(ret, &othis->value);
    return NULL;
}

/* ===================== Darray_prototype_shift ================= */

BUILTIN_FUNCTION(Darray_prototype_, shift, 0)
{
    // ECMA v3 15.4.4.9
    Value *v;
    Value *result;
    d_uint32 len;
    d_uint32 k;

    // If othis is a Darray, then we can optimize this significantly
    //WPRINTF(L"shift(othis = %p)\n", othis);
    v = othis->Get(TEXT_length);
    if (!v)
	v = &vundefined;
    len = v->toUint32();
    if (len)
    {
	result = othis->Get(0u);
	Value::copy(ret, result);
	for (k = 1; k != len; k++)
	{
	    v = othis->Get(k);
	    if (v)
	    {
		othis->Put(k - 1, v, 0);
	    }
	    else
	    {
		othis->Delete(k - 1);
	    }
	}
	othis->Delete(len - 1);
	len--;
    }
    else
	Value::copy(ret, &vundefined);

    othis->Put(TEXT_length, len, DontDelete | DontEnum);
    return NULL;
}

/* ===================== Darray_prototype_slice ================= */

BUILTIN_FUNCTION(Darray_prototype_, slice, 2)
{
    // ECMA v3 15.4.4.10
    d_uint32 len;
    d_uint32 n;
    d_uint32 k;
    d_uint32 r8;
    d_number start;
    d_number end;
    Value *v;
    Darray *A;

    v = othis->Get(TEXT_length);
    if (!v)
	v = &vundefined;
    len = v->toUint32();

    switch (argc)
    {
	case 0:
	    start = vundefined.toUint32();
	    end = start;
	    break;

	case 1:
	    v = &arglist[0];
	    start = v->toInteger();
	    end = len;
	    break;

	default:
	    v = &arglist[0];
	    start = v->toInteger();
	    v++;
	    end   = v->toInteger();
	    break;
    }

    if (start < 0)
    {
	k = len + (d_uint32) start;
	if ((d_int32)k < 0)
	    k = 0;
    }
    else
    {
	k = (d_uint32) start;
	if (len < k)
	    k = len;
    }

    if (end < 0)
    {
	r8 = len + (d_uint32) end;
	if ((d_int32)r8 < 0)
	    r8 = 0;
    }
    else
    {
	r8 = (d_uint32) end;
	if (len < (d_uint32) end)
	    r8 = len;
    }

    GC_LOG();
    A = new(cc) Darray();
    for (n = 0; k < r8; k++)
    {
	v = othis->Get(k);
	if (v)
	{
	    A->Put(n, v, 0);
	}
	n++;
    }

    A->Put(TEXT_length, n, DontDelete | DontEnum);
    Value::copy(ret, &A->value);
    return NULL;
}

/* ===================== Darray_prototype_sort ================= */

static Mutex mutex;		// wrap comparefn with mutex
static Dobject *comparefn;
static CallContext *comparecc;

int compare_value(const void *x, const void *y)
{
    Value *vx = (Value *)x;
    Value *vy = (Value *)y;
    d_string sx;
    d_string sy;
    int cmp;

    //WPRINTF(L"compare_value()\n");
    if (vx->isUndefined())
    {
	cmp = (vy->isUndefined()) ? 0 : 1;
    }
    else if (vy->isUndefined())
	cmp = -1;
    else
    {
	if (comparefn)
	{   Value arglist[2];
	    Value ret;
	    Value *v;
	    d_number n;

	    Value::copy(&arglist[0], vx);
	    Value::copy(&arglist[1], vy);
	    Value::copy(&ret, &vundefined);
	    comparefn->Call(comparecc, comparefn, &ret, 2, arglist);
	    v = &ret;
	    n = v->toNumber();
	    if (n < 0)
		cmp = -1;
	    else if (n > 0)
		cmp = 1;
	    else
		cmp = 0;
	}
	else
	{
	    sx = vx->toString();
	    sy = vy->toString();
	    cmp = Dchar::cmp(d_string_ptr(sx), d_string_ptr(sy));
	    if (cmp < 0)
		cmp = -1;
	    else if (cmp > 0)
		cmp = 1;
	}
    }
    return cmp;
}

BUILTIN_FUNCTION(Darray_prototype_, sort, 1)
{
    // ECMA v3 15.4.4.11
    Value *v;
    d_uint32 len;
    unsigned u;

    //PRINTF("Array.prototype.sort()\n");
    v = othis->Get(TEXT_length);
    len = v ? v->toUint32() : 0;

    // This is not optimal, as isArrayIndex is done at least twice
    // for every array member. Additionally, the qsort() by index
    // can be avoided if we can deduce it is not a sparse array.

    Property *p;
    Value *pvalues;
    d_uint32 *pindices;
    d_uint32 parraydim;
    d_uint32 nprops;

    // First, size & alloc our temp array
    if (len < 100)
    {	// Probably not too sparse an array
	parraydim = len;
    }
    else
    {
	parraydim = 0;
	for (p = othis->proptable.start; p; p = p->next)
	{   if (p->attributes == 0)	// don't count special properties
		parraydim++;
	}
	if (parraydim > len)		// could theoretically happen
	    parraydim = len;
    }

    void *p1 = NULL;
    if (parraydim >= 128 || (pvalues = (Value *)alloca(parraydim * sizeof(Value))) == NULL)
    {	GC_LOG();
	p1 = mem.malloc(parraydim * sizeof(Value));
	pvalues = (Value *)p1;
    }

    void *p2 = NULL;
    if (parraydim >= 128 || (pindices = (d_uint32 *)alloca(parraydim * sizeof(d_uint32))) == NULL)
    {	GC_LOG();
	p2 = mem.malloc(parraydim * sizeof(d_uint32));
	pindices = (d_uint32 *)p2;
    }

    // Now fill it with all the Property's that are array indices
    nprops = 0;
    for (p = othis->proptable.start; p; p = p->next)
    {	d_uint32 index;

	if (p->attributes == 0 &&
	    (&p->key)->isArrayIndex(&index))
	{
	    assert(nprops < parraydim);
	    pindices[nprops] = index;
	    Value::copy(&pvalues[nprops], &p->value);
	    nprops++;
	}
    }

    mutex.acquire();

    comparefn = NULL;
    comparecc = cc;
    if (argc)
    {	Value *v;

	v = &arglist[0];
	if (!v->isPrimitive())
	    comparefn = v->object;
    }

    // Sort pvalues[]
    qsort(pvalues, nprops, sizeof(Value), compare_value);

    comparefn = NULL;
    comparecc = NULL;
    mutex.release();

    // Stuff the sorted value's back into the array
    for (u = 0; u < nprops; u++)
    {	d_uint32 index;

	othis->Put(u, &pvalues[u], 0);
	index = pindices[u];
	if (index >= nprops)
	{
	    othis->Delete(index);
	}
    }

    mem.free(p1);
    mem.free(p2);

    Vobject::putValue(ret, othis);
    return NULL;
}

/* ===================== Darray_prototype_splice ================= */

BUILTIN_FUNCTION(Darray_prototype_, splice, 2)
{
    // ECMA v3 15.4.4.12
    d_uint32 len;
    d_uint32 k;
    d_number start;
    d_number deleteCount;
    Value *v;
    Darray *A;
    d_uint32 a;
    d_uint32 delcnt;
    d_uint32 inscnt;
    d_uint32 startidx;

    v = othis->Get(TEXT_length);
    if (!v)
	v = &vundefined;
    len = v->toUint32();

    switch (argc)
    {
	case 0:
	    start = vundefined.toUint32();
	    deleteCount = start;
	    break;

	case 1:
	    v = &arglist[0];
	    start = v->toInteger();
	    deleteCount = vundefined.toUint32();
	    break;

	default:
	    v = &arglist[0];
	    start = v->toInteger();
	    v++;
	    deleteCount = v->toInteger();
	    break;
    }

    if (start < 0)
    {
	startidx = len + (d_uint32) start;
	if ((d_int32)startidx < 0)
	    startidx = 0;
    }
    else
    {
	startidx = (d_uint32) start;
	if (len < startidx)
	    startidx = len;
    }

    GC_LOG();
    A = new(cc) Darray();

    delcnt = (deleteCount > 0) ? (d_uint32) deleteCount : 0;
    if (delcnt > len - startidx)
	delcnt = len - startidx;

    // If deleteCount is not specified, ECMA implies it should
    // be 0, while "JavaScript The Definitive Guide" says it should
    // be delete to end of array. Jscript doesn't implement splice().
    // We'll do it the Guide way.
    if (argc < 2)
	delcnt = len - startidx;

    //PRINTF("Darray::splice(startidx = %d, delcnt = %d)\n", startidx, delcnt);
    for (k = 0; k != delcnt; k++)
    {
	v = othis->Get(startidx + k);
	if (v)
	    A->Put(k, v, 0);
    }

    A->Put(TEXT_length, delcnt, DontDelete | DontEnum);
    inscnt = (argc > 2) ? argc - 2 : 0;
    if (inscnt != delcnt)
    {
	if (inscnt <= delcnt)
	{
	    for (k = startidx; k != (len - delcnt); k++)
	    {
		v = othis->Get(k + delcnt);
		if (v)
		    othis->Put(k + inscnt, v, 0);
		else
		    othis->Delete(k + inscnt);
	    }

	    for (k = len; k != (len - delcnt + inscnt); k--)
		othis->Delete(k - 1);
	}
	else
	{
	    for (k = len - delcnt; k != startidx; k--)
	    {
		v = othis->Get(k + delcnt - 1);
		if (v)
		    othis->Put(k + inscnt - 1, v, 0);
		else
		    othis->Delete(k + inscnt - 1);
	    }
	}
    }
    k = startidx;
    for (a = 2; a < argc; a++)
    {
	v = &arglist[a];
	othis->Put(k, v, 0);
	k++;
    }

    othis->Put(TEXT_length, len - delcnt + inscnt, DontDelete | DontEnum);
    Value::copy(ret, &A->value);
    return NULL;
}

/* ===================== Darray_prototype_unshift ================= */

BUILTIN_FUNCTION(Darray_prototype_, unshift, 1)
{
    // ECMA v3 15.4.4.13
    Value *v;
    d_uint32 len;
    d_uint32 k;

    v = othis->Get(TEXT_length);
    if (!v)
	v = &vundefined;
    len = v->toUint32();

    for (k = len; k; k--)
    {
	v = othis->Get(k - 1);
	if (v)
	    othis->Put(k + argc - 1, v, 0);
	else
	    othis->Delete(k + argc - 1);
    }

    for (k = 0; k < argc; k++)
    {
	othis->Put(k, &arglist[k], 0);
    }
    othis->Put(TEXT_length, len + argc, DontDelete | DontEnum);
    Vnumber::putValue(ret, len + argc);
    return NULL;
}

/* =========================== Darray_prototype =================== */

struct Darray_prototype : Darray
{
    Darray_prototype(ThreadContext *tc);
};

Darray_prototype::Darray_prototype(ThreadContext *tc)
    : Darray(tc->Dobject_prototype)
{
    Dobject *f = tc->Dfunction_prototype;

    Put(TEXT_constructor, tc->Darray_constructor, DontEnum);
    GC_LOG();
    Put(TEXT_toString, new(this) Darray_prototype_toString(f), DontEnum);
    GC_LOG();
    Put(TEXT_toLocaleString, new(this) Darray_prototype_toLocaleString(f), DontEnum);
    GC_LOG();
    Put(TEXT_toSource, new(this) Darray_prototype_toSource(f), DontEnum);
    GC_LOG();
    Put(TEXT_concat, new(this) Darray_prototype_concat(f), DontEnum);
    GC_LOG();
    Put(TEXT_join, new(this) Darray_prototype_join(f), DontEnum);
    GC_LOG();
    Put(TEXT_pop, new(this) Darray_prototype_pop(f), DontEnum);
    GC_LOG();
    Put(TEXT_push, new(this) Darray_prototype_push(f), DontEnum);
    GC_LOG();
    Put(TEXT_reverse, new(this) Darray_prototype_reverse(f), DontEnum);
    GC_LOG();
    Put(TEXT_shift, new(this) Darray_prototype_shift(f), DontEnum);
    GC_LOG();
    Put(TEXT_slice, new(this) Darray_prototype_slice(f), DontEnum);
    GC_LOG();
    Put(TEXT_sort, new(this) Darray_prototype_sort(f), DontEnum);
    GC_LOG();
    Put(TEXT_splice, new(this) Darray_prototype_splice(f), DontEnum);
    GC_LOG();
    Put(TEXT_unshift, new(this) Darray_prototype_unshift(f), DontEnum);
}

/* =========================== Darray =================== */

Darray::Darray()
    : Dobject(Darray::getPrototype()), length(0)
{
    classname = TEXT_Array;
}

Darray::Darray(Dobject *prototype)
    : Dobject(prototype), length(0)
{
    classname = TEXT_Array;
}

Value *Darray::Put(d_string name, Value *v, unsigned attributes)
{
    d_uint32 i;
    unsigned c;
    Value *result;

    // ECMA 15.4.5.1
    result = proptable.put(name, v, attributes);
    if (!result)
    {
	if (d_string_cmp(name,TEXT_length) == 0)
	{
	    i = v->toUint32();
	    if (i != v->toInteger())
	    {	ErrInfo errinfo;

		return Dobject::RuntimeError(&errinfo, ERR_LENGTH_INT);
	    }
	    if (i < length.number)
	    {
		// delete all properties with value >= i
		Property *p;
		Property *pnext;

		for (p = proptable.start; p; p = pnext)
		{
		    pnext = p->next;
		    if (p->key.toUint32() >= i)
			proptable.del(&p->key);
		}
	    }
	    length.number = i;
	    proptable.put(name, v, attributes | DontDelete | DontEnum);
	}

	// if (name is an array index i)
	dchar *p;
	unsigned len;
	unsigned j;

	i = 0;
	len = d_string_len(name);
	p = d_string_ptr(name);
	for (j = 0; j < len; j++)
	{   ulonglong k;

	    c = p[j];
	    if (c == '0' && i == 0 && len > 1)
		goto Lret;
	    if (c >= '0' && c <= '9')
	    {   k = i * (ulonglong)10 + c - '0';
		i = (d_uint32)k;
		if (i != k)
		    goto Lret;		// overflow
	    }
	    else
		goto Lret;
	}
	if (i >= length.number)
	{
	    if (i == 0xFFFFFFFF)
		goto Lret;
	    length.number = i + 1;
	}
    }
  Lret:
    return NULL;
}

Value *Darray::Put(d_string name, Dobject *o, unsigned attributes)
{
    return Put(name, &o->value, attributes);
}

Value *Darray::Put(d_string PropertyName, d_number n, unsigned attributes)
{
    Vnumber v(n);

    return Put(PropertyName, &v, attributes);
}

Value *Darray::Put(d_string PropertyName, d_string string, unsigned attributes)
{
    Vstring v(string);

    return Put(PropertyName, &v, attributes);
}

Value *Darray::Put(d_uint32 index, Value *value, unsigned attributes)
{
    if (index >= length.number)
	length.number = index + 1;

    proptable.put(index, value, attributes);
    return NULL;
}

Value *Darray::Put(d_uint32 index, d_string string, unsigned attributes)
{
    if (index >= length.number)
	length.number = index + 1;

    proptable.put(index, string, attributes);
    return NULL;
}

Value *Darray::Get(d_string PropertyName, unsigned hash)
{
    //PRINTF("Darray::Get(%p, '%s')\n", &proptable, PropertyName);
    if (Dstring::cmp(PropertyName, TEXT_length) == 0)
	return &length;
    else
	return Dobject::Get(PropertyName, hash);
}

Value *Darray::Get(d_uint32 index)
{   Value *v;

    //PRINTF("Darray::Get(%p, %d)\n", &proptable, index);
    v = proptable.get(index);
    return v;
}

int Darray::Delete(d_string PropertyName)
{
    // ECMA 8.6.2.5
    //WPRINTF(L"Darray::Delete('%ls')\n", d_string_ptr(PropertyName));
    if (Dstring::cmp(PropertyName, TEXT_length) == 0)
	return 0;		// can't delete 'length' property
    else
	return proptable.del(PropertyName);
}

int Darray::Delete(d_uint32 index)
{
    // ECMA 8.6.2.5
    return proptable.del(index);
}


Dfunction *Darray::getConstructor()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Darray_constructor;
}

Dobject *Darray::getPrototype()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Darray_prototype;
}

void Darray::init(ThreadContext *tc)
{
    GC_LOG();
    tc->Darray_constructor = new(&tc->mem) Darray_constructor(tc);
    GC_LOG();
    tc->Darray_prototype = new(&tc->mem) Darray_prototype(tc);

    tc->Darray_constructor->Put(TEXT_prototype, tc->Darray_prototype, DontEnum | DontDelete | ReadOnly);
}


