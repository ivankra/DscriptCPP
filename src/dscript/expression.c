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

#include "root.h"
#include "dscript.h"
#include "expression.h"
#include "identifier.h"
#include "statement.h"
#include "text.h"
#include "scope.h"
#include "lexer.h"

/******************************** Expression **************************/

Expression::Expression(Loc loc, enum TOK op)
{
    this->loc = loc;
    this->op = op;
#if INVARIANT
    signature = EXPRESSION_SIGNATURE;
#endif
}

#if INVARIANT
void Expression::invariant()
{
    assert(signature == EXPRESSION_SIGNATURE);
    assert(op != TOKreserved && op < TOKMAX);
    mem.check(this);
}
#endif

/**************************
 * Semantically analyze Expression.
 * Determine types, fold constants, etc.
 */

Expression *Expression::semantic(Scope *sc)
{
    invariant();
    return this;
}

dchar *Expression::toDchars()
{   OutBuffer buf;
    dchar *p;

    toBuffer(&buf);
    buf.writedchar(0);
    p = (dchar *)buf.data;
    buf.data = NULL;
    return p;
}

void Expression::toBuffer(OutBuffer *buf)
{
    buf->writedstring(toDchars());
}

void Expression::checkLvalue(Scope *sc)
{
    OutBuffer buf;

    //printf("checkLvalue(), op = %d\n", op);
    if (sc->funcdef)
    {	if (sc->funcdef->isanonymous)
	    buf.writedstring("anonymous");
	else if (sc->funcdef->name)
	    buf.writedstring(sc->funcdef->name->toDchars());
    }
    buf.printf(L"(%d) : Error: ", loc);
    dchar *format = errmsg(ERR_CANNOT_ASSIGN_TO);
    buf.printf(format, toDchars());
    buf.writedchar(0);

    if (!sc->errinfo.message)
    {	sc->errinfo.message = (dchar *)buf.data;
	buf.data = NULL;
	sc->errinfo.linnum = loc;
	sc->errinfo.code = errcodtbl[ERR_CANNOT_ASSIGN_TO];
	sc->errinfo.srcline = Lexer::locToSrcline(sc->getSource(), loc);
    }
}

// Do we match for purposes of optimization?

int Expression::match(Expression *e)
{
    return FALSE;
}

// Is the result of the expression guaranteed to be a boolean?

int Expression::isBooleanResult()
{
    return FALSE;
}

/******************************** RealExpression **************************/

RealExpression::RealExpression(Loc loc, real_t value)
	: Expression(loc, TOKreal)
{
    this->value = value;
}

