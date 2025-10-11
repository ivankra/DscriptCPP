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
#include <assert.h>

#if _MSC_VER
#include <malloc.h>
#endif

#include "mem.h"
#include "root.h"
#include "expression.h"
#include "statement.h"
#include "identifier.h"
#include "symbol.h"
#include "scope.h"
#include "ir.h"
#include "value.h"
#include "property.h"
#include "dobject.h"
#include "program.h"
#include "text.h"

/* ========================== DdeclaredFunction ================== */

struct DdeclaredFunction : Dfunction
{
    FunctionDefinition *fd;

    DdeclaredFunction(FunctionDefinition *fd);
    void *Call(CALL_ARGS);
    void *Construct(CONSTRUCT_ARGS);
    d_string toString();
};

DdeclaredFunction::DdeclaredFunction(FunctionDefinition *fd)
    : Dfunction(fd->parameters.dim, Dfunction::getPrototype())
{
    assert(Dfunction::getPrototype());
    assert(internal_prototype);
    this->fd = fd;

    Dobject *o;

    // ECMA 3 13.2
    GC_LOG();
    o = new(this) Dobject(Dobject::getPrototype());	// step 9
    Put(TEXT_prototype, o, DontEnum);		// step 11
    o->Put(TEXT_constructor, this, DontEnum);	// step 10
}

void *DdeclaredFunction::Call(CALL_ARGS)
{
    // 1. Create activation object per ECMA 10.1.6
    // 2. Instantiate function variables as properties of
    //    activation object
    // 3. The 'this' value is the activation object

    Dobject *actobj;		// activation object
    Darguments *args;
    Value *locals;
    unsigned i;
    unsigned a;
    unsigned nparams;
    void *result;

    //PRINTF("DdeclaredFunction::Call()\n");
    //PRINTF("\tinstantiate(this = %p, fd = %p)\n", this, fd);

    // if it's an empty function, just return
    if (fd->code[0].op.opcode == IRret)
    {
	return NULL;
    }

    // Generate the activation object
    // ECMA v3 10.1.6
    GC_LOG();
    actobj = new(this) Dobject(NULL);

    // Instantiate the parameters
    {
	Value *v;

	a = 0;
	nparams = fd->parameters.dim;
	for (i = 0; i < nparams; i++)
	{   Identifier *p;

	    p = (Identifier *)fd->parameters.data[i];
	    v = (a < argc) ? &arglist[a++] : &vundefined;
	    actobj->Put(p->toLstring(), v, DontDelete);
	}
    }

    // Generate the Arguments Object
    // ECMA v3 10.1.8
    GC_LOG();
    args = new(this) Darguments(cc->caller, this, actobj, &fd->parameters, argc, arglist);

    actobj->Put(TEXT_arguments, args, DontDelete);

    // The following is not specified by ECMA, but seems to be supported
    // by jscript. The url www.grannymail.com has the following code
    // which looks broken to me but works in jscript:
    //
    //	    function MakeArray() {
    //	      this.length = MakeArray.arguments.length
    //	      for (var i = 0; i < this.length; i++)
    //		  this[i+1] = arguments[i]
    //	    }
    //	    var cardpic = new MakeArray("LL","AP","BA","MB","FH","AW","CW","CV","DZ");
    Put(TEXT_arguments, args, DontDelete);		// make grannymail bug work

//-----------
    unsigned scoperootsave = cc->scoperoot;

    /* If calling a nested function, need to increase scoperoot
     */
    if (fd->enclosingFunction == cc->callerf)
    {   assert(cc->scoperoot < cc->scope->dim);
	cc->scoperoot++;
    }
    else
    {
	DdeclaredFunction *df = (DdeclaredFunction *)cc->caller;
	if (df && !fd->isanonymous)
	{
	    if (0)
	    {
		PRINTF("current nestDepth = %d, calling %d, cc->scope->dim = %d\n",
			df->fd->nestDepth,
			fd->nestDepth,
			cc->scope->dim);
	    }
	    int diff = df->fd->nestDepth - fd->nestDepth;
	    if (diff > 0)
	    {   if (diff >= cc->scoperoot)
		    PRINTF("diff %d cc->scoperoot %d\n", diff, cc->scoperoot);
		else
		    cc->scoperoot -= diff;
		assert(cc->scoperoot >= 1);
	    }
	}
    }
//-----------

    Array *scopex = this->scopex;
    size_t dim = this->scopex_dim;

    if (dim == 0)
    {	// Create [[Scope]] for function object
	scopex = new(gc) Array;
	dim = cc->scoperoot;
	scopex->setDim(dim);
	for (unsigned u = 0; u < dim; u++)
	    scopex->data[u] = (void *)cc->scope->data[u];
	this->scopex = scopex;
	this->scopex_dim = dim;
    }

    Array *scope = new(gc) Array;
    scope->reserve(dim + fd->withdepth + 2);
    for (unsigned u = 0; u < dim; u++)
	scope->push(scopex->data[u]);
    scope->push(actobj);

#if 0
    Array scope;
    scope.reserve(cc->scoperoot + fd->withdepth + 2);
    for (unsigned u = 0; u < cc->scoperoot; u++)
	scope.push(cc->scope->data[u]);
    scope.push(actobj);
#endif

    fd->instantiate(scope, dim + 1, actobj, DontDelete);

    Array *scopesave = cc->scope;
    cc->scope = scope;
    Dobject *variablesave = cc->variable;
    cc->variable = actobj;
    Dobject *callersave = cc->caller;
    cc->caller = this;
    FunctionDefinition *callerfsave = cc->callerf;
    cc->callerf = fd;

    void *p1 = NULL;
    if (fd->nlocals >= 128 ||
	(locals = (Value *) alloca(fd->nlocals * sizeof(Value))) == NULL)
    {
	p1 = cc->malloc(fd->nlocals * sizeof(Value));
	locals = (Value *)p1;
    }

    result = IR::call(cc, othis, fd->code, ret, locals);

    mem.free(p1);

    cc->callerf = callerfsave;
    cc->caller = callersave;
    cc->variable = variablesave;
    cc->scope = scopesave;
    cc->scoperoot = scoperootsave;

    // Remove the arguments object
    //Value *v;
    //v=Get(TEXT_arguments);
    //WPRINTF(L"1v = %x, %s, v->object = %x\n", v, v->getType(), v->object);
    Put(TEXT_arguments, &vundefined, 0);
    //actobj->Put(TEXT_arguments, &vundefined, 0);

#if 0
    WPRINTF(L"args = %x, actobj = %x\n", args, actobj);
    v=Get(TEXT_arguments);
    WPRINTF(L"2v = %x, %s, v->object = %x\n", v, v->getType(), v->object);
    v->object = NULL;

    {
	unsigned *p = (unsigned *)0x40a49a80;
	unsigned i;
	for (i = 0; i < 16; i++)
	{
	    WPRINTF(L"p[%x] = %x\n", &p[i], p[i]);
	}
    }
#endif

    return result;
}

