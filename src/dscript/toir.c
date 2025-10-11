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
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "mem.h"
#include "root.h"
#include "lstring.h"

#include "dscript.h"
#include "expression.h"
#include "statement.h"
#include "identifier.h"
#include "ir.h"
#include "dobject.h"
#include "text.h"

void Expression::toIR(IRstate *irs, unsigned ret)
{
    //PRINTF("Expression::toIR('%s')\n", toChars());
}

void Expression::toLvalue(IRstate *irs, unsigned *base, IR *property, int *opoff)
{
    *base = irs->alloc(1);
    toIR(irs, *base);
    property->index = 0;
    *opoff = 3;
}

void RealExpression::toIR(IRstate *irs, unsigned ret)
{
    //PRINTF("RealExpression::toIR(%g)\n", value);

    // Note: this depends on (sizeof(value) == 2 * sizeof(unsigned))
    if (ret)
	irs->gen(loc, IRnumber, 3, ret, value);
}

void IdentifierExpression::toIR(IRstate *irs, unsigned ret)
{
    d_string id = ident->toLstring();

    if (ret)
	irs->gen3(loc, IRgetscope, ret, (unsigned)id, d_string_hash(id));
}

void IdentifierExpression::toLvalue(IRstate *irs, unsigned *base, IR *property, int *opoff)
{
    //irs->gen1(loc, IRthis, base);
    property->string = ident->toLstring();
    *opoff = 2;
    *base = ~0u;
}

void ThisExpression::toIR(IRstate *irs, unsigned ret)
{
    if (ret)
	irs->gen1(loc, IRthis, ret);
}

void NullExpression::toIR(IRstate *irs, unsigned ret)
{
    if (ret)
	irs->gen1(loc, IRnull, ret);
}

void StringExpression::toIR(IRstate *irs, unsigned ret)
{
    if (ret)
	irs->gen2(loc, IRstring, ret, (unsigned)string);
}

void RegExpLiteral::toIR(IRstate *irs, unsigned ret)
{   d_string pattern;
    unsigned patlen;
    d_string attribute = d_string_null;
    dchar *p;
    dchar *e;
    unsigned len;

    unsigned argc;
    unsigned argv;
    unsigned b;

    // Regular expression is of the form:
    //	/pattern/attribute

    // Parse out pattern and attribute strings
    p = string->toDchars();
    len = string->len();
    assert(*p == '/');
    e = Dchar::rchr(p, '/');
    patlen = e - p - 1;
    pattern = Dstring::substring(p, 1, 1 + patlen);
    argc = 1;
    if (e[1])
    {	attribute = Dstring::substring(p, patlen + 2, len);
	argc++;
    }

    // Generate new Regexp(pattern [, attribute])

    b = irs->alloc(1);
    d_string re = TEXT_RegExp;
    irs->gen3(loc, IRgetscope, b, (unsigned)re, d_string_hash(re));
    argv = irs->alloc(argc);
    irs->gen2(loc, IRstring, argv, (unsigned)pattern);
    if (argc == 2)
	irs->gen2(loc, IRstring, argv + 1 * INDEX_FACTOR, (unsigned)attribute);
    irs->gen4(loc, IRnew, ret,b,argc,argv);
    irs->release(b, argc + 1);
}

void BooleanExpression::toIR(IRstate *irs, unsigned ret)
{
    if (ret)
	irs->gen2(loc, IRboolean, ret, boolean);
}