dchar *RealExpression::toDchars()
{
    static dchar buffer[sizeof(value) * 3 + 8 + 1];

#if defined UNICODE
    long i;
    i = (long) value;
    if (i == value)
	SWPRINTF(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%ld", i);
    else
	SWPRINTF(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%g", value);
#else
    sprintf(buffer, "%g", value);
#endif
    return buffer;
}

void RealExpression::toBuffer(OutBuffer *buf)
{
    buf->printf(DTEXT("%g"), value);
}


/******************************** IdentifierExpression **************************/

IdentifierExpression::IdentifierExpression(Loc loc, Identifier *ident)
	: Expression(loc, TOKidentifier)
{
    this->ident = ident;
}

Expression *IdentifierExpression::semantic(Scope *sc)
{
    invariant();
#if 0
    Symbol *s;

    s = sc->search(ident);
    if (s)
    {
    }
#endif
    return this;
}

dchar *IdentifierExpression::toDchars()
{
    return ident->toDchars();
}

void IdentifierExpression::checkLvalue(Scope *sc)
{
}

int IdentifierExpression::match(Expression *e)
{
    if (e->op != TOKidentifier)
	return 0;

    IdentifierExpression *ie = (IdentifierExpression *)(e);

    return Dchar::cmp(ident->toDchars(), ie->ident->toDchars()) == 0;
}

/******************************** ThisExpression **************************/

ThisExpression::ThisExpression(Loc loc)
	: Expression(loc, TOKthis)
{
}

dchar *ThisExpression::toDchars()
{
    return d_string_ptr(TEXT_this);
}

Expression *ThisExpression::semantic(Scope *sc)
{
    return this;
}

/******************************** NullExpression **************************/

NullExpression::NullExpression(Loc loc)
	: Expression(loc, TOKnull)
{
}

dchar *NullExpression::toDchars()
{
    return d_string_ptr(TEXT_null);
}

/******************************** StringExpression **************************/

StringExpression::StringExpression(Loc loc, Lstring *string)
	: Expression(loc, TOKstring)
{
    //WPRINTF(L"StringExpression('%s')\n", d_string_ptr(string));
    this->string = string;
}

void StringExpression::toBuffer(OutBuffer *buf)
{   dchar *s;
    unsigned len;

    buf->writedchar('"');
    len = string->len();
    for (s = string->toDchars(); ; s = Dchar::inc(s))
    {	unsigned c;

	c = Dchar::get(s);
	switch (c)
	{
	    case 0:
		if (s - string->toDchars() < (int)len)
		    goto Ldefault;
		break;

	    case '"':
		buf->writedchar('\\');
		goto Ldefault;

	    default:
	    Ldefault:
		if (c & ~0xFF)
		    buf->printf(DTEXT("\\u%04x"), c);
		else if (isprint(c))
		    buf->writedchar(c);
		else
		    buf->printf(DTEXT("\\x%02x"), c);
		continue;
	}
	break;
    }
    buf->writedchar('"');
}


/******************************** RegExpLiteral **************************/

RegExpLiteral::RegExpLiteral(Loc loc, Lstring *string)
	: Expression(loc, TOKregexp)
{
    //WPRINTF(L"RegExpLiteral('%ls')\n", string->toDchars());
    this->string = string;
}

void RegExpLiteral::toBuffer(OutBuffer *buf)
{
    buf->writedstring(string->toDchars());
}

/******************************** BooleanExpression **************************/

BooleanExpression::BooleanExpression(Loc loc, int boolean)
	: Expression(loc, TOKboolean)
{
    this->boolean = boolean;
}

dchar *BooleanExpression::toDchars()
{
    return (dchar*)(boolean ? DTEXT("true") : DTEXT("false"));
}

void BooleanExpression::toBuffer(OutBuffer *buf)
{
    buf->writedstring(toDchars());
}

int BooleanExpression::isBooleanResult()
{
    return TRUE;
}

/******************************** ArrayLiteral **************************/

ArrayLiteral::ArrayLiteral(Loc loc, Array *elements)
	: Expression(loc, TOKarraylit)
{
    this->elements = elements;
}

Expression *ArrayLiteral::semantic(Scope *sc)
{   unsigned i;
    Expression *e;

    if (elements)
    {
	for (i = 0; i < elements->dim; i++)
	{
	    e = (Expression *)elements->data[i];
	    if (e)
		e = e->semantic(sc);
	    elements->data[i] = e;
	}
    }
    return this;
}

void ArrayLiteral::toBuffer(OutBuffer *buf)
{   unsigned i;
    Expression *e;

    buf->writedchar('[');
    for (i = 0; i < elements->dim; i++)
    {
	if (i)
	    buf->writedchar(',');
	e = ((Expression *)elements->data[i]);
	if (e)
	    e->toBuffer(buf);
    }
    buf->writedchar(']');
}

/******************************** FieldLiteral **************************/

Field::Field(Identifier *ident, Expression *exp)
{
    this->ident = ident;
    this->exp = exp;
}

/******************************** ObjectLiteral **************************/

ObjectLiteral::ObjectLiteral(Loc loc, Array *fields)
	: Expression(loc, TOKobjectlit)
{
    this->fields = fields;
}

Expression *ObjectLiteral::semantic(Scope *sc)
{   unsigned i;

    for (i = 0; i < fields->dim; i++)
    {	Field *f;

	f = (Field *)fields->data[i];
	f->exp = f->exp->semantic(sc);
    }
    return this;
}

void ObjectLiteral::toBuffer(OutBuffer *buf)
{   unsigned i;

    buf->writedchar('{');
    for (i = 0; i < fields->dim; i++)
    {	Field *f;

	if (i)
	    buf->writedchar(',');
	f = (Field *)fields->data[i];
	buf->writedstring(f->ident->toDchars());
	buf->writedchar(':');
	f->exp->toBuffer(buf);
    }
    buf->writedchar('}');
}

/******************************** FunctionLiteral **************************/

FunctionLiteral::FunctionLiteral(Loc loc, FunctionDefinition *func)
	: Expression(loc, TOKobjectlit)
{
    this->func = func;
}

Expression *FunctionLiteral::semantic(Scope *sc)
{
    func = (FunctionDefinition *)func->semantic(sc);
    return this;
}

void FunctionLiteral::toBuffer(OutBuffer *buf)
{
    func->toBuffer(buf);
}

/***************************** UnaExp *************************************/

UnaExp::UnaExp(Loc loc, enum TOK op, Expression *e1)
	: Expression(loc, op)
{
    this->e1 = e1;
}

Expression *UnaExp::semantic(Scope *sc)
{
    e1 = e1->semantic(sc);
    return this;
}

void UnaExp::toBuffer(OutBuffer *buf)
{
    buf->writedstring(Token::toDchars(op));
    buf->writedchar(' ');
    e1->toBuffer(buf);
}

BinExp::BinExp(Loc loc, enum TOK op, Expression *e1, Expression *e2)
	: Expression(loc, op)
{
    this->e1 = e1;
    this->e2 = e2;
}

Expression *BinExp::semantic(Scope *sc)
{
    e1 = e1->semantic(sc);
    e2 = e2->semantic(sc);
    return this;
}

void BinExp::toBuffer(OutBuffer *buf)
{
    e1->toBuffer(buf);
    buf->writedchar(' ');
    buf->writedstring(Token::toDchars(op));
    buf->writedchar(' ');
    e2->toBuffer(buf);
}

/************************************************************/

PreIncExp::PreIncExp(Loc loc, Expression *e)
	: UnaExp(loc, TOKplusplus, e)
{
}

Expression *PreIncExp::semantic(Scope *sc)
{
    UnaExp::semantic(sc);
    e1->checkLvalue(sc);
    return this;
}

void PreIncExp::toBuffer(OutBuffer *buf)
{
    e1->toBuffer(buf);
    buf->writedstring(Token::toDchars(op));
}

PreDecExp::PreDecExp(Loc loc, Expression *e)
	: UnaExp(loc, TOKplusplus, e)
{
}

Expression *PreDecExp::semantic(Scope *sc)
{
    UnaExp::semantic(sc);
    e1->checkLvalue(sc);
    return this;
}

void PreDecExp::toBuffer(OutBuffer *buf)
{
    e1->toBuffer(buf);
    buf->writedstring(Token::toDchars(op));
}

/************************************************************/

PostIncExp::PostIncExp(Loc loc, Expression *e)
	: UnaExp(loc, TOKplusplus, e)
{
}

Expression *PostIncExp::semantic(Scope *sc)
{
    UnaExp::semantic(sc);
    e1->checkLvalue(sc);
    return this;
}

void PostIncExp::toBuffer(OutBuffer *buf)
{
    e1->toBuffer(buf);
    buf->writedstring(Token::toDchars(op));
}

PostDecExp::PostDecExp(Loc loc, Expression *e)
	: UnaExp(loc, TOKplusplus, e)
{
}

Expression *PostDecExp::semantic(Scope *sc)
{
    UnaExp::semantic(sc);
    e1->checkLvalue(sc);
    return this;
}

void PostDecExp::toBuffer(OutBuffer *buf)
{
    e1->toBuffer(buf);
    buf->writedstring(Token::toDchars(op));
}

/************************************************************/

DotExp::DotExp(Loc loc, Expression *e, Identifier *ident)
	: UnaExp(loc, TOKdot, e)
{
    this->ident = ident;
}

void DotExp::checkLvalue(Scope *sc)
{
}

void DotExp::toBuffer(OutBuffer *buf)
{
    e1->toBuffer(buf);
    buf->writedchar('.');
    buf->writedstring(ident->toDchars());
}

/************************************************************/

CallExp::CallExp(Loc loc, Expression *e, Array *arguments)
	: UnaExp(loc, TOKcall, e)
{
    //WPRINTF(L"CallExp(e1 = %x)\n", e);
    this->arguments = arguments;
}

Expression *CallExp::semantic(Scope *sc)
{   IdentifierExpression *ie;

//WPRINTF(L"CallExp(e1=%x, %d, vptr=%x)\n", e1, e1->op, *(unsigned *)e1);
//WPRINTF(L"CallExp(e1='%s')\n", e1->toDchars());
    e1 = e1->semantic(sc);
    if (e1->op != TOKcall)
	e1->checkLvalue(sc);
    if (arguments)
    {	unsigned a;
	Expression *e = NULL;	// initialization keeps lint happy

	for (a = 0; a < arguments->dim; a++)
	{
	    e = (Expression *)arguments->data[a];
	    e = e->semantic(sc);
	    arguments->data[a] = (void *)e;
	}
	if (a == 1)
	{
#if DYNAMIC_CAST
	    ie = dynamic_cast<IdentifierExpression *>(e1);
	    if (ie && Dchar::cmp(ie->ident->toDchars(), DTEXT("assert")) == 0)
	    {
		return new AssertExp(loc, e);
	    }
#else
	    if (e1->op == TOKidentifier)
	    {
		ie = (IdentifierExpression *)e1;
		if (Dchar::cmp(ie->ident->toDchars(), DTEXT("assert")) == 0)
		{
		    return new AssertExp(loc, e);
		}
	    }
#endif
	}
    }
    return this;
}

void CallExp::toBuffer(OutBuffer *buf)
{
    e1->toBuffer(buf);
    buf->writedchar('(');
    if (arguments)
    {	unsigned u;

	for (u = 0; u < arguments->dim; u++)
	{
	    if (u)
		buf->writedstring(", ");
	    ((Object *)arguments->data[u])->toBuffer(buf);
	}
    }
    buf->writedchar(')');
}

/************************************************************/

AssertExp::AssertExp(Loc loc, Expression *e)
	: UnaExp(loc, TOKassert, e)
{
}

void AssertExp::toBuffer(OutBuffer *buf)
{
    buf->writedstring("assert(");
    e1->toBuffer(buf);
    buf->writedchar(')');
}

/************************* NewExp ***********************************/

NewExp::NewExp(Loc loc, Expression *e, Array *arguments)
	: UnaExp(loc, TOKnew, e)
{
    this->arguments = arguments;
}

Expression *NewExp::semantic(Scope *sc)
{
    e1 = e1->semantic(sc);
    if (arguments)
    {	unsigned a;
	Expression *e;

	for (a = 0; a < arguments->dim; a++)
	{
	    e = (Expression *)arguments->data[a];
	    e = e->semantic(sc);
	    arguments->data[a] = (void *)e;
	}
    }
    return this;
}

void NewExp::toBuffer(OutBuffer *buf)
{
    buf->writedstring(Token::toDchars(op));
    buf->writedchar(' ');

    e1->toBuffer(buf);
    buf->writedchar('(');
    if (arguments)
    {	unsigned u;

	for (u = 0; u < arguments->dim; u++)
	{
	    if (u)
		buf->writedstring(", ");
	    ((Object *)arguments->data[u])->toBuffer(buf);
	}
    }
    buf->writedchar(')');
}

/************************************************************/

PosExp::PosExp(Loc loc, Expression *e)
	: UnaExp(loc, TOKpos, e)
{
}

NegExp::NegExp(Loc loc, Expression *e)
	: UnaExp(loc, TOKneg, e)
{
}

ComExp::ComExp(Loc loc, Expression *e)
	: UnaExp(loc, TOKtilde, e)
{
}

NotExp::NotExp(Loc loc, Expression *e)
	: UnaExp(loc, TOKnot, e)
{
}

int NotExp::isBooleanResult()
{
    return TRUE;
}

DeleteExp::DeleteExp(Loc loc, Expression *e)
	: UnaExp(loc, TOKdelete, e)
{
}

Expression *DeleteExp::semantic(Scope *sc)
{
    e1->checkLvalue(sc);
    return this;
}

TypeofExp::TypeofExp(Loc loc, Expression *e)
	: UnaExp(loc, TOKtypeof, e)
{
}

VoidExp::VoidExp(Loc loc, Expression *e)
	: UnaExp(loc, TOKvoid, e)
{
}

/************************* CommaExp ***********************************/

CommaExp::CommaExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKcomma, e1, e2)
{
}

void CommaExp::checkLvalue(Scope *sc)
{
    e2->checkLvalue(sc);
}

/************************* ArrayExp ***********************************/

ArrayExp::ArrayExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKarray, e1, e2)
{
}

