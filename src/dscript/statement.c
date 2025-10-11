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

#include "root.h"
#include "expression.h"
#include "statement.h"
#include "identifier.h"
#include "symbol.h"
#include "scope.h"
#include "ir.h"
#include "program.h"
#include "text.h"

/******************************** TopStatement ***************************/

TopStatement::TopStatement(Loc loc)
{
    this->loc = loc;
    this->done = 0;
    this->st = TOPSTATEMENT;
#if INVARIANT
    this->signature = TOPSTATEMENT_SIGNATURE;
#endif
}

#if INVARIANT

void TopStatement::invariant()
{
    assert(signature == TOPSTATEMENT_SIGNATURE);
    mem.check(this);
}

#endif

void TopStatement::toBuffer(OutBuffer *buf)
{
    buf->printf("TopStatement::toBuffer()");
    buf->writenl();
}

Statement *TopStatement::semantic(Scope *sc)
{
    PRINTF("TopStatement::semantic(%p)\n", this);
    return NULL;
}

void TopStatement::toIR(IRstate *irs)
{
    PRINTF("TopStatement::toIR(%p)\n", this);
}

void TopStatement::error(Scope *sc, int msgnum, ...)
{
    OutBuffer buf;

    if (sc->funcdef)
    {	if (sc->funcdef->isanonymous)
	    buf.writedstring("anonymous");
	else if (sc->funcdef->name)
	    buf.writedstring(sc->funcdef->name->toDchars());
    }
    buf.printf(L"(%d) : Error: ", loc);

    va_list ap;
    va_start(ap, msgnum);
    dchar *format = errmsg(msgnum);
    buf.vprintf(format, ap);
    va_end(ap);
    buf.writedchar(0);

    //vwprintf(format, ap);

    if (!sc->errinfo.message)
    {
	sc->errinfo.message = (dchar *)buf.data;
	sc->errinfo.linnum = loc;
	sc->errinfo.code = errcodtbl[msgnum];
	sc->errinfo.srcline = Lexer::locToSrcline(sc->getSource(), loc);
    }
    buf.data = NULL;
}

TopStatement *TopStatement::ImpliedReturn()
{
    return this;
}

/******************************** Statement ***************************/

Statement::Statement(Loc loc)
	: TopStatement(loc)
{
    this->loc = loc;
    label = NULL;
}

void Statement::toBuffer(OutBuffer *buf)
{
    buf->printf("Statement::toBuffer()");
    buf->writenl();
}

Statement *Statement::semantic(Scope *sc)
{
    PRINTF("Statement::semantic(%p)\n", this);
    return this;
}

void Statement::toIR(IRstate *irs)
{
    PRINTF("Statement::toIR(%p)\n", this);
}

unsigned Statement::getBreak()
{
    assert(0);
    return 0;
}

unsigned Statement::getContinue()
{
    assert(0);
    return 0;
}

unsigned Statement::getGoto()
{
    assert(0);
    return 0;
}

unsigned Statement::getTarget()
{
    assert(0);
    return 0;
}

ScopeStatement *Statement::getScope()
{
    return NULL;
}

/******************************** EmptyStatement ***************************/

EmptyStatement::EmptyStatement(Loc loc)
	: Statement(loc)
{
    this->loc = loc;
}

void EmptyStatement::toBuffer(OutBuffer *buf)
{
    buf->writedchar(';');
    buf->writenl();
}

Statement *EmptyStatement::semantic(Scope *sc)
{
    //PRINTF("EmptyStatement::semantic(%p)\n", this);
    return this;
}

void EmptyStatement::toIR(IRstate *irs)
{
}

/******************************** ExpStatement ***************************/

ExpStatement::ExpStatement(Loc loc, Expression *exp)
    : Statement(loc)
{
    //WPRINTF(L"ExpStatement::ExpStatement(this = %x, exp = %x)\n", this, exp);
    st = EXPSTATEMENT;
    this->exp = exp;
}

void ExpStatement::toBuffer(OutBuffer *buf)
{
    if (exp)
	exp->toBuffer(buf);
    buf->writedchar(';');
    buf->writenl();
}

Statement *ExpStatement::semantic(Scope *sc)
{
    //WPRINTF(L"exp = '%s'\n", exp->toDchars());
    //WPRINTF(L"ExpStatement::semantic(this = %x, exp = %x, exp->vptr = %x, %x, %x)\n", this, exp, ((unsigned *)exp)[0], /*(*(unsigned **)exp)[12],*/ *(unsigned *)(*(unsigned **)exp)[12]);
    invariant();
    if (exp)
    {	exp->invariant();
	exp = exp->semantic(sc);
    }
    //WPRINTF(L"-semantic()\n");
    return this;
}

TopStatement *ExpStatement::ImpliedReturn()
{
    return new ImpliedReturnStatement(loc, exp);
}

void ExpStatement::toIR(IRstate *irs)
{
    //PRINTF("ExpStatement::toIR(%p)\n", exp);
    if (exp)
    {	unsigned marksave = irs->mark();

	exp->invariant();
	exp->toIR(irs, 0);
	irs->release(marksave);

	exp = NULL;		// release to garbage collector
    }
}