void ArrayLiteral::toIR(IRstate *irs, unsigned ret)
{
    unsigned argc;
    unsigned argv;
    unsigned b;
    unsigned v;

    b = irs->alloc(1);
    d_string ar = TEXT_Array;
    irs->gen3(loc, IRgetscope, b, (unsigned)ar, d_string_hash(ar));
    if (elements && elements->dim)
    {	Expression *e;

	argc = elements->dim;
	argv = irs->alloc(argc);
	if (argc > 1)
	{   unsigned i;

	    // array literal [a, b, c] is equivalent to:
	    //	new Array(a,b,c)
	    for (i = 0; i < argc; i++)
	    {
		e = (Expression *)elements->data[i];
		if (e)
		{
		    e->toIR(irs, argv + i * INDEX_FACTOR);
		}
		else
		    irs->gen1(loc, IRundefined, argv + i * INDEX_FACTOR);
	    }
	    irs->gen4(loc, IRnew, ret,b,argc,argv);
	}
	else
	{   //	[a] translates to:
	    //	ret = new Array(1);
	    //  ret[0] = a
	    irs->gen(loc, IRnumber, 3, argv, 1.0);
	    irs->gen4(loc, IRnew, ret,b,argc,argv);

	    e = (Expression *)elements->data[0];
	    v = irs->alloc(1);
	    if (e)
		e->toIR(irs, v);
	    else
		irs->gen1(loc, IRundefined, v);
	    irs->gen3(loc, IRputs, v, ret, (unsigned)TEXT_0);
	    irs->release(v, 1);
	}
	irs->release(argv, argc);
    }
    else
    {
	// Generate new Array()
	irs->gen4(loc, IRnew, ret,b,0,0);
    }
    irs->release(b, 1);
}

void ObjectLiteral::toIR(IRstate *irs, unsigned ret)
{
    unsigned b;

    b = irs->alloc(1);
    //irs->gen2(loc, IRstring, b, TEXT_Object);
    d_string ob = TEXT_Object;
    irs->gen3(loc, IRgetscope, b, (unsigned)ob, Vstring::calcHash(ob));
    // Generate new Object()
    irs->gen4(loc, IRnew, ret,b,0,0);
    if (fields && fields->dim)
    {
	Field *f;
	unsigned i;
	unsigned x;

	x = irs->alloc(1);
	for (i = 0; i < fields->dim; i++)
	{
	    f = (Field *)fields->data[i];
	    f->exp->toIR(irs, x);
	    irs->gen3(loc, IRputs, x, ret, (unsigned)f->ident->toLstring());
	}
    }
}

void FunctionLiteral::toIR(IRstate *irs, unsigned ret)
{
    func->toIR(NULL);
    irs->gen2(loc, IRobject, ret, (unsigned)func);
}

/**************************************************/

void PreIncExp::toIR(IRstate *irs, unsigned ret)
{
    unsigned base;
    IR property;
    int opoff;

    //PRINTF("PreIncExp::toIR('%s')\n", toChars());
    e1->toLvalue(irs, &base, &property, &opoff);
    assert(opoff != 3);
    if (opoff == 2)
    {
	//irs->gen2(loc, IRpreincscope, ret, property.index);
	irs->gen3(loc, IRpreincscope, ret, property.index, Vstring::calcHash((d_string&)property));
    }
    else
	irs->gen3(loc, IRpreinc + opoff, ret, base, property.index);
}

void PreDecExp::toIR(IRstate *irs, unsigned ret)
{
    unsigned base;
    IR property;
    int opoff;

    //PRINTF("PreDecExp::toIR('%s')\n", toChars());
    e1->toLvalue(irs, &base, &property, &opoff);
    assert(opoff != 3);
    if (opoff == 2)
    {
	//irs->gen2(loc, IRpredecscope, ret, property.index);
	irs->gen3(loc, IRpredecscope, ret, property.index, Vstring::calcHash((d_string&)property));
    }
    else
	irs->gen3(loc, IRpredec + opoff, ret, base, property.index);
}

/**************************************************/

void PostIncExp::toIR(IRstate *irs, unsigned ret)
{
    unsigned base;
    IR property;
    int opoff;

    //PRINTF("PostIncExp::toIR('%s')\n", toChars());
    e1->toLvalue(irs, &base, &property, &opoff);
    assert(opoff != 3);
    if (opoff == 2)
    {
	if (ret)
	{
	    irs->gen2(loc, IRpostincscope, ret, property.index);
	}
	else
	{
	    //irs->gen2(loc, IRpreincscope, ret, property.index);
	    irs->gen3(loc, IRpreincscope, ret, property.index, Vstring::calcHash((d_string&)property));
	}
    }
    else
	irs->gen3(loc, (ret ? IRpostinc : IRpreinc) + opoff, ret, base, property.index);
}