Expression *ArrayExp::semantic(Scope *sc)
{
    checkLvalue(sc);
    return this;
}

void ArrayExp::checkLvalue(Scope *sc)
{
}

void ArrayExp::toBuffer(OutBuffer *buf)
{
    e1->toBuffer(buf);
    buf->writedchar('[');
    e2->toBuffer(buf);
    buf->writedchar(']');
}

/************************* BinAssignExp ***********************************/

BinAssignExp::BinAssignExp(Loc loc, enum TOK op, Expression *e1, Expression *e2)
	: BinExp(loc, op, e1, e2)
{
}

Expression *BinAssignExp::semantic(Scope *sc)
{
    BinExp::semantic(sc);
    e1->checkLvalue(sc);
    return this;
}

/************************* AssignExp ***********************************/

AssignExp::AssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinAssignExp(loc, TOKassign, e1, e2)
{
}

Expression *AssignExp::semantic(Scope *sc)
{
    //WPRINTF(L"AssignExp::semantic()\n");
    BinExp::semantic(sc);
    if (e1->op != TOKcall)		// special case for CallExp lvalue's
	e1->checkLvalue(sc);
    return this;
}

/************************* AddAssignExp ***********************************/

AddAssignExp::AddAssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinAssignExp(loc, TOKplusass, e1, e2)
{
}