/****************************** VarDeclaration ******************************/

VarDeclaration::VarDeclaration(Loc loc, Identifier *name, Expression *init)
{
    this->loc = loc;
    this->init = init;
    this->name = name;
}

/******************************** VarStatement ***************************/

VarStatement::VarStatement(Loc loc)
    : Statement(loc)
{
    this->st = VARSTATEMENT;
}

Statement *VarStatement::semantic(Scope *sc)
{   FunctionDefinition *fd;
    unsigned i;

    // Collect all the Var statements in order in the function
    // declaration, this is so it is easy to instantiate them
    fd = sc->funcdef;
    fd->varnames.reserve(vardecls.dim);

    for (i = 0; i < vardecls.dim; i++)
    {
	VarDeclaration *vd;

	vd = (VarDeclaration *)vardecls.data[i];
	if (vd->init)
	    vd->init = vd->init->semantic(sc);
	fd->varnames.push(vd->name);
    }

    return this;
}

void VarStatement::toBuffer(OutBuffer *buf)
{
    unsigned i;

    if (vardecls.dim)
    {
	buf->writedstring("var ");

	for (i = 0; i < vardecls.dim; i++)
	{
	    VarDeclaration *vd;

	    vd = (VarDeclaration *)vardecls.data[i];
	    buf->writedstring(vd->name->toDchars());
	    if (vd->init)
	    {	buf->writedstring(" = ");
		vd->init->toBuffer(buf);
	    }
	}
	buf->writedchar(';');
	buf->writenl();
    }
}

void VarStatement::toIR(IRstate *irs)
{
    unsigned i;
    unsigned ret;

    if (vardecls.dim)
    {	unsigned marksave;

	marksave = irs->mark();
	ret = irs->alloc(1);

	for (i = 0; i < vardecls.dim; i++)
	{
	    VarDeclaration *vd;

	    vd = (VarDeclaration *)vardecls.data[i];

	    // This works like assignment statements:
	    //	name = init;
	    IR property;

	    if (vd->init)
	    {
		vd->init->toIR(irs, ret);
		property.string = vd->name->toLstring();
		irs->gen2(loc, IRputthis, ret, property.index);
	    }
	}
	irs->release(marksave);
	vardecls.zero();		// help gc
    }
}

/******************************** BlockStatement ***************************/

BlockStatement::BlockStatement(Loc loc)
    : Statement(loc)
{
}

Statement *BlockStatement::semantic(Scope *sc)
{   unsigned i;

    for (i = 0; i < statements.dim; i++)
    {	Statement *s;

	s = (Statement *) statements.data[i];
	s->invariant();
	statements.data[i] = s->semantic(sc);
    }

    return this;
}

TopStatement *BlockStatement::ImpliedReturn()
{
    unsigned i = statements.dim;

    if (i)
    {
	TopStatement *ts = (TopStatement *)statements.data[i - 1];
	ts = ts->ImpliedReturn();
	statements.data[i - 1] = (void *)ts;
    }
    return this;
}


void BlockStatement::toBuffer(OutBuffer *buf)
{   unsigned i;

    buf->writedchar('{');
    buf->writenl();

    for (i = 0; i < statements.dim; i++)
    {	Statement *s;

	s = (Statement *) statements.data[i];
	s->toBuffer(buf);
    }

    buf->writedchar('}');
    buf->writenl();
}

void BlockStatement::toIR(IRstate *irs)
{   unsigned i;

    for (i = 0; i < statements.dim; i++)
    {	Statement *s;

	s = (Statement *) statements.data[i];
	s->invariant();
	s->toIR(irs);
    }

    // Release to garbage collector
    statements.zero();
    statements.dim = 0;
}


/******************************** LabelStatement ***************************/

LabelStatement::LabelStatement(Loc loc, Identifier *ident, Statement *statement)
    : Statement(loc)
{
    //PRINTF("LabelStatement::LabelStatement(%p, '%s', %p)\n", this, ident->toChars(), statement);
    this->ident = ident;
    this->statement = statement;
    gotoIP = ~0u;
    breakIP = ~0u;
    scopeContext = NULL;
}

Statement *LabelStatement::semantic(Scope *sc)
{
    LabelSymbol *ls;

    //WPRINTF(L"LabelStatement::semantic('%ls')\n", ident->toDchars());
    scopeContext = sc->scopeContext;
    ls = sc->searchLabel(ident);
    if (ls)
    {
	// Ignore multiple definition errors
	//if (ls->statement)
	    //error(sc, DTEXT("label '%s' is already defined"), ident->toDchars());
	ls->statement = this;
    }
    else
    {
	ls = new LabelSymbol(loc, ident, this);
	sc->insertLabel(ls);
    }
    if (statement)
	statement = statement->semantic(sc);
    return this;
}

TopStatement *LabelStatement::ImpliedReturn()
{
    if (statement)
	statement = (Statement *)statement->ImpliedReturn();
    return this;
}

