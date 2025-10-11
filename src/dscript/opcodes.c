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
#include <math.h>

#include "gc.h"
#include "root.h"
#include "port.h"
#include "dscript.h"
#include "value.h"
#include "dobject.h"
#include "ir.h"
#include "iterator.h"
#include "text.h"
#include "statement.h"
#include "function.h"

#define VERIFY	0	// verify integrity of code

extern void *dcomobject_call(d_string s, CALL_ARGS);
extern d_boolean dcomobject_isNull(Dcomobject* pObj);
extern d_boolean dcomobject_areEqual(Dcomobject* pLeft, Dcomobject* pRight);

// Catch & Finally are "fake" Dobjects that sit in the scope
// chain to implement our exception handling context.

struct Catch : Dobject
{
    // This is so scope_get() will skip over these objects
    Value *Get(d_string PropertyName) { return NULL; }
    Value *Get(d_string PropertyName, unsigned hash) { return NULL; }

    // This is so we can distinguish between a real Dobject
    // and these fakers
    d_string getTypeof() { return d_string_null; }

    unsigned offset;    // offset of CatchBlock
    d_string name;      // catch identifier

    Catch(unsigned offset, d_string name);

    int isCatch() { return TRUE; }
};

Catch::Catch(unsigned offset, d_string name)
    : Dobject(NULL)
{
    this->offset = offset;
    this->name = name;
}

struct Finally : Dobject
{
    Value *Get(d_string PropertyName) { return NULL; }
    Value *Get(d_string PropertyName, unsigned hash) { return NULL; }
    d_string getTypeof() { return d_string_null; }

    IR *finallyblock;    // code for FinallyBlock

    Finally(IR *finallyblock);

    int isFinally() { return TRUE; }
};

Finally::Finally(IR *finallyblock)
    : Dobject(NULL)
{
    this->finallyblock = finallyblock;
}

/************************
 * Look for identifier in scope.
 */

Value *scope_get(Array *scope, d_string s, unsigned hash, Dobject **pthis)
{   unsigned d;
    Dobject *o;
    Value *v = NULL;            // =NULL necessary for /W4

    //PRINTF("scope_get: scope = %p, scope->data = %p\n", scope, scope->data);
    d = scope->dim;
    for (;;)
    {
        if (!d)
        {   v = NULL;
            *pthis = NULL;
            break;
        }
        d--;
        o = (Dobject *)scope->data[d];
        //PRINTF("o = %x, hash = x%x, s = '%s'\n", o, hash, s);
        v = o->Get(s, hash);
        if (v)
        {   *pthis = o;
            break;
        }
    }
    return v;
}

Value *scope_get_lambda(Array *scope, d_string s, unsigned hash, Dobject **pthis)
{   unsigned d;
    Dobject *o;
    Value *v = NULL;            // =NULL necessary for /W4

    //PRINTF("scope_get: scope = %p, scope->data = %p\n", scope, scope->data);
    d = scope->dim;
    for (;;)
    {
        if (!d)
        {   v = NULL;
            *pthis = NULL;
            break;
        }
        d--;
        o = (Dobject *)scope->data[d];
        //PRINTF("o = %x, hash = x%x, s = '%s'\n", o, hash, s);
        v = o->GetLambda(s, hash);
        if (v)
        {   *pthis = o;
            break;
        }
    }
    return v;
}

Value *scope_get(Array *scope, d_string s, unsigned hash)
{   unsigned d;
    Dobject *o;
    Value *v = NULL;            // =NULL necessary for /W4

    d = scope->dim;
    // 1 is most common case for d
    if (d == 1)
    {
        return ((Dobject *)scope->data[0])->Get(s, hash);
    }
    for (;;)
    {
        if (!d)
        {   v = NULL;
            break;
        }
        d--;
        o = (Dobject *)scope->data[d];
        v = o->Get(s, hash);
        if (v)
            break;
    }
    return v;
}

/************************************
 * Find last object in scope, NULL if none.
 */

Dobject *scope_tos(Array *scope)
{   unsigned d;
    Dobject *o;

    for (d = scope->dim; d;)
    {
        d--;
        o = (Dobject *)scope->data[d];
        if (o->getTypeof() != d_string_null) // if not a Finally or a Catch
            return o;
    }
    return NULL;
}

/*****************************************
 */

void PutValue(CallContext *cc, d_string s, Value *a)
{
    // ECMA v3 8.7.2
    // Look for the object o in the scope chain.
    // If we find it, put its value.
    // If we don't find it, put it into the global object

    unsigned d;
    unsigned hash;
    Value *v;
    Dobject *o;

    d = cc->scope->dim;
    if (d == cc->globalroot)
    {
        o = scope_tos(cc->scope);
        o->Put(s, a, 0);
        return;
    }

    hash = Vstring::calcHash(s);

    for (;; d--)
    {
        assert(d > 0);
        o = (Dobject *)cc->scope->data[d - 1];
        if (d == cc->globalroot)
        {
            o->Put(s, a, 0);
            return;
        }
        v = o->Get(s, hash);
        if (v)
        {
            // Overwrite existing property with new one
            //Value::copy(v, a);
            o->Put(s, a, 0);
            break;
        }
    }
}

/***************************************
 * Cache for getscope's
 */

struct ScopeCache
{
    d_string s;
    Value *v;		// never NULL, and never from a Dcomobject
};

#define SCOPECACHING	1	// turn scope caching on (1) and off (0)
#define SCOPECACHEMAX 8

#if SCOPECACHING
#define SCOPECACHE_SI(s)	(((unsigned)(s) >> 4) & 7)
#define SCOPECACHE_CLEAR()	memset(scopecache, 0, SCOPECACHEMAX * sizeof(ScopeCache))
#else
#define SCOPECACHE_SI(s)
#define SCOPECACHE_CLEAR()
#endif

/*****************************************
 * Helper function for Values that cannot be converted to Objects.
 */

Value *cannotConvert(Value *b, int linnum)
{
    ErrInfo errinfo;

    errinfo.linnum = linnum;
    if (b->isUndefinedOrNull())
    {
	b = Dobject::RuntimeError(&errinfo,
		ERR_CANNOT_CONVERT_TO_OBJECT4,
		b->getType());
    }
    else
    {
	b = Dobject::RuntimeError(&errinfo,
		ERR_CANNOT_CONVERT_TO_OBJECT2,
		b->getType(), d_string_ptr(b->toString()));
    }
    return b;
}

/****************************
 * This is the main interpreter loop.
 */