void PostDecExp::toIR(IRstate *irs, unsigned ret)
{
    unsigned base;
    IR property;
    int opoff;

    //PRINTF("PostDecExp::toIR('%s')\n", toChars());
    e1->toLvalue(irs, &base, &property, &opoff);
    assert(opoff != 3);
    if (opoff == 2)
    {
	if (ret)
	{
	    irs->gen2(loc, IRpostdecscope, ret, property.index);
	}
	else
	{
	    //irs->gen2(loc, IRpredecscope, ret, property.index);
	    irs->gen3(loc, IRpredecscope, ret, property.index, Vstring::calcHash((d_string&)property));
	}
    }
    else
	irs->gen3(loc, (ret ? IRpostdec : IRpredec) + opoff, ret, base, property.index);
}

/**************************************************/

void DotExp::toIR(IRstate *irs, unsigned ret)
{
    unsigned base;

    //PRINTF("DotExp::toIR('%s')\n", toChars());
#ifdef CHCOM
    // CHCOM test cases depend on things like:
    //		foo.bar;
    // generating a property get even if the result is thrown away.
    base = irs->alloc(1);
    e1->toIR(irs, base);
    irs->gen3(loc, IRgets, ret, base, (unsigned)ident->toLstring());
#else
    if (ret)
    {
	base = irs->alloc(1);
	e1->toIR(irs, base);
	irs->gen3(loc, IRgets, ret, base, (unsigned)ident->toLstring());
    }
    else
	e1->toIR(irs, 0);
#endif
}

void DotExp::toLvalue(IRstate *irs, unsigned *base, IR *property, int *opoff)
{
    *base = irs->alloc(1);
    e1->toIR(irs, *base);
    property->string = ident->toLstring();
    *opoff = 1;
}

void CallExp::toIR(IRstate *irs, unsigned ret)
{
    // ret = base.property(argc, argv)
    // CALL ret,base,property,argc,argv
    unsigned base;
    unsigned argc;
    unsigned argv;
    IR property;
    int opoff;

    //PRINTF("CallExp::toIR('%s')\n", toChars());
    e1->toLvalue(irs, &base, &property, &opoff);

    if (arguments)
    {	unsigned u;

	argc = arguments->dim;
	argv = irs->alloc(argc);
	for (u = 0; u < argc; u++)
	{   Expression *e;

	    e = (Expression *)arguments->data[u];
	    e->toIR(irs, argv + u * INDEX_FACTOR);
	}
	arguments->zero();		// release to GC
	arguments = NULL;
    }
    else
    {
	argc = 0;
	argv = 0;
    }

    if (opoff == 3)
	irs->gen4(loc, IRcallv, ret,base,argc,argv);
    else if (opoff == 2)
	irs->gen4(loc, IRcallscope, ret,property.index,argc,argv);
    else
	irs->gen(loc, IRcall + opoff, 5, ret,base,property,argc,argv);
    irs->release(argv, argc);
}

void AssertExp::toIR(IRstate *irs, unsigned ret)
{   unsigned linnum;
    unsigned u;
    unsigned b;

    b = ret ? ret : irs->alloc(1);

    e1->toIR(irs, b);
    u = irs->getIP();
    irs->gen2(loc, IRjt, 0, b);
    linnum = (unsigned)loc;
    irs->gen1(loc, IRassert, linnum);
    irs->patchJmp(u, irs->getIP());

    if (!ret)
	irs->release(b, 1);
}

void PosExp::toIR(IRstate *irs, unsigned ret)
{
    e1->toIR(irs, ret);
    if (ret)
	irs->gen1(loc, IRpos, ret);
}

void NegExp::toIR(IRstate *irs, unsigned ret)
{
    e1->toIR(irs, ret);
    if (ret)
	irs->gen1(loc, IRneg, ret);
}

void ComExp::toIR(IRstate *irs, unsigned ret)
{
    e1->toIR(irs, ret);
    if (ret)
	irs->gen1(loc, IRcom, ret);
}

void NotExp::toIR(IRstate *irs, unsigned ret)
{
    e1->toIR(irs, ret);
    if (ret)
	irs->gen1(loc, IRnot, ret);
}

void DeleteExp::toIR(IRstate *irs, unsigned ret)
{
    unsigned base;
    IR property;
    int opoff;

    e1->toLvalue(irs, &base, &property, &opoff);
    assert(opoff != 3);
    if (opoff == 2)
	irs->gen2(loc, IRdelscope, ret, property.index);
    else
	irs->gen3(loc, IRdel + opoff, ret, base, property.index);
}