void LabelStatement::toBuffer(OutBuffer *buf)
{
    buf->writedstring(ident->toDchars());
    buf->writedstring(": ");
    if (statement)
	statement->toBuffer(buf);
    else
	buf->writenl();
}

void LabelStatement::toIR(IRstate *irs)
{
    gotoIP = irs->getIP();
    statement->toIR(irs);
    breakIP = irs->getIP();
}

unsigned LabelStatement::getGoto()
{
    return gotoIP;
}

unsigned LabelStatement::getBreak()
{
    return breakIP;
}

unsigned LabelStatement::getContinue()
{
    return statement->getContinue();
}

ScopeStatement *LabelStatement::getScope()
{
    return scopeContext;
}

/******************************** IfStatement ***************************/

IfStatement::IfStatement(Loc loc, Expression *condition, Statement *ifbody, Statement *elsebody)
    : Statement(loc)
{
    this->condition = condition;
    this->ifbody = ifbody;
    this->elsebody = elsebody;
}

Statement *IfStatement::semantic(Scope *sc)
{
    //PRINTF("IfStatement::semantic(%p)\n", sc);
    assert(condition);
    condition = condition->semantic(sc);
    ifbody = ifbody->semantic(sc);
    if (elsebody)
	elsebody = elsebody->semantic(sc);

    return this;
}

TopStatement *IfStatement::ImpliedReturn()
{
    assert(condition);
    ifbody = (Statement *)ifbody->ImpliedReturn();
    if (elsebody)
	elsebody = (Statement *)elsebody->ImpliedReturn();
    return this;
}



void IfStatement::toIR(IRstate *irs)
{
    unsigned c;
    unsigned u1;
    unsigned u2;

    assert(condition);
    c = irs->alloc(1);
    condition->toIR(irs, c);
    u1 = irs->getIP();
    irs->gen2(loc, (condition->isBooleanResult() ? IRjfb : IRjf), 0, c);
    irs->release(c, 1);
    ifbody->toIR(irs);
    if (elsebody)
    {
	u2 = irs->getIP();
	irs->gen1(loc, IRjmp, 0);
	irs->patchJmp(u1, irs->getIP());
	elsebody->toIR(irs);
	irs->patchJmp(u2, irs->getIP());
    }
    else
    {
	irs->patchJmp(u1, irs->getIP());
    }

    // Help GC
    condition = NULL;
    ifbody = NULL;
    elsebody = NULL;
}

/******************************** SwitchStatement ***************************/

SwitchStatement::SwitchStatement(Loc loc, Expression *c, Statement *b)
    : Statement(loc)
{
    condition = c;
    body = b;
    breakIP = ~0u;
    scopeContext = NULL;

    swdefault = NULL;
    cases = NULL;
}

Statement *SwitchStatement::semantic(Scope *sc)
{
    condition = condition->semantic(sc);

    SwitchStatement *switchSave = sc->switchTarget;
    Statement *breakSave = sc->breakTarget;

    scopeContext = sc->scopeContext;
    sc->switchTarget = this;
    sc->breakTarget = this;

    body = body->semantic(sc);

    sc->switchTarget = switchSave;
    sc->breakTarget = breakSave;

    return this;
}

void SwitchStatement::toIR(IRstate *irs)
{   unsigned c;
    unsigned udefault;
    unsigned marksave;

    //PRINTF("SwitchStatement::toIR()\n");
    marksave = irs->mark();
    c = irs->alloc(1);
    condition->toIR(irs, c);

    // Generate a sequence of cmp-jt
    // Not the most efficient, but we await a more formal
    // specification of switch before we attempt to optimize

    if (cases && cases->dim)
    {	unsigned x;

	x = irs->alloc(1);
	for (unsigned i = 0; i < cases->dim; i++)
	{
	    unsigned x;
	    CaseStatement *cs;

	    x = irs->alloc(1);
	    cs = (CaseStatement *)cases->data[i];
	    cs->exp->toIR(irs, x);
	    irs->gen3(loc, IRcid, x, c, x);
	    cs->patchIP = irs->getIP();
	    irs->gen2(loc, IRjt, 0, x);
	}
    }
    udefault = irs->getIP();
    irs->gen1(loc, IRjmp, 0);

    Statement *breakSave = irs->breakTarget;
    irs->breakTarget = this;
    body->toIR(irs);

    irs->breakTarget = breakSave;
    breakIP = irs->getIP();

    // Patch jump addresses
    if (cases)
    {
	for (unsigned i = 0; i < cases->dim; i++)
	{   CaseStatement *cs;

	    cs = (CaseStatement *)cases->data[i];
	    irs->patchJmp(cs->patchIP, cs->caseIP);
	}
    }
    if (swdefault)
	irs->patchJmp(udefault, swdefault->defaultIP);
    else
	irs->patchJmp(udefault, breakIP);
    irs->release(marksave);

    // Help gc
    condition = NULL;
    body = NULL;
}

unsigned SwitchStatement::getBreak()
{
    return breakIP;
}

ScopeStatement *SwitchStatement::getScope()
{
    return scopeContext;
}

/******************************** CaseStatement ***************************/