void *IR::call(CallContext *cc, Dobject *othis, IR *code, Value *ret, Value *locals)
{   Value *a;
    Value *b;
    Value *c;
    Value *v;
    Iterator *iter;
    d_string s;
    d_string s2;
    d_number n;
    d_boolean bo;
    d_int32 i32;
    d_uint32 u32;
    d_boolean res;
    dchar *tx;
    dchar *ty;
    Dobject *o;
    Array *scope;
    unsigned dimsave;
    unsigned offset;
    Catch *ca;
    Finally *f;
    IR *codestart = code;
    d_number inc;

#if SCOPECACHING
    int si;
    ScopeCache scopecache[SCOPECACHEMAX];
    //int scopecache_cnt = 0;

    SCOPECACHE_CLEAR();
#endif

#if defined __DMC__ || __GNUC__ || 1
    // Eliminate the scale factor of sizeof(Value) by computing it at compile time
    #define GETa(code)  ((Value *)((char *)locals + (code + 1)->index * (16 / INDEX_FACTOR)))
    #define GETb(code)  ((Value *)((char *)locals + (code + 2)->index * (16 / INDEX_FACTOR)))
    #define GETc(code)  ((Value *)((char *)locals + (code + 3)->index * (16 / INDEX_FACTOR)))
    #define GETd(code)  ((Value *)((char *)locals + (code + 4)->index * (16 / INDEX_FACTOR)))
    #define GETe(code)  ((Value *)((char *)locals + (code + 5)->index * (16 / INDEX_FACTOR)))
#else
    #define GETa(code)  (&locals[(code + 1)->index])
    #define GETb(code)  (&locals[(code + 2)->index])
    #define GETc(code)  (&locals[(code + 3)->index])
    #define GETd(code)  (&locals[(code + 4)->index])
    #define GETe(code)  (&locals[(code + 5)->index])
#endif
    #define GETlinnum(code)	(code->op.linnum)

#if VERIFY
    unsigned checksum = IR::verify(__LINE__, code);
#endif

#if 0
    WPRINTF(L"+printfunc\n");
    printfunc(code);
    WPRINTF(L"-printfunc\n");
#endif
    scope = cc->scope;
    dimsave = scope->dim;
//if (logflag)
//    PRINTF("IR::call(othis = %p, code = %p, locals = %p)\n",othis,code,locals);

#ifdef DEBUG
    unsigned debug_scoperoot = cc->scoperoot;
    unsigned debug_globalroot = cc->globalroot;
    unsigned debug_scopedim = scope->dim;
    unsigned debug_scopeallocdim = scope->allocdim;
    Dobject* debug_global = cc->global;
    Dobject* debug_variable = cc->variable;

    GC_LOG();
    void** debug_pscoperootdata = (void**)mem.malloc(sizeof(void*)*debug_scoperoot);
    GC_LOG();
    void** debug_pglobalrootdata = (void**)mem.malloc(sizeof(void*)*debug_globalroot);

    assert(NULL != debug_pscoperootdata && NULL != debug_pglobalrootdata);
    memcpy(debug_pscoperootdata, scope->data, sizeof(void*)*debug_scoperoot);
    memcpy(debug_pglobalrootdata, scope->data, sizeof(void*)*debug_globalroot);

#endif // DEBUG

    assert(code);
    //Value::copy(ret, &vundefined);
    othis->invariant();

Lnext:
    //WPRINTF(L"cc = %x, interrupt = %d\n", cc, cc->Interrupt);
    if (cc->Interrupt)			// see if script was interrupted
	goto Linterrupt;
    for (;;)
    {
#if 0
	//if (logflag)
	{
        PRINTF("%2d:", code - codestart);
        print(code - codestart, code);
	}
#endif

#ifdef DEBUG
        assert(scope == cc->scope);
        assert(debug_scoperoot == cc->scoperoot);
        assert(debug_globalroot == cc->globalroot);
        assert(debug_global == cc->global);
        assert(debug_variable == cc->variable);
        assert(scope->dim >= debug_scoperoot);
        assert(scope->dim >= debug_globalroot);
        assert(scope->dim >= debug_scopedim);
        assert(scope->allocdim >= debug_scopeallocdim);
        assert(0 == memcmp(debug_pscoperootdata, scope->data, sizeof(void*)*debug_scoperoot));
        assert(0 == memcmp(debug_pglobalrootdata, scope->data, sizeof(void*)*debug_globalroot));

		scope->invariant();
#endif // DEBUG

#if 0
	v = &vundefined;
	tx = v->getType();
	assert(tx == TypeUndefined);
#endif
        //PRINTF("\tIR%d:\n", code->op.opcode);
        switch (code->op.opcode)
        {
            case IRerror:
                assert(0);
                break;

            case IRnop:
                code++;
                break;

            case IRget:                 // a = b.c
                a = GETa(code);
                b = GETb(code);
                o = b->toObject();
                if (!o)
                {
		    a = cannotConvert(b, GETlinnum(code));
                    goto Lthrow;
                }
                c = GETc(code);
                if (c->isNumber() && (u32 = (d_uint32)c->number) == c->number)
                {
                    //PRINTF("IRget %d\n", u32);
                    v = o->Get(u32);
                }
                else
                {
                    s = c->toString();
                    v = o->Get(s);
                }
                if (!v)
                    v = &vundefined;
                Value::copy(a,v);
                code += 4;
                break;

            case IRput:                 // b.c = a
                a = GETa(code);
                b = GETb(code);
                c = GETc(code);
                if (c->isNumber() && (u32 = (d_uint32)c->number) == c->number)
                {
                    //PRINTF("IRput %d\n", u32);
                    a = b->Put(u32, a);
                }
                else
                {
                    s = c->toString();
                    a = b->Put(s, a);
                }
		if (a) goto Lthrow;
                code += 4;
                break;

            case IRgets:                // a = b.s
                a = GETa(code);
                b = GETb(code);
                s = (code + 3)->string;
                o = b->toObject();
                if (!o)
                {
                    //WPRINTF(DTEXT("%ls %ls.%ls cannot convert to Object"), b->getType(), b->toString(), s);
		    ErrInfo errinfo;
		    a = Dobject::RuntimeError(&errinfo,
			    ERR_CANNOT_CONVERT_TO_OBJECT3,
			    b->getType(), d_string_ptr(b->toString()),
			    d_string_ptr(s));
                    goto Lthrow;
                }
                v = o->Get(s);
                if (!v)
                {
                    //WPRINTF(L"IRgets: %ls.%ls is undefined\n", b->getType(), d_string_ptr(s));
                    v = &vundefined;
                }
                Value::copy(a,v);
                code += 4;
                goto Lnext;

            case IRgetscope:            // a = s
                a = GETa(code);
                s = (code + 2)->string;
#if SCOPECACHING
		si = SCOPECACHE_SI(s);
		if (s == scopecache[si].s)
		{
		    //scopecache_cnt++;
		    Value::copy(a, scopecache[si].v);
		    code += 4;
		    break;
		}
#endif
#if 1
                // Inline scope_get() for speed
                {   unsigned d;
                    unsigned hash;

                    hash = (code + 3)->hash;
                    d = scope->dim;
                    // 1 is most common case for d
                    if (d == 1)
                    {
			o = (Dobject *)scope->data[0];
                        v = o->Get(s, hash);
                    }
                    else
                    {
			o = NULL;	// for _MSC_VER warnings
                        for (;;)
                        {
                            if (!d)
                            {   v = NULL;
                                break;
                            }
                            d--;
                            o = (Dobject *)scope->data[d];
                            v = o->Get(s, hash);
                            //WPRINTF(L"o = %x, v = %x\n", o, v);
                            if (v)
                                break;
                        }
                    }
                }
                if (!v)
                    v = &vundefined;
#if SCOPECACHING
		else if (!o->isDcomobject())
		{
		    scopecache[si].s = s;
		    scopecache[si].v = v;
		}
#endif
#else
                v = scope_get(scope, s, (code + 3)->hash);
                if (!v)
                    v = &vundefined;
#endif
                //WPRINTF(L"v = %p\n", v);
                //WPRINTF(L"v = %g\n", v->toNumber());
                //WPRINTF(L"v = %ls\n", d_string_ptr(v->toString()));
                Value::copy(a, v);
                code += 4;
                break;

            case IRaddass:              // a = (b.c += a)
                c = GETc(code);
                s = c->toString();
                goto Laddass;

            case IRaddasss:             // a = (b.s += a)
                s = (code + 3)->string;
            Laddass:
                b = GETb(code);
                v = b->Get(s);
                goto Laddass2;

            case IRaddassscope:         // a = (s += a)
                b = NULL;               // Needed for the b->Put() below to shutup a compiler use-without-init warning
                s = (code + 2)->string;
#if SCOPECACHING
		si = SCOPECACHE_SI(s);
		if (s == scopecache[si].s)
		    v = scopecache[si].v;
		else
		    v = scope_get(scope, s, (code + 3)->hash);
#else
                v = scope_get(scope, s, (code + 3)->hash);
#endif
            Laddass2:
                a = GETa(code);
                if (!v)
		{
                    v = &vundefined;
		    Value::copy(a, v);
#if 0
		    if (b)
		    {
			a = b->Put(s, v);
			//if (a) goto Lthrow;
		    }
		    else
		    {
			PutValue(cc, s, v);
		    }
#endif
		}
                else if (a->isNumber() && v->isNumber())
                {   a->number += v->number;
                    v->number = a->number;
                }
                else
                {
                    v->toPrimitive(v, NULL);
                    a->toPrimitive(a, NULL);
                    if (v->isString())
                    {
                        s2 = Dstring::dup2(cc, v->toString(), a->toString());
                        Vstring::putValue(a, s2);
                        Value::copy(v, a);
                    }
                    else if (a->isString())
                    {
                        s2 = Dstring::dup2(cc, v->toString(), a->toString());
                        Vstring::putValue(a, s2);
                        Value::copy(v, a);
                    }
                    else
                    {
                        Vnumber::putValue(a, a->toNumber() + v->toNumber());
                        v->number = a->number;
                    }
                }
                code += 4;
                break;

	    case IRputs:		// b.s = a
		a = GETa(code);
		b = GETb(code);
		o = b->toObject();
		if (!o)
		{
		    a = cannotConvert(b, GETlinnum(code));
		    goto Lthrow;
		}
		a = o->Put((code + 3)->string, a, 0);
		if (a) goto Lthrow;
		code += 4;
		goto Lnext;

            case IRputscope:            // s = a
                PutValue(cc, (code + 2)->string, GETa(code));
                code += 3;
                break;

	    case IRputdefault:		// b = a
		a = GETa(code);
		b = GETb(code);
		o = b->toObject();
		if (!o)
		{
		    ErrInfo errinfo;
		    a = Dobject::RuntimeError(&errinfo,
			    ERR_CANNOT_ASSIGN, a->getType(),
			    b->getType());
		    goto Lthrow;
		}
		a = o->PutDefault(a);
		if (a)
		    goto Lthrow;
		code += 3;
		break;

            case IRputthis:             // s = a
                a = cc->variable->Put((code + 2)->string, GETa(code), 0);
		//if (a) goto Lthrow;
                code += 3;
                break;

            case IRmov:                 // a = b
                Value::copy(GETa(code), GETb(code));
                code += 3;
                break;

            case IRstring:              // a = "string"
                Vstring::putValue(GETa(code),(code + 2)->string);
                code += 3;
                break;

            case IRobject:              // a = object
	    {	FunctionDefinition *fd;
		fd = (FunctionDefinition *)((code + 2)->ptr);
		Dfunction *fobject = new DdeclaredFunction(fd);
		fobject->scopex = scope;
		fobject->scopex_dim = scope->dim;
                Vobject::putValue(GETa(code), fobject);
                code += 3;
                break;
	    }

            case IRthis:                // a = this
                Vobject::putValue(GETa(code),othis);
		//WPRINTF(L"IRthis: %s, othis = %x\n", GETa(code)->getType(), othis);
                code += 2;
                break;

            case IRnumber:              // a = number
                Vnumber::putValue(GETa(code),*(d_number *)(code + 2));
                code += 4;
                break;

            case IRboolean:             // a = boolean
                Vboolean::putValue(GETa(code),(code + 2)->boolean);
                code += 3;
                break;

            case IRnull:                // a = null
                Value::copy(GETa(code), &vnull);
                code += 2;
                break;

            case IRundefined:           // a = undefined
                Value::copy(GETa(code), &vundefined);
                code += 2;
                break;

            case IRthisget:             // a = othis->ident
                a = GETa(code);
                v = othis->Get((code + 2)->string);
                if (!v)
                    v = &vundefined;
                Value::copy(a, v);
                code += 3;
                break;

            case IRneg:                 // a = -a
                a = GETa(code);
                n = a->toNumber();
                Vnumber::putValue(a, -n);
                code += 2;
                break;

            case IRpos:                 // a = a
                a = GETa(code);
                n = a->toNumber();
                Vnumber::putValue(a, n);
                code += 2;
                break;

            case IRcom:                 // a = ~a
                a = GETa(code);
                i32 = a->toInt32();
                Vnumber::putValue(a, ~i32);
                code += 2;
                break;

            case IRnot:                 // a = !a
                a = GETa(code);
                Vboolean::putValue(a, !a->toBoolean());
                code += 2;
                break;

            case IRtypeof:      // a = typeof a
                // ECMA 11.4.3 says that if the result of (a)
                // is a Reference and GetBase(a) is null,
                // then the result is "undefined". I don't know
                // what kind of script syntax will generate this.
                a = GETa(code);
                Vstring::putValue(a, a->getTypeof());
                code += 2;
                break;

	    case IRinstance:	// a = b instanceof c
	    {
		Dobject *co;

		// ECMA v3 11.8.6

		b = GETb(code);
		o = b->toObject();
		c = GETc(code);
		if (c->isPrimitive())
		{
		    ErrInfo errinfo;
		    a = Dobject::RuntimeError(&errinfo,
			    ERR_RHS_MUST_BE_OBJECT,
			    DTEXT("instanceof"), c->getType());
		    goto Lthrow;
		}
		co = c->toObject();
		a = GETa(code);
		v = (Value *)co->HasInstance(a, b);
		if (v)
		{    a = v;
		     goto Lthrow;
		}
		code += 4;
		break;
	    }
	    case IRadd:			// a = b + c
		a = GETa(code);
		b = GETb(code);
		c = GETc(code);

                if (b->isNumber() && c->isNumber())
                {
                    Vnumber::putValue(a, b->number + c->number);
                }
                else
                {
                    char vtmpb[sizeof(Value)];
                    Value *vb = (Value *)vtmpb;
                    char vtmpc[sizeof(Value)];
                    Value *vc = (Value *)vtmpc;

                    v = (Value *)b->toPrimitive(vb, NULL);
                    if (v)
                    {   a = v;
                        goto Lthrow;
                    }
                    v = (Value *)c->toPrimitive(vc, NULL);
                    if (v)
                    {   a = v;
                        goto Lthrow;
                    }
                    if (vb->isString() || vc->isString())
                    {
                        s = Dstring::dup2(cc, vb->toString(), vc->toString());
                        Vstring::putValue(a, s);
                    }
                    else
                    {
                        Vnumber::putValue(a, vb->toNumber() + vc->toNumber());
                    }
                }

                code += 4;
                break;

            case IRsub:                 // a = b - c
                a = GETa(code);
                b = GETb(code);
                c = GETc(code);
                Vnumber::putValue(a, b->toNumber() - c->toNumber());
                code += 4;
                break;

            case IRmul:                 // a = b * c
                a = GETa(code);
                b = GETb(code);
                c = GETc(code);
                Vnumber::putValue(a, b->toNumber() * c->toNumber());
                code += 4;
                break;

            case IRdiv:                 // a = b / c
                a = GETa(code);
                b = GETb(code);
                c = GETc(code);

                //PRINTF("%g / %g = %g\n", b->toNumber() , c->toNumber(), b->toNumber() / c->toNumber());
                Vnumber::putValue(a, b->toNumber() / c->toNumber());
                code += 4;
                break;

            case IRmod:                 // a = b % c
                a = GETa(code);
                b = GETb(code);
                c = GETc(code);
#if _MSC_VER
                // fmod() does not work right for special values
                {   double x = b->toNumber();
                    double y = c->toNumber();
                    double result;

                    if (Port::isnan(x) || Port::isnan(y))
                        result = Port::nan;
                    else if (Port::isinfinity(x) || y == 0)
                        result = Port::nan;
                    else if (Port::isfinite(x) && Port::isinfinity(y))
                        result = x;
                    else if (x == 0 && Port::isfinite(y))
                        result = x;
                    else
                        result = fmod(x, y);
                    Vnumber::putValue(a, result);
                }
#else
                Vnumber::putValue(a, fmod(b->toNumber(),c->toNumber()));
#endif
                code += 4;
                break;

            case IRshl:                 // a = b << c
                a = GETa(code);
                b = GETb(code);
                c = GETc(code);
                i32 = b->toInt32();
                u32 = c->toUint32() & 0x1F;
                i32 <<= u32;
                Vnumber::putValue(a, i32);
                code += 4;
                break;

            case IRshr:                 // a = b >> c
                a = GETa(code);
                b = GETb(code);
                c = GETc(code);
                i32 = b->toInt32();
                u32 = c->toUint32() & 0x1F;
                i32 >>= (d_int32) u32;
                Vnumber::putValue(a, i32);
                code += 4;
                break;

            case IRushr:                // a = b >>> c
                a = GETa(code);
                b = GETb(code);
                c = GETc(code);
                i32 = b->toUint32();
                u32 = c->toUint32() & 0x1F;
                u32 = ((d_uint32) i32) >> u32;
                Vnumber::putValue(a, u32);
                code += 4;
                break;

            case IRand:         // a = b & c
                a = GETa(code);
                b = GETb(code);
                c = GETc(code);
                Vnumber::putValue(a, b->toInt32() & c->toInt32());
                code += 4;
                break;

            case IRor:          // a = b | c
                a = GETa(code);
                b = GETb(code);
                c = GETc(code);
                Vnumber::putValue(a, b->toInt32() | c->toInt32());
                code += 4;
                break;

            case IRxor:         // a = b ^ c
                a = GETa(code);
                b = GETb(code);
                c = GETc(code);
                Vnumber::putValue(a, b->toInt32() ^ c->toInt32());
                code += 4;
                break;

	    /********************/

            case IRpreinc:     // a = ++b.c
                c = GETc(code);
                s = c->toString();
                goto Lpreinc;
            case IRpreincs:    // a = ++b.s
                s = (code + 3)->string;
            Lpreinc:
		inc = 1;
	    Lpre:
                a = GETa(code);
                b = GETb(code);
                v = b->Get(s);
                if (!v)
                    v = &vundefined;
                n = v->toNumber();
                Vnumber::putValue(a, n + inc);
                b->Put(s, a);
                code += 4;
                break;

            case IRpreincscope:        // a = ++s
		inc = 1;
	    Lprescope:
		a = GETa(code);
                s = (code + 2)->string;
#if SCOPECACHING
		si = SCOPECACHE_SI(s);
		if (s == scopecache[si].s)
		{
		    v = scopecache[si].v;
		    n = v->toNumber();
                    Vnumber::putValue(v, n + inc);
		    Value::copy(a, v);
		}
		else
		{
		    v = scope_get(scope, s, (code + 3)->hash, &o);
		    if (v)
		    {   n = v->toNumber();
			Vnumber::putValue(v, n + inc);
			Value::copy(a, v);
			if (o->isDcomobject())
			    o->Put(s, a, 0);
		    }
		    else
			Value::copy(a, &vundefined);
		}
#else
                v = scope_get(scope, s, (code + 3)->hash, &o);
                if (v)
                {   n = v->toNumber();
                    Vnumber::putValue(v, n + inc);
		    Value::copy(a, v);
                    if (o->isDcomobject())
                        o->Put(s, a, 0);
                }
                else
                    Value::copy(a, &vundefined);
#endif
                code += 4;
                break;

            case IRpredec:     // a = --b.c
                c = GETc(code);
                s = c->toString();
                goto Lpredec;
            case IRpredecs:    // a = --b.s
                s = (code + 3)->string;
            Lpredec:
		inc = -1;
		goto Lpre;

            case IRpredecscope:        // a = --s
		inc = -1;
		goto Lprescope;

	    /********************/

            case IRpostinc:     // a = b.c++
                c = GETc(code);
                s = c->toString();
                goto Lpostinc;
            case IRpostincs:    // a = b.s++
                s = (code + 3)->string;
            Lpostinc:
                a = GETa(code);
                b = GETb(code);
                v = b->Get(s);
                if (!v)
                    v = &vundefined;
                n = v->toNumber();
                Vnumber::putValue(a, n + 1);
		b->Put(s, a);
                Vnumber::putValue(a, n);
                code += 4;
                break;

            case IRpostincscope:        // a = s++
                s = (code + 2)->string;
                v = scope_get(scope, s, Vstring::calcHash(s), &o);
                if (v && v != &vundefined)
                {
		    n = v->toNumber();
                    a = GETa(code);
                    Vnumber::putValue(v, n + 1);
                    if (o->isDcomobject())
                        o->Put(s, v, 0);
                    Vnumber::putValue(a, n);
                }
                else
                    Value::copy(GETa(code), &vundefined);
                code += 3;
                break;

            case IRpostdec:     // a = b.c--
                c = GETc(code);
                s = c->toString();
                goto Lpostdec;
            case IRpostdecs:    // a = b.s--
                s = (code + 3)->string;
            Lpostdec:
                a = GETa(code);
                b = GETb(code);
                v = b->Get(s);
                if (!v)
                    v = &vundefined;
                n = v->toNumber();
                Vnumber::putValue(a, n - 1);
                b->Put(s, a);
                Vnumber::putValue(a, n);
                code += 4;
                break;

            case IRpostdecscope:        // a = s--
                s = (code + 2)->string;
                v = scope_get(scope, s, Vstring::calcHash(s), &o);
                if (v && v != &vundefined)
                {   n = v->toNumber();
                    a = GETa(code);
                    Vnumber::putValue(v, n - 1);
                    if (o->isDcomobject())
                        o->Put(s, v, 0);
                    Vnumber::putValue(a, n);
                }
                else
                    Value::copy(GETa(code), &vundefined);
                code += 3;
                break;

	    case IRdel:		// a = delete b.c
	    case IRdels:	// a = delete b.s
		b = GETb(code);
		if (b->isPrimitive())
		    bo = TRUE;
		else
		{
		    o = b->toObject();
		    if (!o)
		    {
			a = cannotConvert(b, GETlinnum(code));
			goto Lthrow;
		    }
		    s = (code->op.opcode == IRdel)
			? GETc(code)->toString()
			: (code + 3)->string;
		    if (o->implementsDelete())
			bo = o->Delete(s);
		    else
			bo = !o->HasProperty(s);
		}
		Vboolean::putValue(GETa(code), bo);
		code += 4;
		break;

            case IRdelscope:    // a = delete s
                s = (code + 2)->string;
		//o = scope_tos(scope);		// broken way
		if (!scope_get(scope, s, Vstring::calcHash(s), &o))
		    bo = TRUE;
		else if (o->implementsDelete())
                    bo = o->Delete(s);
                else
                    bo = !o->HasProperty(s);
                Vboolean::putValue(GETa(code), bo);
                code += 3;
                break;

#if defined __DMC__
// ECMA requires that if one of the numeric operands is NAN, then
// the result of the comparison is false. DMC generates a correct
// test for NAN operands. Other compilers do not, necessitating
// adding extra tests.
#define COMPARE(op)                                             \
                if (b->isNumber() && c->isNumber())             \
                    res = (b->number op c->number);             \
                else                                            \
                {                                               \
                    b->toPrimitive(b, TypeNumber);              \
                    c->toPrimitive(c, TypeNumber);              \
                    if (b->isString() && c->isString())         \
                    {   d_string x = b->toString();             \
                        d_string y = c->toString();             \
                                                                \
                        res = Dstring::cmp(x, y) op 0;          \
                    }                                           \
                    else                                        \
                        res = b->toNumber() op c->toNumber();   \
                }
#else
#define COMPARE(op)                                             \
                if (b->isNumber() && c->isNumber())             \
                {                                               \
                    if (Port::isnan(b->number) || Port::isnan(c->number)) \
                        res = FALSE;                            \
                    else                                        \
                        res = (b->number op c->number);         \
                }                                               \
                else                                            \
                {                                               \
                    b->toPrimitive(b, TypeNumber);              \
                    c->toPrimitive(c, TypeNumber);              \
                    if (b->isString() && c->isString())         \
                    {   d_string x = b->toString();             \
                        d_string y = c->toString();             \
                                                                \
                        res = Dstring::cmp(x, y) op 0;          \
                    }                                           \
                    else                                        \
                    {   d_number x = b->toNumber();             \
                        d_number y = c->toNumber();             \
                                                                \
                        if (Port::isnan(x) || Port::isnan(y))   \
                            res = FALSE;                        \
                        else                                    \
                            res = (x op y);                     \
                    }                                           \
                }
#endif

// We know the second argument is a number that is not NaN
#define COMPAREC(op)                                            \
                    {   d_number x = b->toNumber();             \
                                                                \
                        if (Port::isnan(x))                     \
                            res = FALSE;                        \
                        else                                    \
                            res = (x op *(d_number *)(code + 3)); \
                    }                                           \

            case IRclt:         // a = (b <   c)
                a = GETa(code);
                b = GETb(code);
                c = GETc(code);
                COMPARE( < );
                Vboolean::putValue(a, res);
                code += 4;
                break;

            case IRcle:         // a = (b <=  c)
                a = GETa(code);
                b = GETb(code);
                c = GETc(code);
                COMPARE( <= );
                Vboolean::putValue(a, res);
                code += 4;
                break;

            case IRcgt:         // a = (b >   c)
                a = GETa(code);
                b = GETb(code);
                c = GETc(code);
                COMPARE( > );
                Vboolean::putValue(a, res);
                code += 4;
                break;


            case IRcge:         // a = (b >=  c)
                a = GETa(code);
                b = GETb(code);
                c = GETc(code);
                COMPARE( >= );
                Vboolean::putValue(a, res);
                code += 4;
                break;

            case IRceq:         // a = (b ==  c)
            case IRcne:         // a = (b !=  c)
                a = GETa(code);
                b = GETb(code);
                c = GETc(code);
            Lagain:
                tx = b->getType();
                ty = c->getType();
                if (logflag) WPRINTF(L"tx('%ls', '%ls')\n", tx, ty);
                if (tx == ty)
                {
                    if (tx == TypeUndefined ||
                        tx == TypeNull)
                        res = TRUE;
                    else if (tx == TypeNumber)
                    {   d_number x = b->number;
                        d_number y = c->number;

#if defined __DMC__
                        res = (x == y);
#else
                        if (Port::isnan(x) || Port::isnan(y))
                        {   res = FALSE;
                            //goto Lceq;
                        }
                        else
                            res = (x == y);
#endif
                        //PRINTF("x = %g, y = %g, res = %d\n", x, y, res);
                    }
                    else if (tx == TypeString)
                    {
if (logflag)
{
WPRINTF(L"b = %x, c = %x\n", b, c);
                        WPRINTF(L"cmp('%s', '%s')\n", d_string_ptr(b->x.string), d_string_ptr(c->x.string));
                        WPRINTF(L"cmp(%d, %d)\n", d_string_len(b->x.string), d_string_len(c->x.string));
}
                        res = (Lstring::cmp(b->x.string, c->x.string) == 0);
                    }
                    else if (tx == TypeBoolean)
                        res = (b->boolean == c->boolean);
                    else // TypeObject
                    {
                        if (b->object->isDcomobject() && c->object->isDcomobject())
                            res = dcomobject_areEqual((Dcomobject*)b->object,
                                                      (Dcomobject*)c->object);
                        else
                            res = b->object == c->object;
                    }
                }
                else if (tx == TypeNull && ty == TypeUndefined)
                    res = TRUE;
                else if (tx == TypeUndefined && ty == TypeNull)
                    res = TRUE;
                else if (tx == TypeNumber && ty == TypeString)
                {
                    Vnumber::putValue(c, c->toNumber());
                    goto Lagain;
                }
                else if (tx == TypeString && ty == TypeNumber)
                {
                    Vnumber::putValue(b, b->toNumber());
                    goto Lagain;
                }
                else if (tx == TypeNull && ty == TypeObject && c->object->isDcomobject())
                {
                    res = dcomobject_isNull((Dcomobject*)(c->object));
                }
                else if (tx == TypeObject && b->object->isDcomobject() && ty == TypeNull)
                {
                    res = dcomobject_isNull((Dcomobject*)(b->object));
                }
                else if (tx == TypeBoolean)
                {
                    Vnumber::putValue(b, b->toNumber());
                    goto Lagain;
                }
                else if (ty == TypeBoolean)
                {
                    Vnumber::putValue(c, c->toNumber());
                    goto Lagain;
                }
                else if (ty == TypeObject)
                {
                    v = (Value *)c->toPrimitive(c, NULL);
                    if (v)
                    {   a = v;
                        goto Lthrow;
                    }
                    goto Lagain;
                }
                else if (tx == TypeObject)
                {
                    v = (Value *)b->toPrimitive(b, NULL);
                    if (v)
                    {   a = v;
                        goto Lthrow;
                    }
                    goto Lagain;
                }
                else
                {
                    res = FALSE;
                }

                res ^= (code->op.opcode == IRcne);
            //Lceq:
                Vboolean::putValue(a, res);
                code += 4;
                break;

            case IRcid:         // a = (b === c)
            case IRcnid:        // a = (b !== c)
                a = GETa(code);
                b = GETb(code);
                c = GETc(code);
                tx = b->getType();
                ty = c->getType();
                if (tx == ty)
                {
                    if (tx == TypeUndefined ||
                        tx == TypeNull)
                        res = TRUE;
                    else if (tx == TypeNumber)
                    {   d_number x = b->number;
                        d_number y = c->number;

#if defined __DMC__
                        // Ensure that a NAN operand produces FALSE
                        if (code->op.opcode == IRcid)
                            res = (x == y);
                        else
                            res = (x <> y);
                        goto Lcid;
#else
                        // Can't rely on compiler for correct NAN handling
                        if (Port::isnan(x) || Port::isnan(y))
                        {   res = FALSE;
                            goto Lcid;
                        }
                        else
                            res = (x == y);
#endif
                    }
                    else if (tx == TypeString)
                        res = (Lstring::cmp(b->x.string, c->x.string) == 0);
                    else if (tx == TypeBoolean)
                        res = (b->boolean == c->boolean);
                    else // TypeObject
                    {
                        if (b->object->isDcomobject() && c->object->isDcomobject())
                            res = dcomobject_areEqual((Dcomobject*)b->object,
                                                      (Dcomobject*)c->object);
                        else
                            res = b->object == c->object;
                    }
                }
                else
                {
                    res = FALSE;
                }

            Lcid:
                res ^= (code->op.opcode == IRcnid);
                Vboolean::putValue(a, res);
                code += 4;
                break;

            case IRjt:          // if (b) goto t
                b = GETb(code);
                if (b->toBoolean())
                    code += (code + 1)->offset;
                else
                    code += 3;
                break;

            case IRjf:          // if (!b) goto t
                b = GETb(code);
                if (!b->toBoolean())
                    code += (code + 1)->offset;
                else
                    code += 3;
                break;

            case IRjtb:         // if (b) goto t
                b = GETb(code);
                if (b->boolean)
                    code += (code + 1)->offset;
                else
                    code += 3;
                break;

            case IRjfb:         // if (!b) goto t
                b = GETb(code);
                if (!b->boolean)
                    code += (code + 1)->offset;
                else
                    code += 3;
                break;

            case IRjmp:
                code += (code + 1)->offset;
                break;

            case IRjlt:         // if (b <   c) goto c
                b = GETb(code);
                c = GETc(code);
                COMPARE( < );
		if (!res)
                    code += (code + 1)->offset;
                else
		    code += 4;
                break;

            case IRjle:         // if (b <=  c) goto c
                b = GETb(code);
                c = GETc(code);
                COMPARE( <= );
		if (!res)
                    code += (code + 1)->offset;
                else
		    code += 4;
                break;

            case IRjltc:        // if (b < constant) goto c
                b = GETb(code);
                COMPAREC( < );
		if (!res)
                    code += (code + 1)->offset;
                else
		    code += 5;
                break;

            case IRjlec:        // if (b <= constant) goto c
                b = GETb(code);
                COMPAREC( <= );
		if (!res)
                    code += (code + 1)->offset;
                else
		    code += 5;
                break;

            case IRiter:                // a = iter(b)
		a = GETa(code);
		b = GETb(code);
		o = b->toObject();
		if (!o)
		{
		    a = cannotConvert(b, GETlinnum(code));
		    goto Lthrow;
		}
		a = o->putIterator(a);
		if (a)
		    goto Lthrow;
		code += 3;
		break;

            case IRnext:        // a, b.c, iter
                                // if (!(b.c = iter)) goto a; iter = iter.next
                s = GETc(code)->toString();
                goto case_next;

            case IRnexts:       // a, b.s, iter
                s = (code + 3)->string;
            case_next:
                iter = (Iterator *)GETd(code);
                if (iter->done())
                    code += (code + 1)->offset;
                else
                {
                    v = iter->next();
                    b = GETb(code);
                    b->Put(s, v);
                    code += 5;
                }
                break;

            case IRnextscope:   // a, s, iter
                s = (code + 2)->string;
                iter = (Iterator *)GETc(code);
                if (iter->done())
                    code += (code + 1)->offset;
                else
                {
                    v = iter->next();
                    o = scope_tos(scope);
                    o->Put(s, v, 0);
                    code += 4;
                }
                break;

            case IRcall:        // a = b.c(argc, argv)
                s = GETc(code)->toString();
                goto case_call;

            case IRcalls:       // a = b.s(argc, argv)
                s = (code + 3)->string;
                goto case_call;

            case_call:
                a = GETa(code);
                b = GETb(code);
                o = b->toObject();
                if (!o)
		{
                    goto Lcallerror;
		}
                if (o->isDcomobject())
                {
//WPRINTF(L"dcomobject_call\n");
                    a = (Value *)dcomobject_call(s, cc, o, a, (code + 4)->index, GETe(code));
                }
                else
                {
//WPRINTF(L"v->call\n");
                    v = o->Get(s);
                    if (!v)
                        goto Lcallerror;
                    //WPRINTF(L"calling... '%ls'\n", v->toString());
                    cc->callerothis = othis;
		    Value::copy(a, &vundefined);
                    a = (Value *)v->Call(cc, o, a, (code + 4)->index, GETe(code));
		    //WPRINTF(L"regular call, a = %x\n", a);
                }
#if VERIFY
                assert(checksum == IR::verify(__LINE__, codestart));
#endif
                if (a)
                    goto Lthrow;
                code += 6;
                goto Lnext;

            Lcallerror:
	    {
                //WPRINTF(DTEXT("%ls %ls.%ls is undefined and has no Call method\n"), b->getType(), b->toString(), s);
		ErrInfo errinfo;
		a = Dobject::RuntimeError(&errinfo,
			ERR_UNDEFINED_NO_CALL3,
			b->getType(), d_string_ptr(b->toString()),
			d_string_ptr(s));
                goto Lthrow;
	    }

            case IRcallscope:   // a = s(argc, argv)
                s = (code + 2)->string;
                a = GETa(code);
                v = scope_get_lambda(scope, s, Vstring::calcHash(s), &o);
                if (!v)
                {
		    ErrInfo errinfo;
                    a = Dobject::RuntimeError(&errinfo, ERR_UNDEFINED_NO_CALL2, DTEXT("property"), d_string_ptr(s));
                    goto Lthrow;
                }
                // Should we pass othis or o? I think othis.
                cc->callerothis = othis;        // pass othis to eval()
		Value::copy(a, &vundefined);
                a = (Value *)v->Call(cc, o, a, (code + 3)->index, GETd(code));
		//WPRINTF(L"callscope result = %x\n", a);
#if VERIFY
                assert(checksum == IR::verify(__LINE__, codestart));
#endif
                if (a)
                    goto Lthrow;
                code += 5;
                goto Lnext;

	    case IRcallv:	// v(argc, argv) = a
		a = GETa(code);
                b = GETb(code);
                o = b->toObject();
                if (!o)
		{
		    //WPRINTF(DTEXT("%ls %ls is undefined and has no Call method\n"), b->getType(), b->toString());
		    ErrInfo errinfo;
		    a = Dobject::RuntimeError(&errinfo,
			    ERR_UNDEFINED_NO_CALL2,
			    b->getType(), d_string_ptr(b->toString()));
		    goto Lthrow;
		}
                cc->callerothis = othis;        // pass othis to eval()
		Value::copy(a, &vundefined);
		a = (Value *)o->Call(cc, o, a, (code + 3)->index, GETd(code));
                if (a)
                    goto Lthrow;
                code += 5;
		goto Lnext;

            case IRputcall:        // b.c(argc, argv) = a
                s = GETc(code)->toString();
                goto case_putcall;

            case IRputcalls:       //  b.s(argc, argv) = a
                s = (code + 3)->string;
                goto case_putcall;

            case_putcall:
		a = GETa(code);
                b = GETb(code);
                o = b->toObject();
                if (!o)
                    goto Lcallerror;
		v = o->GetLambda(s, Vstring::calcHash(s));
		if (!v)
		    goto Lcallerror;
		//WPRINTF(L"calling... '%ls'\n", v->toString());
		o = v->toObject();
		if (!o)
		{
		    ErrInfo errinfo;
		    a = Dobject::RuntimeError(&errinfo,
			    ERR_CANNOT_ASSIGN_TO2,
			    b->getType(), d_string_ptr(s));
		    goto Lthrow;
		}
		a = (Value *)o->put_Value(a, (code + 4)->index, GETe(code));
                if (a)
                    goto Lthrow;
                code += 6;
		goto Lnext;

            case IRputcallscope:   // a = s(argc, argv)
                s = (code + 2)->string;
                v = scope_get_lambda(scope, s, Vstring::calcHash(s), &o);
                if (!v)
                {
		    ErrInfo errinfo;
		    a = Dobject::RuntimeError(&errinfo,
			    ERR_UNDEFINED_NO_CALL2,
			    DTEXT("property"), d_string_ptr(s));
                    goto Lthrow;
                }
		o = v->toObject();
		if (!o)
		{
		    ErrInfo errinfo;
		    a = Dobject::RuntimeError(&errinfo,
			    ERR_CANNOT_ASSIGN_TO,
			    d_string_ptr(s));
		    goto Lthrow;
		}
                a = (Value *)o->put_Value(GETa(code), (code + 3)->index, GETd(code));
                if (a)
                    goto Lthrow;
		code += 5;
		goto Lnext;

	    case IRputcallv:	// v(argc, argv) = a
                b = GETb(code);
                o = b->toObject();
                if (!o)
		{
		    //WPRINTF(DTEXT("%ls %ls is undefined and has no Call method\n"), b->getType(), b->toString());
		    ErrInfo errinfo;
		    a = Dobject::RuntimeError(&errinfo,
			    ERR_UNDEFINED_NO_CALL2,
			    b->getType(), d_string_ptr(b->toString()));
		    goto Lthrow;
		}
		a = (Value *)o->put_Value(GETa(code), (code + 3)->index, GETd(code));
                if (a)
                    goto Lthrow;
                code += 5;
		goto Lnext;

            case IRnew: // a = new b(argc, argv)
                a = GETa(code);
                b = GETb(code);
		Value::copy(a, &vundefined);
                a = (Value *)b->Construct(cc, a, (code + 3)->index, GETd(code));
#if VERIFY
                assert(checksum == IR::verify(__LINE__, codestart));
#endif
                if (a)
                    goto Lthrow;
                code += 5;
                goto Lnext;

	    case IRpush:
		SCOPECACHE_CLEAR();
		a = GETa(code);
		o = a->toObject();
		if (!o)
		{
		    a = cannotConvert(a, GETlinnum(code));
		    goto Lthrow;
		}
		scope->push(o);		// push entry onto scope chain
		code += 2;
		break;

            case IRpop:
		SCOPECACHE_CLEAR();
                o = (Dobject *)scope->pop();    // pop entry off scope chain
                // If it's a Finally, we need to execute
                // the finally block
                code += 1;
                if (o->isFinally())  // test could be eliminated with virtual func
                {
		    f = (Finally *)o;
		    cc->finallyret = 0;
                    a = (Value *)call(cc, othis, f->finallyblock, ret, locals);
#if VERIFY
		    assert(checksum == IR::verify(__LINE__, codestart));
#endif
                    if (a)
                        goto Lthrow;
		    if (cc->finallyret)
			cc->finallyret = 0;
		    else
		    {	// The rest of any unwinding is already done
			return NULL;
		    }
                }
                goto Lnext;

            case IRfinallyret:
		cc->finallyret = 1;
            case IRret:
		//printf("scopecache_cnt = %d\n", scopecache_cnt);
                return NULL;

            case IRretexp:
                a = GETa(code);
                Value::copy(ret, a);
                //WPRINTF(L"returns: %ls\n", ret->toString());
                return NULL;

            case IRimpret:
                a = GETa(code);
                Value::copy(ret, a);
                //WPRINTF(L"implicit return: %ls\n", ret->toString());
		code += 2;
                goto Lnext;

            case IRthrow:
                a = GETa(code);
		cc->linnum = GETlinnum(code);
            Lthrow:
		//WPRINTF(L"Lthrow: linnum = %d\n", GETlinnum(code));
		a->getErrInfo(NULL, GETlinnum(code));
		SCOPECACHE_CLEAR();
                for (;;)
                {
                    if (scope->dim <= dimsave)
                    {
                        Value::copy(ret, &vundefined);
                        // 'a' may be pointing into the stack, which means
                        // it gets scrambled on return. Therefore, we copy
                        // its contents into a safe area in CallContext.
                        assert(sizeof(cc->value) == sizeof(Value));
                        Value::copy((Value *)cc->value, a);
                        return (Value *)cc->value;
                    }
                    o = (Dobject *)scope->pop();
                    if (o->isCatch())
                    {	ca = (Catch *)o;
			//WPRINTF(L"catch('%s')\n", d_string_ptr(ca->name));
			GC_LOG();
                        o = new(cc) Dobject(Dobject::getPrototype());
#if JSCRIPT_CATCH_BUG
			PutValue(cc, ca->name, a);
#else
                        o->Put(ca->name, a, DontDelete);
#endif
                        scope->push(o);
                        code = codestart + ca->offset;
                        break;
                    }
                    else
                    {
                        if (o->isFinally())
                        {
			    f = (Finally *)o;
                            v = (Value *)call(cc, othis, f->finallyblock, ret, locals);
                            if (v)
                            {   a = v;
                                //PRINTF("changing a\n");
                            }
                        }
                    }
                }
                goto Lnext;

            case IRtrycatch:
		SCOPECACHE_CLEAR();
                offset = (code - codestart) + (code + 1)->offset;
                s = (code + 2)->string;
		GC_LOG();
                ca = new(cc) Catch(offset, s);
                scope->push(ca);
                code += 3;
                break;

            case IRtryfinally:
		SCOPECACHE_CLEAR();
		GC_LOG();
                f = new(cc) Finally(code + (code + 1)->offset);
                scope->push(f);
                code += 2;
                break;

            case IRassert:
	    {
		ErrInfo errinfo;
		errinfo.linnum = (code + 1)->index;
#if 0 // Not supported under Chcom
                a = Dobject::RuntimeError(&errinfo, ERR_ASSERT, (code + 1)->index);
		goto Lthrow;
#else
                RuntimeErrorx(ERR_ASSERT, (code + 1)->index);
#endif
                code += 2;
                break;
	    }

            default:
                //PRINTF("1: Unrecognized IR instruction %d\n", code->op.opcode);
                assert(0);              // unrecognized IR instruction
        }
    }

Linterrupt:
    Value::copy(ret, &vundefined);
    return NULL;
}