void TypeofExp::toIR(IRstate *irs, unsigned ret)
{
    // ECMA 11.4.3
    e1->toIR(irs, ret);
    if (ret)
	irs->gen1(loc, IRtypeof, ret);
}

void VoidExp::toIR(IRstate *irs, unsigned ret)
{
    e1->toIR(irs, ret);
    if (ret)
	irs->gen1(loc, IRundefined, ret);
}

void CommaExp::toIR(IRstate *irs, unsigned ret)
{
    e1->toIR(irs, 0);
    e2->toIR(irs, ret);
}

void NewExp::toIR(IRstate *irs, unsigned ret)
{
    // ret = new b(argc, argv)
    // CALL ret,b,argc,argv
    unsigned b;
    unsigned argc;
    unsigned argv;

    //PRINTF("NewExp::toIR('%s')\n", toChars());
    b = irs->alloc(1);
    e1->toIR(irs, b);
    if (arguments)
    {	unsigned u;

	argc = arguments->dim;
	argv = irs->alloc(argc);
	for (u = 0; u < argc; u++)
	{   Expression *e;

	    e = (Expression *)arguments->data[u];
	    e->toIR(irs, argv + u * INDEX_FACTOR);
	}
    }
    else
    {
	argc = 0;
	argv = 0;
    }

    irs->gen4(loc, IRnew, ret,b,argc,argv);
    irs->release(argv, argc);
    irs->release(b, 1);
}

void ArrayExp::toIR(IRstate *irs, unsigned ret)
{   unsigned base;
    IR property;
    int opoff;

    if (ret)
    {
	toLvalue(irs, &base, &property, &opoff);
	assert(opoff != 3);
	if (opoff == 2)
	    irs->gen3(loc, IRgetscope, ret, property.index, Vstring::calcHash((d_string&)property));
	else
	    irs->gen3(loc, IRget + opoff, ret, base, property.index);
    }
    else
    {
	e1->toIR(irs, 0);
	e2->toIR(irs, 0);
    }
}

void ArrayExp::toLvalue(IRstate *irs, unsigned *base, IR *property, int *opoff)
{   unsigned index;

    *base = irs->alloc(1);
    e1->toIR(irs, *base);
    index = irs->alloc(1);
    e2->toIR(irs, index);
    property->index = index;
    *opoff = 0;
}

void AssignExp::toIR(IRstate *irs, unsigned ret)
{
    unsigned b;

    //PRINTF("AssignExp::toIR('%s')\n", toChars());
    if (e1->op == TOKcall)		// if CallExp
    {
#if DYNAMIC_CAST
	assert(dynamic_cast<CallExp *>(e1));	// make sure we got it right
#endif
	// Special case a function call as an lvalue.
	// This can happen if:
	//	foo() = 3;
	// A Microsoft extension, it means to assign 3 to the default property of
	// the object returned by foo(). It only has meaning for com objects.
	// This functionality should be worked into toLvalue() if it gets used
	// elsewhere.

	unsigned base;
	unsigned argc;
	unsigned argv;
	IR property;
	int opoff;
	CallExp *ec = (CallExp *)e1;

	if (ec->arguments)
	    argc = ec->arguments->dim + 1;
	else
	    argc = 1;

	argv = irs->alloc(argc);

	e2->toIR(irs, argv + (argc - 1) * INDEX_FACTOR);

	ec->e1->toLvalue(irs, &base, &property, &opoff);

	if (ec->arguments)
	{   unsigned u;

	    for (u = 0; u < ec->arguments->dim; u++)
	    {   Expression *e;

		e = (Expression *)ec->arguments->data[u];
		e->toIR(irs, argv + (u + 0) * INDEX_FACTOR);
	    }
	    ec->arguments->zero();		// release to GC
	    ec->arguments = NULL;
	}

	if (opoff == 3)
	    irs->gen4(loc, IRputcallv, ret,base,argc,argv);
	else if (opoff == 2)
	    irs->gen4(loc, IRputcallscope, ret,property.index,argc,argv);
	else
	    irs->gen(loc, IRputcall + opoff, 5, ret,base,property,argc,argv);
	irs->release(argv, argc);
    }
    else
    {
	unsigned base;
	IR property;
	int opoff;

	b = ret ? ret : irs->alloc(1);
	e2->toIR(irs, b);

	e1->toLvalue(irs, &base, &property, &opoff);
	assert(opoff != 3);
	if (opoff == 2)
	    irs->gen2(loc, IRputscope, b, property.index);
	else
	    irs->gen3(loc, IRput + opoff, b, base, property.index);
	if (!ret)
	    irs->release(b, 1);
    }
}