CaseStatement::CaseStatement(Loc loc, Expression *exp)
    : Statement(loc)
{
    this->exp = exp;
    caseIP = ~0u;
    patchIP = ~0u;
}

Statement *CaseStatement::semantic(Scope *sc)
{
    //PRINTF("CaseStatement::semantic(%p)\n", sc);
    exp = exp->semantic(sc);
    if (sc->switchTarget)
    {
	SwitchStatement *sw = sc->switchTarget;
	unsigned i;

	if (!sw->cases)
	    sw->cases = new Array();

	// Look for duplicate
	for (i = 0; i < sw->cases->dim; i++)
	{
	    CaseStatement *cs = (CaseStatement *)sw->cases->data[i];

	    if (exp->equals(cs->exp))
	    {
		error(sc, ERR_SWITCH_REDUNDANT_CASE, exp->toDchars());
		return NULL;
	    }
	}
	sw->cases->push(this);
    }
    else
    {	error(sc, ERR_MISPLACED_SWITCH_CASE, exp->toDchars());
	return NULL;
    }
    return this;
}

void CaseStatement::toIR(IRstate *irs)
{
    caseIP = irs->getIP();
}

/******************************** DefaultStatement ***************************/

DefaultStatement::DefaultStatement(Loc loc)
    : Statement(loc)
{
    defaultIP = ~0u;
}

Statement *DefaultStatement::semantic(Scope *sc)
{
    if (sc->switchTarget)
    {
	SwitchStatement *sw = sc->switchTarget;

	if (sw->swdefault)
	{
	    error(sc, ERR_SWITCH_REDUNDANT_DEFAULT);
	    return NULL;
	}
	sw->swdefault = this;
    }
    else
    {	error(sc, ERR_MISPLACED_SWITCH_DEFAULT);
	return NULL;
    }
    return this;
}

void DefaultStatement::toIR(IRstate *irs)
{
    defaultIP = irs->getIP();
}

/******************************** DoStatement ***************************/

DoStatement::DoStatement(Loc loc, Statement *b, Expression *c)
    : Statement(loc)
{
    body = b;
    condition = c;
    breakIP = ~0u;
    continueIP = ~0u;
    scopeContext = NULL;
}

Statement *DoStatement::semantic(Scope *sc)
{
    Statement *continueSave = sc->continueTarget;
    Statement *breakSave = sc->breakTarget;

    scopeContext = sc->scopeContext;
    sc->continueTarget = this;
    sc->breakTarget = this;

    body = body->semantic(sc);
    condition = condition->semantic(sc);

    sc->continueTarget = continueSave;
    sc->breakTarget = breakSave;

    return this;
}

TopStatement *DoStatement::ImpliedReturn()
{
    if (body)
	body = (Statement *)body->ImpliedReturn();
    return this;
}

void DoStatement::toIR(IRstate *irs)
{   unsigned c;
    unsigned u1;
    Statement *continueSave = irs->continueTarget;
    Statement *breakSave = irs->breakTarget;
    unsigned marksave;

    irs->continueTarget = this;
    irs->breakTarget = this;

    marksave = irs->mark();
    u1 = irs->getIP();
    body->toIR(irs);
    c = irs->alloc(1);
    continueIP = irs->getIP();
    condition->toIR(irs, c);
    irs->gen2(loc, (condition->isBooleanResult() ? IRjtb : IRjt), u1 - irs->getIP(), c);
    breakIP = irs->getIP();
    irs->release(marksave);

    irs->continueTarget = continueSave;
    irs->breakTarget = breakSave;

    // Help GC
    condition = NULL;
    body = NULL;
}

unsigned DoStatement::getBreak()
{
    return breakIP;
}

unsigned DoStatement::getContinue()
{
    return continueIP;
}

ScopeStatement *DoStatement::getScope()
{
    return scopeContext;
}

/******************************** WhileStatement ***************************/

WhileStatement::WhileStatement(Loc loc, Expression *c, Statement *b)
    : Statement(loc)
{
    condition = c;
    body = b;
    breakIP = ~0u;
    continueIP = ~0u;
    scopeContext = NULL;
}

Statement *WhileStatement::semantic(Scope *sc)
{
    Statement *continueSave = sc->continueTarget;
    Statement *breakSave = sc->breakTarget;

    scopeContext = sc->scopeContext;
    sc->continueTarget = this;
    sc->breakTarget = this;

    condition = condition->semantic(sc);
    body = body->semantic(sc);

    sc->continueTarget = continueSave;
    sc->breakTarget = breakSave;

    return this;
}

TopStatement *WhileStatement::ImpliedReturn()
{
    if (body)
	body = (Statement *)body->ImpliedReturn();
    return this;
}