MinAssignExp::MinAssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinAssignExp(loc, TOKminusass, e1, e2)
{
}

MulAssignExp::MulAssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinAssignExp(loc, TOKmultiplyass, e1, e2)
{
}

DivAssignExp::DivAssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinAssignExp(loc, TOKdivideass, e1, e2)
{
}

ModAssignExp::ModAssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinAssignExp(loc, TOKpercentass, e1, e2)
{
}

ShlAssignExp::ShlAssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinAssignExp(loc, TOKshiftleftass, e1, e2)
{
}

ShrAssignExp::ShrAssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinAssignExp(loc, TOKshiftrightass, e1, e2)
{
}

UshrAssignExp::UshrAssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinAssignExp(loc, TOKushiftrightass, e1, e2)
{
}

AndAssignExp::AndAssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinAssignExp(loc, TOKandass, e1, e2)
{
}

OrAssignExp::OrAssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinAssignExp(loc, TOKorass, e1, e2)
{
}

XorAssignExp::XorAssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinAssignExp(loc, TOKxorass, e1, e2)
{
}

/************************* AddExp *****************************/

AddExp::AddExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKplus, e1, e2)
{
}

Expression *AddExp::semantic(Scope *sc)
{
    return this;
}

/************************* MinExp ***********************************/