void AddAssignExp::toIR(IRstate *irs, unsigned ret)
{
    if (ret == 0 && e2->op == TOKreal &&
	((RealExpression *)e2)->value == 1)
    {
	unsigned base;
	IR property;
	int opoff;

	//PRINTF("AddAssign to PostInc('%s')\n", toChars());
	e1->toLvalue(irs, &base, &property, &opoff);
	assert(opoff != 3);
	if (opoff == 2)
	    irs->gen2(loc, IRpostincscope, ret, property.index);
	else
	    irs->gen3(loc, IRpostinc + opoff, ret, base, property.index);
    }
    else
    {
	unsigned r;
	unsigned base;
	IR property;
	int opoff;

	//PRINTF("AddAssignExp::toIR('%s')\n", toChars());
	e1->toLvalue(irs, &base, &property, &opoff);
	assert(opoff != 3);
	r = ret ? ret : irs->alloc(1);
	e2->toIR(irs, r);
	if (opoff == 2)
	    irs->gen3(loc, IRaddassscope, r, property.index, Vstring::calcHash((d_string&)property));
	else
	    irs->gen3(loc, IRaddass + opoff, r, base, property.index);
	if (!ret)
	    irs->release(r, 1);
    }
}

void MinAssignExp::toIR(IRstate *irs, unsigned ret)
{
    binAssignIR(irs, ret, IRsub);
}

void MulAssignExp::toIR(IRstate *irs, unsigned ret)
{
    binAssignIR(irs, ret, IRmul);
}

void DivAssignExp::toIR(IRstate *irs, unsigned ret)
{
    binAssignIR(irs, ret, IRdiv);
}

void ModAssignExp::toIR(IRstate *irs, unsigned ret)
{
    binAssignIR(irs, ret, IRmod);
}

void ShlAssignExp::toIR(IRstate *irs, unsigned ret)
{
    binAssignIR(irs, ret, IRshl);
}

void ShrAssignExp::toIR(IRstate *irs, unsigned ret)
{
    binAssignIR(irs, ret, IRshr);
}

void UshrAssignExp::toIR(IRstate *irs, unsigned ret)
{
    binAssignIR(irs, ret, IRushr);
}

void AndAssignExp::toIR(IRstate *irs, unsigned ret)
{
    binAssignIR(irs, ret, IRand);
}

void OrAssignExp::toIR(IRstate *irs, unsigned ret)
{
    binAssignIR(irs, ret, IRor);
}

void XorAssignExp::toIR(IRstate *irs, unsigned ret)
{
    binAssignIR(irs, ret, IRxor);
}

void BinExp::binAssignIR(IRstate *irs, unsigned ret, unsigned ircode)
{
    unsigned b;
    unsigned c;
    unsigned r;
    unsigned base;
    IR property;
    int opoff;

    //PRINTF("BinExp::binAssignIR('%s')\n", toChars());
    e1->toLvalue(irs, &base, &property, &opoff);
    assert(opoff != 3);
    b = irs->alloc(1);
    if (opoff == 2)
	irs->gen3(loc, IRgetscope, b, property.index, Vstring::calcHash((d_string&)property));
    else
	irs->gen3(loc, IRget + opoff, b, base, property.index);
    c = irs->alloc(1);
    e2->toIR(irs, c);
    r = ret ? ret : irs->alloc(1);
    irs->gen3(loc, ircode, r, b, c);
    if (opoff == 2)
	irs->gen2(loc, IRputscope, r, property.index);
    else
	irs->gen3(loc, IRput + opoff, r, base, property.index);
    if (!ret)
	irs->release(r, 1);
}

void BinExp::binIR(IRstate *irs, unsigned ret, unsigned ircode)
{   unsigned b;
    unsigned c;

    if (ret)
    {
	b = irs->alloc(1);
	e1->toIR(irs, b);
	if (e1->match(e2))
	{
	    irs->gen3(loc, ircode, ret, b, b);
	}
	else
	{
	    c = irs->alloc(1);
	    e2->toIR(irs, c);
	    irs->gen3(loc, ircode, ret, b, c);
	    irs->release(c, 1);
	}
	irs->release(b, 1);
    }
    else
    {
	e1->toIR(irs, 0);
	e2->toIR(irs, 0);
    }
}