void WhileStatement::toIR(IRstate *irs)
{   unsigned c;
    unsigned u1;
    unsigned u2;

    Statement *continueSave = irs->continueTarget;
    Statement *breakSave = irs->breakTarget;
    unsigned marksave = irs->mark();

    irs->continueTarget = this;
    irs->breakTarget = this;

    u1 = irs->getIP();
    continueIP = u1;
    c = irs->alloc(1);
    condition->toIR(irs, c);
    u2 = irs->getIP();
    irs->gen2(loc, (condition->isBooleanResult() ? IRjfb : IRjf), 0, c);
    body->toIR(irs);
    irs->gen1(loc, IRjmp, u1 - irs->getIP());
    irs->patchJmp(u2, irs->getIP());
    breakIP = irs->getIP();

    irs->release(marksave);
    irs->continueTarget = continueSave;
    irs->breakTarget = breakSave;

    // Help GC
    condition = NULL;
    body = NULL;
}

unsigned WhileStatement::getBreak()
{
    return breakIP;
}

unsigned WhileStatement::getContinue()
{
    return continueIP;
}

ScopeStatement *WhileStatement::getScope()
{
    return scopeContext;
}

/******************************** ForStatement ***************************/

ForStatement::ForStatement(Loc loc, Statement *init, Expression *condition, Expression *increment, Statement *body)
    : Statement(loc)
{
    this->init = init;
    this->condition = condition;
    this->increment = increment;
    this->body = body;
    breakIP = ~0u;
    continueIP = ~0u;
    scopeContext = NULL;
}

Statement *ForStatement::semantic(Scope *sc)
{
    Statement *continueSave = sc->continueTarget;
    Statement *breakSave = sc->breakTarget;

    if (init)
	init = init->semantic(sc);
    if (condition)
	condition = condition->semantic(sc);
    if (increment)
	increment = increment->semantic(sc);

    scopeContext = sc->scopeContext;
    sc->continueTarget = this;
    sc->breakTarget = this;

    body = body->semantic(sc);

    sc->continueTarget = continueSave;
    sc->breakTarget = breakSave;

    return this;
}

TopStatement *ForStatement::ImpliedReturn()
{
    if (body)
	body = (Statement *)body->ImpliedReturn();
    return this;
}

void ForStatement::toIR(IRstate *irs)
{
    unsigned u1;
    unsigned u2 = 0;	// unneeded initialization keeps lint happy

    Statement *continueSave = irs->continueTarget;
    Statement *breakSave = irs->breakTarget;
    unsigned marksave = irs->mark();

    irs->continueTarget = this;
    irs->breakTarget = this;

    if (init)
	init->toIR(irs);
    u1 = irs->getIP();
    if (condition)
    {
	if (condition->op == TOKless || condition->op == TOKlessequal)
	{
	    BinExp *be = (BinExp *)condition;
	    RealExpression *re;
	    unsigned b;
	    unsigned c;

	    b = irs->alloc(1);
	    be->e1->toIR(irs, b);
	    re = (RealExpression *)be->e2;
	    if (be->e2->op == TOKreal && !Port::isnan(re->value))
	    {
		u2 = irs->getIP();
		irs->gen(loc, (condition->op == TOKless) ? IRjltc : IRjlec, 4, 0, b, re->value);
	    }
	    else
	    {
		c = irs->alloc(1);
		be->e2->toIR(irs, c);
		u2 = irs->getIP();
		irs->gen3(loc, (condition->op == TOKless) ? IRjlt : IRjle, 0, b, c);
	    }
	}
	else
	{   unsigned c;

	    c = irs->alloc(1);
	    condition->toIR(irs, c);
	    u2 = irs->getIP();
	    irs->gen2(loc, (condition->isBooleanResult() ? IRjfb : IRjf), 0, c);
	}
    }
    body->toIR(irs);
    continueIP = irs->getIP();
    if (increment)
	increment->toIR(irs, 0);
    irs->gen1(loc, IRjmp, u1 - irs->getIP());
    if (condition)
	irs->patchJmp(u2, irs->getIP());

    breakIP = irs->getIP();

    irs->release(marksave);
    irs->continueTarget = continueSave;
    irs->breakTarget = breakSave;

    // Help GC
    init = NULL;
    condition = NULL;
    body = NULL;
    increment = NULL;
}

unsigned ForStatement::getBreak()
{
    return breakIP;
}

unsigned ForStatement::getContinue()
{
    return continueIP;
}

ScopeStatement *ForStatement::getScope()
{
    return scopeContext;
}

/******************************** ForInStatement ***************************/

ForInStatement::ForInStatement(Loc loc, Statement *init, Expression *in, Statement *body)
    : Statement(loc)
{
    this->init = init;
    this->in = in;
    this->body = body;
    breakIP = ~0u;
    continueIP = ~0u;
    scopeContext = NULL;
}

Statement *ForInStatement::semantic(Scope *sc)
{
    Statement *continueSave = sc->continueTarget;
    Statement *breakSave = sc->breakTarget;

    init = init->semantic(sc);

    if (init->st == EXPSTATEMENT)
    {	ExpStatement *es;

	es = (ExpStatement *)(init);
	es->exp->checkLvalue(sc);
    }
    else if (init->st == VARSTATEMENT)
    {
    }
    else
    {
	error(sc, ERR_INIT_NOT_EXPRESSION);
	return NULL;
    }

    in = in->semantic(sc);

    scopeContext = sc->scopeContext;
    sc->continueTarget = this;
    sc->breakTarget = this;

    body = body->semantic(sc);

    sc->continueTarget = continueSave;
    sc->breakTarget = breakSave;

    return this;
}