/*******************************************
 * This is a 'disassembler' for our interpreted code.
 * Useful for debugging.
 */

void IR::print(unsigned address, IR *code)
{
    switch (code->op.opcode)
    {
        case IRerror:
            PRINTF("\tIRerror\n");
            break;

        case IRnop:
            PRINTF("\tIRnop\n");
            break;

        case IRend:
            PRINTF("\tIRend\n");
            break;

        case IRget:             // a = b.c
            PRINTF("\tIRget       %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRput:             // b.c = a
            PRINTF("\tIRput       %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRgets:            // a = b.s
#if defined UNICODE
            WPRINTF(L"\tIRgets      %d, %d, '%ls'\n",(code + 1)->index, (code + 2)->index, d_string_ptr((code + 3)->string));
#else
            PRINTF("\tIRgets      %d, %d, '%s'\n",(code + 1)->index, (code + 2)->index, d_string_ptr((code + 3)->string));
#endif
            break;

        case IRgetscope:        // a = othis->ident
#if defined UNICODE
            WPRINTF(L"\tIRgetscope  %d, '%ls', hash=%d\n",(code + 1)->index,d_string_ptr((code + 2)->string),(code + 3)->index);
#else
            PRINTF("\tIRgetscope  %d, '%s', hash=%d\n",(code + 1)->index,d_string_ptr((code + 2)->string),(code + 3)->index);
#endif
            break;

        case IRaddass:          // b.c += a
            PRINTF("\tIRaddass    %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRaddasss:         // b.s += a
            WPRINTF(L"\tIRaddasss   %d, %d, '%s'\n",(code + 1)->index, (code + 2)->index, d_string_ptr((code + 3)->string));
            break;

        case IRaddassscope:     // othis->ident += a
#if defined UNICODE
            WPRINTF(L"\tIRaddassscope  %d, '%ls', hash=%d\n",(code + 1)->index,d_string_ptr((code + 2)->string),(code + 3)->index);
#else
            PRINTF("\tIRaddassscope  %d, '%s', hash=%d\n",(code + 1)->index,d_string_ptr((code + 2)->string),(code + 3)->index);
#endif
            break;

        case IRputs:            // b.s = a
#if defined UNICODE
            WPRINTF(L"\tIRputs      %d, %d, '%ls'\n",(code + 1)->index, (code + 2)->index, d_string_ptr((code + 3)->string));
#else
            PRINTF("\tIRputs      %d, %d, '%s'\n",(code + 1)->index, (code + 2)->index, d_string_ptr((code + 3)->string));
#endif
            break;

        case IRputscope:        // s = a
#if defined UNICODE
            WPRINTF(L"\tIRputscope  %d, '%ls'\n",(code + 1)->index, d_string_ptr((code + 2)->string));
#else
            PRINTF("\tIRputscope  %d, '%s'\n",(code + 1)->index, d_string_ptr((code + 2)->string));
#endif
            break;

        case IRputdefault:            // b = a
#if defined UNICODE
            WPRINTF(L"\tIRputdefault %d, %d\n",(code + 1)->index, (code + 2)->index);
#else
            PRINTF("\tIRputdefault %d, %d\n",(code + 1)->index, (code + 2)->index);
#endif
            break;

        case IRputthis:         // b = s
#if defined UNICODE 
            WPRINTF(L"\tIRputthis   '%ls', %d\n",d_string_ptr((code + 2)->string),(code + 1)->index);
#else
            PRINTF("\tIRputthis   '%s', %d\n",d_string_ptr((code + 2)->string),(code + 1)->index);
#endif
            break;

        case IRmov:             // a = b
            PRINTF("\tIRmov       %d, %d\n", (code + 1)->index, (code + 2)->index);
            break;

        case IRstring:          // a = "string"
#if defined UNICODE
            WPRINTF(L"\tIRstring    %d, '%ls'\n",(code + 1)->index,d_string_ptr((code + 2)->string));
#else
            PRINTF("\tIRstring    %d, '%s'\n",(code + 1)->index,d_string_ptr((code + 2)->string));
#endif
            break;

        case IRobject:          // a = object
            PRINTF("\tIRobject    %d, %p\n",(code + 1)->index,(code + 2)->object);
            break;

        case IRthis:            // a = this
            PRINTF("\tIRthis      %d\n",(code + 1)->index);
            break;

        case IRnumber:          // a = number
            PRINTF("\tIRnumber    %d, %g\n",(code + 1)->index,*(d_number *)(code + 2));
            break;

        case IRboolean:         // a = boolean
            PRINTF("\tIRboolean   %d, %d\n",(code + 1)->index, (code + 2)->boolean);
            break;

        case IRnull:            // a = null
            PRINTF("\tIRnull      %d\n",(code + 1)->index);
            break;

        case IRundefined:       // a = undefined
            PRINTF("\tIRundefined %d\n",(code + 1)->index);
            break;

        case IRthisget:         // a = othis->ident
            PRINTF("\tIRthisget   %d, '%s'\n",(code + 1)->index,d_string_ptr((code + 2)->string));
            break;

        case IRneg:             // a = -a
            PRINTF("\tIRneg      %d\n",(code + 1)->index);
            break;

        case IRpos:             // a = a
            PRINTF("\tIRpos      %d\n",(code + 1)->index);
            break;

        case IRcom:             // a = ~a
            PRINTF("\tIRcom      %d\n",(code + 1)->index);
            break;

        case IRnot:             // a = !a
            PRINTF("\tIRnot      %d\n",(code + 1)->index);
            break;

        case IRtypeof:          // a = typeof a
            PRINTF("\tIRtypeof   %d\n", (code + 1)->index);
            break;

        case IRinstance:        // a = b instanceof c
            PRINTF("\tIRinstance  %d, %d, %d\n", (code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRadd:             // a = b + c
            PRINTF("\tIRadd       %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRsub:             // a = b - c
            PRINTF("\tIRsub       %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRmul:             // a = b * c
            PRINTF("\tIRmul       %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRdiv:             // a = b / c
            PRINTF("\tIRdiv       %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRmod:             // a = b % c
            PRINTF("\tIRmod       %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRshl:             // a = b << c
            PRINTF("\tIRshl       %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRshr:             // a = b >> c
            PRINTF("\tIRshr       %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRushr:            // a = b >>> c
            PRINTF("\tIRushr      %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRand:             // a = b & c
            PRINTF("\tIRand       %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRor:              // a = b | c
            PRINTF("\tIRor        %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRxor:             // a = b ^ c
            PRINTF("\tIRxor       %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRpreinc:		// a = ++b.c
            PRINTF("\tIRpreinc  %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRpreincs:        // a = ++b.s
            WPRINTF(L"\tIRpreincs %d, %d, %s\n",(code + 1)->index, (code + 2)->index, d_string_ptr((code + 3)->string));
            break;

        case IRpreincscope:    // a = ++s
            WPRINTF(L"\tIRpreincscope %d, '%s', hash=%d\n",(code + 1)->index, d_string_ptr((code + 2)->string), (code + 3)->hash);
            break;

        case IRpredec:         // a = --b.c
            PRINTF("\tIRpredec  %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRpredecs:        // a = --b.s
            WPRINTF(L"\tIRpredecs %d, %d, %s\n",(code + 1)->index, (code + 2)->index, d_string_ptr((code + 3)->string));
            break;

        case IRpredecscope:    // a = --s
            WPRINTF(L"\tIRpredecscope %d, '%s', hash=%d\n",(code + 1)->index, d_string_ptr((code + 2)->string), (code + 3)->hash);
            break;

        case IRpostinc: // a = b.c++
            PRINTF("\tIRpostinc  %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRpostincs:        // a = b.s++
            PRINTF("\tIRpostincs %d, %d, %s\n",(code + 1)->index, (code + 2)->index, d_string_ptr((code + 3)->string));
            break;

        case IRpostincscope:    // a = s++
            WPRINTF(L"\tIRpostincscope %d, %s\n",(code + 1)->index, d_string_ptr((code + 2)->string));
            break;

        case IRpostdec:         // a = b.c--
            PRINTF("\tIRpostdec  %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRpostdecs:        // a = b.s--
            PRINTF("\tIRpostdecs %d, %d, %s\n",(code + 1)->index, (code + 2)->index, d_string_ptr((code + 3)->string));
            break;

        case IRpostdecscope:    // a = s--
            PRINTF("\tIRpostdecscope %d, %s\n",(code + 1)->index, d_string_ptr((code + 2)->string));
            break;

        case IRdel:             // a = delete b.c
            PRINTF("\tIRdel       %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRdels:            // a = delete b.s
#if defined UNICODE
            WPRINTF(L"\tIRdels      %d, %d, '%ls'\n",(code + 1)->index, (code + 2)->index, d_string_ptr((code + 3)->string));
#else
            PRINTF("\tIRdels      %d, %d, '%s'\n",(code + 1)->index, (code + 2)->index, d_string_ptr((code + 3)->string));
#endif
            break;

        case IRdelscope:        // a = delete s
#if defined UNICODE
            WPRINTF(L"\tIRdelscope  %d, '%ls'\n",(code + 1)->index, d_string_ptr((code + 2)->string));
#else
            PRINTF("\tIRdelscope  %d, '%s'\n",(code + 1)->index, d_string_ptr((code + 2)->string));
#endif
            break;

        case IRclt:             // a = (b <   c)
            PRINTF("\tIRclt       %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRcle:             // a = (b <=  c)
            PRINTF("\tIRcle       %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRcgt:             // a = (b >   c)
            PRINTF("\tIRcgt       %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRcge:             // a = (b >=  c)
            PRINTF("\tIRcge       %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRceq:             // a = (b ==  c)
            PRINTF("\tIRceq       %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRcne:             // a = (b !=  c)
            PRINTF("\tIRcne       %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRcid:             // a = (b === c)
            PRINTF("\tIRcid       %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRcnid:    // a = (b !== c)
            PRINTF("\tIRcnid      %d, %d, %d\n",(code + 1)->index, (code + 2)->index, (code + 3)->index);
            break;

        case IRjt:              // if (b) goto t
            PRINTF("\tIRjt        %d, %d\n", (code + 1)->index + address, (code + 2)->index);
            break;

        case IRjf:              // if (!b) goto t
            PRINTF("\tIRjf        %d, %d\n", (code + 1)->index + address, (code + 2)->index);
            break;

        case IRjtb:             // if (b) goto t
            PRINTF("\tIRjtb       %d, %d\n", (code + 1)->index + address, (code + 2)->index);
            break;

        case IRjfb:             // if (!b) goto t
            PRINTF("\tIRjfb       %d, %d\n", (code + 1)->index + address, (code + 2)->index);
            break;

        case IRjmp:
            PRINTF("\tIRjmp       %d\n", (code + 1)->offset + address);
            break;

        case IRjlt:             // if (b < c) goto t
            PRINTF("\tIRjlt       %d, %d, %d\n",(code + 1)->index + address, (code + 2)->index, (code + 3)->index);
            break;

        case IRjle:             // if (b <= c) goto t
            PRINTF("\tIRjle       %d, %d, %d\n",(code + 1)->index + address, (code + 2)->index, (code + 3)->index);
            break;

        case IRjltc:            // if (b < constant) goto t
            PRINTF("\tIRjltc      %d, %d, %g\n",(code + 1)->index + address, (code + 2)->index, *(d_number *)(code + 3));
            break;

        case IRjlec:            // if (b <= constant) goto t
            PRINTF("\tIRjlec      %d, %d, %g\n",(code + 1)->index + address, (code + 2)->index, *(d_number *)(code + 3));
            break;

        case IRiter:            // a = iter(b)
            PRINTF("\tIRiter    %d, %d\n",(code + 1)->index, (code + 2)->index);
            break;

        case IRnext:            // a, b.c, iter
            PRINTF("\tIRnext    %d, %d, %d, %d\n",
                    (code + 1)->index,
                    (code + 2)->index,
                    (code + 3)->index,
                    (code + 4)->index);
            break;

        case IRnexts:           // a, b.s, iter
            PRINTF("\tIRnexts   %d, %d, '%s', %d\n",
                    (code + 1)->index,
                    (code + 2)->index,
                    d_string_ptr((code + 3)->string),
                    (code + 4)->index);
            break;

        case IRnextscope:       // a, s, iter
#if defined UNICODE
            WPRINTF
#else
            PRINTF
#endif
                (DTEXT("\tIRnextscope   %d, '%s', %d\n"),
                    (code + 1)->index,
                    d_string_ptr((code + 2)->string),
                    (code + 3)->index);
            break;

        case IRcall:            // a = b.c(argc, argv)
            PRINTF("\tIRcall     %d,%d,%d, argc=%d, argv=%d \n",
                    (code + 1)->index,
                    (code + 2)->index,
                    (code + 3)->index,
                    (code + 4)->index,
                    (code + 5)->index);
            break;

        case IRcalls:           // a = b.s(argc, argv)
            WPRINTF
                (DTEXT("\tIRcalls     %d,%d,'%s', argc=%d, argv=%d \n"),
                    (code + 1)->index,
                    (code + 2)->index,
                    d_string_ptr((code + 3)->string),
                    (code + 4)->index,
                    (code + 5)->index);
            break;

        case IRcallscope:       // a = s(argc, argv)
            WPRINTF
                (DTEXT("\tIRcallscope %d,'%s', argc=%d, argv=%d \n"),
                    (code + 1)->index,
                    d_string_ptr((code + 2)->string),
                    (code + 3)->index,
                    (code + 4)->index);
            break;

        case IRputcall:            // a = b.c(argc, argv)
            PRINTF("\tIRputcall  %d,%d,%d, argc=%d, argv=%d \n",
                    (code + 1)->index,
                    (code + 2)->index,
                    (code + 3)->index,
                    (code + 4)->index,
                    (code + 5)->index);
            break;

        case IRputcalls:           // a = b.s(argc, argv)
            WPRINTF
                (DTEXT("\tIRputcalls  %d,%d,'%s', argc=%d, argv=%d \n"),
                    (code + 1)->index,
                    (code + 2)->index,
                    d_string_ptr((code + 3)->string),
                    (code + 4)->index,
                    (code + 5)->index);
            break;

        case IRputcallscope:       // a = s(argc, argv)
            WPRINTF
                (DTEXT("\tIRputcallscope %d,'%s', argc=%d, argv=%d \n"),
                    (code + 1)->index,
                    d_string_ptr((code + 2)->string),
                    (code + 3)->index,
                    (code + 4)->index);
            break;

        case IRcallv:           // a = v(argc, argv)
            PRINTF("\tIRcallv    %d, %d(argc=%d, argv=%d)\n",
                    (code + 1)->index,
                    (code + 2)->index,
                    (code + 3)->index,
                    (code + 4)->index);
            break;

        case IRputcallv:           // a = v(argc, argv)
            PRINTF("\tIRputcallv %d, %d(argc=%d, argv=%d)\n",
                    (code + 1)->index,
                    (code + 2)->index,
                    (code + 3)->index,
                    (code + 4)->index);
            break;

        case IRnew:     // a = new b(argc, argv)
            PRINTF("\tIRnew      %d,%d, argc=%d, argv=%d \n",
                    (code + 1)->index,
                    (code + 2)->index,
                    (code + 3)->index,
                    (code + 4)->index);
            break;

        case IRpush:
            PRINTF("\tIRpush    %d\n",(code + 1)->index);
            break;

        case IRpop:
            PRINTF("\tIRpop\n");
            break;

        case IRret:
            PRINTF("\tIRret\n");
            return;

        case IRretexp:
            PRINTF("\tIRretexp    %d\n",(code + 1)->index);
            return;

        case IRimpret:
            PRINTF("\tIRimpret    %d\n",(code + 1)->index);
            return;

        case IRthrow:
            PRINTF("\tIRthrow     %d\n",(code + 1)->index);
            break;

        case IRassert:
            PRINTF("\tIRassert    %d\n",(code + 1)->index);
            break;

        case IRtrycatch:
            WPRINTF(L"\tIRtrycatch  %d, '%s'\n", (code + 1)->offset + address, d_string_ptr((code + 2)->string));
            break;

        case IRtryfinally:
            PRINTF("\tIRtryfinally %d\n", (code + 1)->offset + address);
            break;

        case IRfinallyret:
            PRINTF("\tIRfinallyret\n");
            break;

        default:
            WPRINTF(L"2: Unrecognized IR instruction %d\n", code->op.opcode);
            assert(0);          // unrecognized IR instruction
    }
}

/*********************************
 * Give size of opcode.
 */

unsigned IR::size(unsigned opcode)
{   unsigned sz = 9999;                 // initializer necessary for /W4

    switch (opcode)
    {
        case IRerror:
        case IRnop:
        case IRend:
            sz = 1;
            break;

        case IRget:             // a = b.c
        case IRaddass:
            sz = 4;
            break;

        case IRput:             // b.c = a
            sz = 4;
            break;

        case IRgets:            // a = b.s
        case IRaddasss:
            sz = 4;
            break;

        case IRgetscope:        // a = s
        case IRaddassscope:
            sz = 4;
            break;

        case IRputs:            // b.s = a
            sz = 4;
            break;

        case IRputscope:        // s = a
	case IRputdefault:	// b = a
            sz = 3;
            break;

        case IRputthis:         // a = s
            sz = 3;
            break;

        case IRmov:             // a = b
            sz = 3;
            break;

        case IRstring:          // a = "string"
            sz = 3;
            break;

        case IRobject:          // a = object
            sz = 3;
            break;

        case IRthis:            // a = this
            sz = 2;
            break;

        case IRnumber:          // a = number
            sz = 4;
            break;

        case IRboolean:         // a = boolean
            sz = 3;
            break;

        case IRnull:            // a = null
            sz = 2;
            break;

        case IRundefined:       // a = undefined
            sz = 2;
            break;

        case IRthisget:         // a = othis->ident
            sz = 3;
            break;

        case IRneg:             // a = -a
        case IRpos:             // a = a
        case IRcom:             // a = ~a
        case IRnot:             // a = !a
        case IRtypeof:          // a = typeof a
            sz = 2;
            break;

        case IRinstance:        // a = b instanceof c
        case IRadd:             // a = b + c
        case IRsub:             // a = b - c
        case IRmul:             // a = b * c
        case IRdiv:             // a = b / c
        case IRmod:             // a = b % c
        case IRshl:             // a = b << c
        case IRshr:             // a = b >> c
        case IRushr:            // a = b >>> c
        case IRand:             // a = b & c
        case IRor:              // a = b | c
        case IRxor:             // a = b ^ c
            sz = 4;
            break;

        case IRpreinc:         // a = ++b.c
        case IRpreincs:        // a = ++b.s
        case IRpredec:         // a = --b.c
        case IRpredecs:        // a = --b.s
        case IRpostinc:         // a = b.c++
        case IRpostincs:        // a = b.s++
        case IRpostdec:         // a = b.c--
        case IRpostdecs:        // a = b.s--
            sz = 4;
            break;

        case IRpostincscope:    // a = s++
        case IRpostdecscope:    // a = s--
            sz = 3;
            break;

        case IRpreincscope:	// a = ++s
        case IRpredecscope:	// a = --s
            sz = 4;
            break;

        case IRdel:             // a = delete b.c
        case IRdels:            // a = delete b.s
            sz = 4;
            break;

        case IRdelscope:        // a = delete s
            sz = 3;
            break;

        case IRclt:             // a = (b <   c)
        case IRcle:             // a = (b <=  c)
        case IRcgt:             // a = (b >   c)
        case IRcge:             // a = (b >=  c)
        case IRceq:             // a = (b ==  c)
        case IRcne:             // a = (b !=  c)
        case IRcid:             // a = (b === c)
        case IRcnid:            // a = (b !== c)
        case IRjlt:             // if (b < c) goto t
        case IRjle:             // if (b <= c) goto t
            sz = 4;
            break;

        case IRjltc:            // if (b < constant) goto t
        case IRjlec:            // if (b <= constant) goto t
            sz = 5;
            break;

        case IRjt:              // if (b) goto t
        case IRjf:              // if (!b) goto t
        case IRjtb:             // if (b) goto t
        case IRjfb:             // if (!b) goto t
            sz = 3;
            break;

        case IRjmp:
            sz = 2;
            break;

        case IRiter:            // a = iter(b)
            sz = 3;
            break;

        case IRnext:            // a, b.c, iter
        case IRnexts:           // a, b.s, iter
            sz = 5;
            break;

        case IRnextscope:       // a, s, iter
            sz = 4;
            break;

        case IRcall:            // a = b.c(argc, argv)
        case IRcalls:           // a = b.s(argc, argv)
        case IRputcall:         //  b.c(argc, argv) = a
        case IRputcalls:        //  b.s(argc, argv) = a
            sz = 6;
            break;

        case IRcallscope:       // a = s(argc, argv)
        case IRputcallscope:    // s(argc, argv) = a
	case IRcallv:
	case IRputcallv:
            sz = 5;
            break;

        case IRnew:             // a = new b(argc, argv)
            sz = 5;
            break;

        case IRpush:
            sz = 2;
            break;

        case IRpop:
            sz = 1;
            break;

        case IRfinallyret:
        case IRret:
            sz = 1;
            break;

        case IRretexp:
	case IRimpret:
        case IRthrow:
            sz = 2;
            break;

        case IRtrycatch:
            sz = 3;
            break;

        case IRtryfinally:
            sz = 2;
            break;

        case IRassert:
            sz = 2;
            break;

        default:
            PRINTF("3: Unrecognized IR instruction %d\n", opcode);
            assert(0);          // unrecognized IR instruction
    }
    assert(sz <= 6);
    return sz;
}

void IR::printfunc(IR *code)
{
    IR *codestart = code;

    for (;;)
    {
        PRINTF("%2d(%d):", code - codestart, code->op.linnum);
        print(code - codestart, code);
        if (code->op.opcode == IRend)
            return;
        code += size(code->op.opcode);
    }
}

/***************************************
 * Verify that it is a correct sequence of code.
 * Useful for isolating memory corruption bugs.
 */

unsigned IR::verify(unsigned linnum, IR *codestart)
{
#if VERIFY
    unsigned checksum = 0;
    unsigned sz;
    unsigned i;
    IR *code;

    // Verify code
    for (code = codestart;;)
    {
        switch (code->op.opcode)
        {
            case IRend:
                return checksum;

            case IRerror:
		PRINTF("verify failure line %u\n", linnum);
                assert(0);
                break;

            default:
                if ((unsigned)code->op.opcode >= IRMAX)
                {   PRINTF("undefined opcode %d in code %p\n", code->op.opcode, codestart);
                    assert(0);
                }
                sz = IR::size(code->op.opcode);
                for (i = 0; i < sz; i++)
                {   checksum += code->op.opcode;
                    code++;
                }
                break;
        }
    }
#else
    return 0;
#endif
}