void *DdeclaredFunction::Construct(CONSTRUCT_ARGS)
{
    // ECMA 3 13.2.2
    Dobject *othis;
    Dobject *proto;
    Value *v;
    void *result;

    v = Get(TEXT_prototype);
    if (v->isPrimitive())
	proto = Dobject::getPrototype();
    else
	proto = v->toObject();
    GC_LOG();
    othis = new(cc) Dobject(proto);
    result = Call(cc, othis, ret, argc, arglist);
    if (!result)
    {
	if (ret->isPrimitive())
	    Vobject::putValue(ret, othis);
    }
    return result;
}

d_string DdeclaredFunction::toString()
{
    OutBuffer buf;
    d_string s;

    //PRINTF("DdeclaredFunction::toString()\n");
    fd->toBuffer(&buf);
    buf.writedchar(0);
    s = d_string_ctor((dchar *)buf.data);
    buf.data = NULL;
    return s;
}

/* ========================== FunctionDefinition ================== */


FunctionDefinition::FunctionDefinition(Array *topstatements)
	: TopStatement(0)
{
    st = FUNCTIONDEFINITION;
    this->isglobal = 1;
    this->isanonymous = 0;
    this->iseval = 0;
    this->name = NULL;
    this->topstatements = topstatements;
    labtab = NULL;
    enclosingFunction = NULL;
    nestDepth = 0;
    withdepth = 0;
    code = NULL;
    nlocals = 0;
}

FunctionDefinition::FunctionDefinition(Loc loc, int isglobal,
	Identifier *name, Array *parameters, Array *topstatements)
	: TopStatement(loc)
{
    //WPRINTF(L"FunctionDefinition('%ls')\n", name ? name->string : L"");
    st = FUNCTIONDEFINITION;
    this->isglobal = isglobal;
    this->isanonymous = 0;
    this->iseval = 0;
    this->name = name;
    if (parameters)
    {
	this->parameters = *parameters;
	memset(parameters, 0, sizeof(Array));
    }
    this->topstatements = topstatements;
    labtab = NULL;
    enclosingFunction = NULL;
    nestDepth = 0;
    withdepth = 0;
    code = NULL;
    nlocals = 0;
}