TopStatement *ForInStatement::ImpliedReturn()
{
    body = (Statement *)body->ImpliedReturn();
    return this;
}

void ForInStatement::toIR(IRstate *irs)
{
    unsigned e;
    unsigned iter;
    ExpStatement *es;
    VarStatement *vs;
    unsigned base;
    IR property;
    int opoff;
    unsigned marksave = irs->mark();

    e = irs->alloc(1);
    in->toIR(irs, e);
    iter = irs->alloc(1);
    irs->gen2(loc, IRiter, iter, e);

    Statement *continueSave = irs->continueTarget;
    Statement *breakSave = irs->breakTarget;

    irs->continueTarget = this;
    irs->breakTarget = this;

    if (init->st == EXPSTATEMENT)
    {
	es = (ExpStatement *)(init);
	es->exp->toLvalue(irs, &base, &property, &opoff);
    }
    else if (init->st == VARSTATEMENT)
    {
	VarDeclaration *vd;

	vs = (VarStatement *)(init);
	assert(vs->vardecls.dim == 1);
	vd = (VarDeclaration *)vs->vardecls.data[0];

	property.string = vd->name->toLstring();
	opoff = 2;
	base = ~0u;
    }
    else
    {	// Error already reported by semantic()
	return;
    }

    continueIP = irs->getIP();
    if (opoff == 2)
	irs->gen3(loc, IRnextscope, 0, property.index, iter);
    else
	irs->gen(loc, IRnext + opoff, 4, 0, base, property.index, iter);
    body->toIR(irs);
    irs->gen1(loc, IRjmp, continueIP - irs->getIP());
    irs->patchJmp(continueIP, irs->getIP());

    breakIP = irs->getIP();

    irs->continueTarget = continueSave;
    irs->breakTarget = breakSave;
    irs->release(marksave);

    // Help GC
    init = NULL;
    in = NULL;
    body = NULL;
}

unsigned ForInStatement::getBreak()
{
    return breakIP;
}

unsigned ForInStatement::getContinue()
{
    return continueIP;
}

ScopeStatement *ForInStatement::getScope()
{
    return scopeContext;
}

/******************************** ScopeStatement ***************************/

ScopeStatement::ScopeStatement(Loc loc)
    : Statement(loc)
{
    enclosingScope = NULL;
    depth = 1;
    npops = 1;
}

/******************************** WithStatement ***************************/

WithStatement::WithStatement(Loc loc, Expression *exp, Statement *body)
    : ScopeStatement(loc)
{
    this->exp = exp;
    this->body = body;
}

Statement *WithStatement::semantic(Scope *sc)
{
    exp = exp->semantic(sc);

    enclosingScope = sc->scopeContext;
    sc->scopeContext = this;

    // So enclosing FunctionDeclaration knows how deep the With's
    // can nest
    if (enclosingScope)
	depth = enclosingScope->depth + 1;
    if (depth > sc->funcdef->withdepth)
	sc->funcdef->withdepth = depth;

    sc->nestDepth++;
    body = body->semantic(sc);
    sc->nestDepth--;

    sc->scopeContext = enclosingScope;
    return this;
}

TopStatement *WithStatement::ImpliedReturn()
{
    body = (Statement *)body->ImpliedReturn();
    return this;
}



void WithStatement::toIR(IRstate *irs)
{
    unsigned c;
    unsigned marksave = irs->mark();

    irs->scopeContext = this;

    c = irs->alloc(1);
    exp->toIR(irs, c);
    irs->gen1(loc, IRpush, c);
    body->toIR(irs);
    irs->gen0(loc, IRpop);

    irs->scopeContext = enclosingScope;
    irs->release(marksave);

    // Help GC
    exp = NULL;
    body = NULL;
}

/******************************** ContinueStatement ***************************/

ContinueStatement::ContinueStatement(Loc loc, Identifier *ident)
    : Statement(loc)
{
    this->ident = ident;
    target = NULL;
}

Statement *ContinueStatement::semantic(Scope *sc)
{
    if (ident == NULL)
    {
	target = sc->continueTarget;
	if (!target)
	{
	    sc->errinfo.code = 1020;
	    error(sc, ERR_MISPLACED_CONTINUE);
	    return NULL;
	}
    }
    else
    {	LabelSymbol *ls;

	ls = sc->searchLabel(ident);
	if (!ls || !ls->statement)
	{   error(sc, ERR_UNDEFINED_STATEMENT_LABEL, ident->toDchars());
	    return NULL;
	}
	else
	    target = ls->statement;
    }
    return this;
}

void ContinueStatement::toIR(IRstate *irs)
{   ScopeStatement *w;
    ScopeStatement *tw;

    tw = target->getScope();
    for (w = irs->scopeContext; w != tw; w = w->enclosingScope)
    {
	assert(w);
	irs->pops(w->npops);
    }
    irs->addFixup(irs->getIP());
    irs->gen1(loc, IRjmp, (unsigned)this);
}