void MulExp::toIR(IRstate *irs, unsigned ret)
{
    binIR(irs, ret, IRmul);
}

void AddExp::toIR(IRstate *irs, unsigned ret)
{
    binIR(irs, ret, IRadd);
}

void MinExp::toIR(IRstate *irs, unsigned ret)
{
    binIR(irs, ret, IRsub);
}

void DivExp::toIR(IRstate *irs, unsigned ret)
{
    binIR(irs, ret, IRdiv);
}

void ModExp::toIR(IRstate *irs, unsigned ret)
{
    binIR(irs, ret, IRmod);
}

void ShlExp::toIR(IRstate *irs, unsigned ret)
{
    binIR(irs, ret, IRshl);
}

void ShrExp::toIR(IRstate *irs, unsigned ret)
{
    binIR(irs, ret, IRshr);
}

void UshrExp::toIR(IRstate *irs, unsigned ret)
{
    binIR(irs, ret, IRushr);
}

void AndExp::toIR(IRstate *irs, unsigned ret)
{
    binIR(irs, ret, IRand);
}

void OrExp::toIR(IRstate *irs, unsigned ret)
{
    binIR(irs, ret, IRor);
}

void XorExp::toIR(IRstate *irs, unsigned ret)
{
    binIR(irs, ret, IRxor);
}

void LessExp::toIR(IRstate *irs, unsigned ret)
{
    binIR(irs, ret, IRclt);
}

void LessEqualExp::toIR(IRstate *irs, unsigned ret)
{
    binIR(irs, ret, IRcle);
}

void GreaterExp::toIR(IRstate *irs, unsigned ret)
{
    binIR(irs, ret, IRcgt);
}

void GreaterEqualExp::toIR(IRstate *irs, unsigned ret)
{
    binIR(irs, ret, IRcge);
}

void EqualExp::toIR(IRstate *irs, unsigned ret)
{
    binIR(irs, ret, IRceq);
}

void NotEqualExp::toIR(IRstate *irs, unsigned ret)
{
    binIR(irs, ret, IRcne);
}

void IdentityExp::toIR(IRstate *irs, unsigned ret)
{
    binIR(irs, ret, IRcid);
}

void NonIdentityExp::toIR(IRstate *irs, unsigned ret)
{
    binIR(irs, ret, IRcnid);
}

void InstanceofExp::toIR(IRstate *irs, unsigned ret)
{
    binIR(irs, ret, IRinstance);
}

void AndAndExp::toIR(IRstate *irs, unsigned ret)
{   unsigned u;
    unsigned b;

    if (ret)
	b = ret;
    else
	b = irs->alloc(1);

    e1->toIR(irs, b);
    u = irs->getIP();
    irs->gen2(loc, IRjf, 0, b);
    e2->toIR(irs, ret);
    irs->patchJmp(u, irs->getIP());

    if (!ret)
	irs->release(b, 1);
}

void OrOrExp::toIR(IRstate *irs, unsigned ret)
{   unsigned u;
    unsigned b;

    if (ret)
	b = ret;
    else
	b = irs->alloc(1);

    e1->toIR(irs, b);
    u = irs->getIP();
    irs->gen2(loc, IRjt, 0, b);
    e2->toIR(irs, ret);
    irs->patchJmp(u, irs->getIP());

    if (!ret)
	irs->release(b, 1);
}

void CondExp::toIR(IRstate *irs, unsigned ret)
{   unsigned u1;
    unsigned u2;
    unsigned b;

    if (ret)
	b = ret;
    else
	b = irs->alloc(1);

    econd->toIR(irs, b);
    u1 = irs->getIP();
    irs->gen2(loc, IRjf, 0, b);
    e1->toIR(irs, ret);
    u2 = irs->getIP();
    irs->gen1(loc, IRjmp, 0);
    irs->patchJmp(u1, irs->getIP());
    e2->toIR(irs, ret);
    irs->patchJmp(u2, irs->getIP());

    if (!ret)
	irs->release(b, 1);
}