Statement *FunctionDefinition::semantic(Scope *sc)
{
    unsigned i;
    TopStatement *ts;
    FunctionDefinition *fd;
    Array *a;

    //PRINTF("FunctionDefinition::semantic(%p)\n", this);

    // Log all the FunctionDefinition's so we can rapidly
    // instantiate them at runtime
    fd = enclosingFunction = sc->funcdef;

    // But only push it if it is not already in the array
    a = &fd->functiondefinitions;
    for (i = 0; ; i++)
    {
	if (i == a->dim)			// not in the array
	{   a->push(this);
	    break;
	}
	if (a->data[i] == (void *)this)		// already in the array
	    break;
    }

    //PRINTF("isglobal = %d, isanonymous = %d\n", isglobal, isanonymous);
    if (!isglobal && !isanonymous)
    {	sc = sc->push(this);
	sc->nestDepth++;
    }
    nestDepth = sc->nestDepth;

    if (topstatements)
    {
	for (i = 0; i < topstatements->dim; i++)
	{
	    ts = (TopStatement *)topstatements->data[i];
	    //PRINTF("calling semantic routine %d which is %p\n",i, ts);
	    if (!ts->done)
	    {   ts = ts->semantic(sc);
		if (sc->errinfo.message)
		    break;

		if (iseval)
		{
		    // There's an implied "return" on the last statement
		    if ((i + 1) == topstatements->dim)
		    {
			ts = ts->ImpliedReturn();
		    }
		}
		topstatements->data[i] = ts;
		ts->done = 1;
	    }
	}

	// Make sure all the LabelSymbol's are defined
	if (labtab)
	{   Array *members = &labtab->members;

	    for (i = 0; i < members->dim; i++)
	    {   LabelSymbol *ls;

		ls = (LabelSymbol *)members->data[i];
		if (!ls->statement)
		    error(sc, ERR_UNDEFINED_LABEL,
			ls->toDchars(), toDchars());
	    }
	}
    }

    if (!isglobal && !isanonymous)
	sc->pop();
    return (Statement *)this;
}

void FunctionDefinition::toBuffer(OutBuffer *buf)
{   unsigned i;

    //PRINTF("FunctionDefinition::toBuffer()\n");
    if (!isglobal)
    {
	buf->writedstring("function ");
	if (isanonymous)
	    buf->writedstring("anonymous");
	else if (name)
	    buf->writedstring(name->toDchars());
	buf->writedchar('(');
	for (i = 0; i < parameters.dim; i++)
	{   Identifier *parameter;

	    if (i)
		buf->writedchar(',');
	    parameter = (Identifier *)parameters.data[i];
	    buf->writedstring(d_string_ptr(parameter->toLstring()));
	}
	buf->writedchar(')');
	buf->writenl();
	buf->writedchar('{');
	buf->writedchar(' ');
	buf->writenl();
    }
    if (topstatements)
    {
	for (i = 0; i < topstatements->dim; i++)
	{   TopStatement *ts;

	    ts = (TopStatement *)topstatements->data[i];
	    ts->toBuffer(buf);
	}
    }
    if (!isglobal)
    {
	buf->writedchar('}');
	buf->writenl();
    }
}

void FunctionDefinition::toIR(IRstate *ignore)
{
    IRstate irs;
    unsigned i;

    //WPRINTF(L"FunctionDefinition::toIR('%ls'), code = %p, done = %d\n", name ? name->string : L"", code, done);
    if (topstatements)
    {
	for (i = 0; i < topstatements->dim; i++)
	{   TopStatement *ts;
	    FunctionDefinition *fd;

	    ts = (TopStatement *)topstatements->data[i];
	    if (ts->st == FUNCTIONDEFINITION)
	    {
#if DYNAMIC_CAST
		assert(dynamic_cast<FunctionDefinition *>(ts));
#endif
		fd = (FunctionDefinition *)ts;
		if (fd->code)
		    continue;
	    }
	    ts->toIR(&irs);
	}

	// Don't need parse trees anymore, release to garbage collector
	topstatements->zero();
	topstatements = NULL;
	labtab = NULL;			// maybe delete it?
#ifdef DEBUG
//	mem.fullcollect();		// stress garbage collector
#endif
    }
    irs.gen0(0, IRret);
    irs.gen0(0, IRend);
    irs.doFixups();
    irs.optimize();
    //mem.free(code);			// eliminate previous version
    code = (IR *) irs.codebuf->data;
    irs.codebuf->data = NULL;
    nlocals = irs.nlocals;
}

void FunctionDefinition::instantiate(Array *scopex, size_t dim, Dobject *actobj, unsigned attributes)
{   unsigned i;

    // Instantiate all the Var's per 10.1.3
    for (i = 0; i < varnames.dim; i++)
    {
	Identifier *name;

	name = (Identifier *)varnames.data[i];
	// If name is already declared, don't override it
	actobj->Put(name->toLstring(), &vundefined, Instantiate | DontOverride | attributes);
    }

    // Instantiate the Function's per 10.1.3
    for (i = 0; i < functiondefinitions.dim; i++)
    {
	FunctionDefinition *fd;

	fd = (FunctionDefinition *)functiondefinitions.data[i];

	// Set [[Scope]] property per 13.2 step 7
	Dfunction *fobject = new DdeclaredFunction(fd);
	fobject->scopex = scopex;
	fobject->scopex_dim = dim;

	if (fd->name)       // skip anonymous functions
	    actobj->Put(fd->name->toLstring(), fobject, Instantiate | attributes);
    }
}