unsigned ContinueStatement::getTarget()
{
    assert(target);
    return target->getContinue();
}

/******************************** BreakStatement ***************************/

BreakStatement::BreakStatement(Loc loc, Identifier *ident)
    : Statement(loc)
{
    this->ident = ident;
    target = NULL;
}

Statement *BreakStatement::semantic(Scope *sc)
{
    //PRINTF("BreakStatement::semantic(%p)\n", sc);
    if (ident == NULL)
    {
	target = sc->breakTarget;
	if (!target)
	{
	    sc->errinfo.code = 1019;
	    error(sc, ERR_MISPLACED_BREAK);
	    return NULL;
	}
    }
    else
    {	LabelSymbol *ls;

	ls = sc->searchLabel(ident);
	if (!ls || !ls->statement)
	{   error(sc, ERR_UNDEFINED_STATEMENT_LABEL, ident->toDchars());
	    return NULL;
	}
	else
	    target = ls->statement;
    }
    return this;
}

void BreakStatement::toIR(IRstate *irs)
{   ScopeStatement *w;
    ScopeStatement *tw;

    assert(target);
    tw = target->getScope();
    for (w = irs->scopeContext; w != tw; w = w->enclosingScope)
    {
	assert(w);
	irs->pops(w->npops);
    }

    irs->addFixup(irs->getIP());
    irs->gen1(loc, IRjmp, (unsigned)this);
}

unsigned BreakStatement::getTarget()
{
    assert(target);
    return target->getBreak();
}

/******************************** GotoStatement ***************************/

GotoStatement::GotoStatement(Loc loc, Identifier *ident)
    : Statement(loc)
{
    this->ident = ident;
    label = NULL;
}

Statement *GotoStatement::semantic(Scope *sc)
{
    LabelSymbol *ls;

    ls = sc->searchLabel(ident);
    if (!ls)
    {
	ls = new LabelSymbol(loc, ident, NULL);
	sc->insertLabel(ls);
    }
    label = ls;
    return this;
}

void GotoStatement::toIR(IRstate *irs)
{
    assert(label);

    // Determine how many with pops we need to do
    for (ScopeStatement *w = irs->scopeContext; ; w = w->enclosingScope)
    {
	if (!w)
	{
	    if (label->statement->scopeContext)
	    {
		::error(errmsg(ERR_GOTO_INTO_WITH));
	    }
	    break;
	}
	if (w == label->statement->scopeContext)
	    break;
	irs->pops(w->npops);
    }

    irs->addFixup(irs->getIP());
    irs->gen1(loc, IRjmp, (unsigned)this);
}

unsigned GotoStatement::getTarget()
{
    return label->statement->getGoto();
}

/******************************** ReturnStatement ***************************/

ReturnStatement::ReturnStatement(Loc loc, Expression *exp)
    : Statement(loc)
{
    this->exp = exp;
}

Statement *ReturnStatement::semantic(Scope *sc)
{
    if (exp)
	exp = exp->semantic(sc);

    // Don't allow return from eval functions or global function
    if (sc->funcdef->iseval || sc->funcdef->isglobal)
	error(sc, ERR_MISPLACED_RETURN);

    return this;
}

void ReturnStatement::toBuffer(OutBuffer *buf)
{
    //PRINTF("ReturnStatement::toBuffer()\n");
    buf->printf(DTEXT("return "));
    if (exp)
	exp->toBuffer(buf);
    buf->writedchar(';');
    buf->writenl();
}

void ReturnStatement::toIR(IRstate *irs)
{   ScopeStatement *w;
    int npops;

    npops = 0;
    for (w = irs->scopeContext; w; w = w->enclosingScope)
	npops += w->npops;

    if (exp)
    {	unsigned e;

	e = irs->alloc(1);
	exp->toIR(irs, e);
	if (npops)
	{
	    irs->gen1(loc, IRimpret, e);
	    irs->pops(npops);
	    irs->gen0(loc, IRret);
	}
	else
	    irs->gen1(loc, IRretexp, e);
	irs->release(e, 1);
    }
    else
    {
	if (npops)
	    irs->pops(npops);
	irs->gen0(loc, IRret);
    }

    // Help GC
    exp = NULL;
}

/******************************** ImpliedReturnStatement ***************************/

// Same as ReturnStatement, except that the return value is set but the
// function does not actually return. Useful for setting the return
// value for loop bodies.

ImpliedReturnStatement::ImpliedReturnStatement(Loc loc, Expression *exp)
    : Statement(loc)
{
    this->exp = exp;
}

Statement *ImpliedReturnStatement::semantic(Scope *sc)
{
    if (exp)
	exp = exp->semantic(sc);
    return this;
}

void ImpliedReturnStatement::toBuffer(OutBuffer *buf)
{
    if (exp)
	exp->toBuffer(buf);
    buf->writedchar(';');
    buf->writenl();
}

void ImpliedReturnStatement::toIR(IRstate *irs)
{
    if (exp)
    {	unsigned e;

	e = irs->alloc(1);
	exp->toIR(irs, e);
	irs->gen1(loc, IRimpret, e);
	irs->release(e, 1);

	// Help GC
	exp = NULL;
    }
}