MinExp::MinExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKminus, e1, e2)
{
}

/************************* MulExp ***********************************/

MulExp::MulExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKmultiply, e1, e2)
{
}

/************************* DivExp ***********************************/

DivExp::DivExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKdivide, e1, e2)
{
}

/************************* ModExp ***********************************/

ModExp::ModExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKpercent, e1, e2)
{
}

/************************* ShlExp ***********************************/

ShlExp::ShlExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKshiftleft, e1, e2)
{
}

/************************* ShrExp ***********************************/

ShrExp::ShrExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKshiftright, e1, e2)
{
}

/************************* UshrExp ***********************************/

UshrExp::UshrExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKushiftright, e1, e2)
{
}

/************************* AndExp ***********************************/

AndExp::AndExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKand, e1, e2)
{
}

/************************* OrExp ***********************************/

OrExp::OrExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKor, e1, e2)
{
}

/************************* XorExp ***********************************/

XorExp::XorExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKxor, e1, e2)
{
}

/************************* OrOrExp ***********************************/

OrOrExp::OrOrExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKoror, e1, e2)
{
}

/************************* AndAndExp ***********************************/

AndAndExp::AndAndExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKandand, e1, e2)
{
}

/************************* LessExp ***********************************/

LessExp::LessExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKless, e1, e2)
{
}

int LessExp::isBooleanResult()
{
    return TRUE;
}

LessEqualExp::LessEqualExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKlessequal, e1, e2)
{
}

int LessEqualExp::isBooleanResult()
{
    return TRUE;
}

GreaterExp::GreaterExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKgreater, e1, e2)
{
}

int GreaterExp::isBooleanResult()
{
    return TRUE;
}

GreaterEqualExp::GreaterEqualExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKgreaterequal, e1, e2)
{
}

int GreaterEqualExp::isBooleanResult()
{
    return TRUE;
}

EqualExp::EqualExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKequal, e1, e2)
{
}

Expression *EqualExp::semantic(Scope *sc)
{
    BinExp::semantic(sc);
    return this;
}

int EqualExp::isBooleanResult()
{
    return TRUE;
}

NotEqualExp::NotEqualExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKnotequal, e1, e2)
{
}

int NotEqualExp::isBooleanResult()
{
    return TRUE;
}

IdentityExp::IdentityExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKidentity, e1, e2)
{
}

Expression *IdentityExp::semantic(Scope *sc)
{
    BinExp::semantic(sc);
    return this;
}

int IdentityExp::isBooleanResult()
{
    return TRUE;
}

NonIdentityExp::NonIdentityExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKnonidentity, e1, e2)
{
}

int NonIdentityExp::isBooleanResult()
{
    return TRUE;
}

InstanceofExp::InstanceofExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKinstanceof, e1, e2)
{
}

InExp::InExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKin, e1, e2)
{
}

/****************************************************************/

CondExp::CondExp(Loc loc, Expression *econd, Expression *e1, Expression *e2)
	: BinExp(loc, TOKquestion, e1, e2)
{
    this->econd = econd;
}