/******************************** ThrowStatement ***************************/

ThrowStatement::ThrowStatement(Loc loc, Expression *exp)
    : Statement(loc)
{
    this->exp = exp;
}

Statement *ThrowStatement::semantic(Scope *sc)
{
    if (exp)
	exp = exp->semantic(sc);
    else
    {
	error(sc, ERR_NO_THROW_EXPRESSION);
	return new EmptyStatement(loc);
    }
    return this;
}

void ThrowStatement::toBuffer(OutBuffer *buf)
{
    buf->writedstring("throw ");
    if (exp)
	exp->toBuffer(buf);
    buf->writedchar(';');
    buf->writenl();
}

void ThrowStatement::toIR(IRstate *irs)
{
    unsigned e;

    assert(exp);
    e = irs->alloc(1);
    exp->toIR(irs, e);
    irs->gen1(loc, IRthrow, e);
    irs->release(e, 1);

    // Help GC
    exp = NULL;
}

/******************************** TryStatement ***************************/

TryStatement::TryStatement(Loc loc, Statement *body,
	Identifier *catchident, Statement *catchbody,
	Statement *finalbody)
    : ScopeStatement(loc)
{
    this->body = body;
    this->catchident = catchident;
    this->catchbody = catchbody;
    this->finalbody = finalbody;
    if (catchbody && finalbody)
	npops = 2;		// 2 items in scope chain
}

Statement *TryStatement::semantic(Scope *sc)
{
    enclosingScope = sc->scopeContext;
    sc->scopeContext = this;

    // So enclosing FunctionDeclaration knows how deep the With's
    // can nest
    if (enclosingScope)
	depth = enclosingScope->depth + 1;
    if (depth > sc->funcdef->withdepth)
	sc->funcdef->withdepth = depth;

    body->semantic(sc);
    if (catchbody)
	catchbody->semantic(sc);
    if (finalbody)
	finalbody->semantic(sc);

    sc->scopeContext = enclosingScope;
    return this;
}

void TryStatement::toBuffer(OutBuffer *buf)
{
    buf->writedstring("try");
    buf->writenl();
    body->toBuffer(buf);
    if (catchident)
    {
	buf->writedstring("catch (");
	catchident->toBuffer(buf);
	buf->writedchar(')');
	buf->writenl();
    }
    if (catchbody)
	catchbody->toBuffer(buf);
    if (finalbody)
    {
	buf->writedstring("finally");
	buf->writenl();
	finalbody->toBuffer(buf);
    }
}

void TryStatement::toIR(IRstate *irs)
{   unsigned f;
    unsigned c;
    unsigned e;
    unsigned e2;
    unsigned marksave = irs->mark();

    irs->scopeContext = this;
    if (finalbody)
    {
	f = irs->getIP();
	irs->gen1(loc, IRtryfinally, 0);
	if (catchbody)
	{
	    c = irs->getIP();
	    irs->gen2(loc, IRtrycatch, 0, (unsigned)catchident->toLstring());
	    body->toIR(irs);
	    irs->gen0(loc, IRpop);		// remove catch clause
	    irs->gen0(loc, IRpop);		// call finalbody

	    e = irs->getIP();
	    irs->gen1(loc, IRjmp, 0);
	    irs->patchJmp(c, irs->getIP());
	    catchbody->toIR(irs);
	    irs->gen0(loc, IRpop);		// remove catch object
	    irs->gen0(loc, IRpop);		// call finalbody code
	    e2 = irs->getIP();
	    irs->gen1(loc, IRjmp, 0);	// jmp past finalbody

	    irs->patchJmp(f, irs->getIP());
	    irs->scopeContext = enclosingScope;
	    finalbody->toIR(irs);
	    irs->gen0(loc, IRfinallyret);
	    irs->patchJmp(e, irs->getIP());
	    irs->patchJmp(e2, irs->getIP());
	}
	else // finalbody only
	{
	    body->toIR(irs);
	    irs->gen0(loc, IRpop);
	    e = irs->getIP();
	    irs->gen1(loc, IRjmp, 0);
	    irs->patchJmp(f, irs->getIP());
	    irs->scopeContext = enclosingScope;
	    finalbody->toIR(irs);
	    irs->gen0(loc, IRfinallyret);
	    irs->patchJmp(e, irs->getIP());
	}
    }
    else // catchbody only
    {
	c = irs->getIP();
	irs->gen2(loc, IRtrycatch, 0, (unsigned)catchident->toLstring());
	body->toIR(irs);
	irs->gen0(loc, IRpop);
	e = irs->getIP();
	irs->gen1(loc, IRjmp, 0);
	irs->patchJmp(c, irs->getIP());
	catchbody->toIR(irs);
	irs->gen0(loc, IRpop);
	irs->patchJmp(e, irs->getIP());
    }
    irs->scopeContext = enclosingScope;
    irs->release(marksave);

    // Help GC
    body = NULL;
    catchident = NULL;
    catchbody = NULL;
    finalbody = NULL;
}
