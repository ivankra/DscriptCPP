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

#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "root.h"
#include "mem.h"

#include "dscript.h"
#include "lexer.h"

struct Scope;
struct FunctionDefinition;
struct IRstate;
struct IR;

struct Expression : Object
{
#if INVARIANT
#   define EXPRESSION_SIGNATURE	0x3AF31E3F
    unsigned signature;
    virtual void invariant();
#else
    void invariant() { }
#endif

    Loc loc;			// file location
    enum TOK op;

    Expression(Loc loc, enum TOK op);
    virtual Expression *semantic(Scope *sc);
    dchar *toDchars();
    virtual void toBuffer(OutBuffer *buf);
    virtual void checkLvalue(Scope *sc);
    virtual void toIR(IRstate *irs, unsigned ret);
    virtual void toLvalue(IRstate *irs, unsigned *base, IR *property, int *opoff);
    virtual int match(Expression *e);
    virtual int isBooleanResult();
};

struct RealExpression : Expression
{
    real_t value;

    RealExpression(Loc loc, real_t value);
    dchar *toDchars();
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs, unsigned ret);
};

struct IdentifierExpression : Expression
{
    Identifier *ident;

    IdentifierExpression(Loc loc, Identifier *ident);
    Expression *semantic(Scope *sc);
    dchar *toDchars();
    void checkLvalue(Scope *sc);
    void toIR(IRstate *irs, unsigned ret);
    void toLvalue(IRstate *irs, unsigned *base, IR *property, int *opoff);
    int match(Expression *e);
};

struct ThisExpression : Expression
{
    ThisExpression(Loc loc);
    dchar *toDchars();
    Expression *semantic(Scope *sc);
    void toIR(IRstate *irs, unsigned ret);
};

struct NullExpression : Expression
{
    NullExpression(Loc loc);
    dchar *toDchars();
    void toIR(IRstate *irs, unsigned ret);
};

struct StringExpression : Expression
{
    Lstring *string;

    StringExpression(Loc loc, Lstring *s);
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs, unsigned ret);
};

struct RegExpLiteral : Expression
{
    Lstring *string;

    RegExpLiteral(Loc loc, Lstring *s);
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs, unsigned ret);
};

struct BooleanExpression : Expression
{
    int boolean;

    BooleanExpression(Loc loc, int boolean);
    dchar *toDchars();
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs, unsigned ret);
    int isBooleanResult();
};

struct ArrayLiteral : Expression
{   Array *elements;	// array of Expression's

    ArrayLiteral(Loc loc, Array *elements);
    Expression *semantic(Scope *sc);
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs, unsigned ret);
};

struct Field : Mem		// pick up new/delete from mem
{
    Identifier *ident;
    Expression *exp;

    Field(Identifier *ident, Expression *exp);
};

struct ObjectLiteral : Expression
{   Array *fields;	// array of Field's

    ObjectLiteral(Loc loc, Array *fields);
    Expression *semantic(Scope *sc);
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs, unsigned ret);
};

struct FunctionLiteral : Expression
{   FunctionDefinition *func;

    FunctionLiteral(Loc loc, FunctionDefinition *func);
    Expression *semantic(Scope *sc);
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs, unsigned ret);
};

/****************************************************************/

struct UnaExp : Expression
{
    Expression *e1;

    UnaExp(Loc loc, enum TOK op, Expression *e1);
    Expression *semantic(Scope *sc);
    void toBuffer(OutBuffer *buf);
};

struct BinExp : Expression
{
    Expression *e1;
    Expression *e2;

    BinExp(Loc loc, enum TOK op, Expression *e1, Expression *e2);
    Expression *semantic(Scope *sc);
    void toBuffer(OutBuffer *buf);
    void binIR(IRstate *irs, unsigned ret, unsigned ircode);
    void binAssignIR(IRstate *irs, unsigned ret, unsigned ircode);
};

/****************************************************************/

struct PreIncExp : UnaExp
{
    PreIncExp(Loc loc, Expression *e);
    Expression *semantic(Scope *sc);
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs, unsigned ret);
};

struct PreDecExp : UnaExp
{
    PreDecExp(Loc loc, Expression *e);
    Expression *semantic(Scope *sc);
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs, unsigned ret);
};

struct PostIncExp : UnaExp
{
    PostIncExp(Loc loc, Expression *e);
    Expression *semantic(Scope *sc);
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs, unsigned ret);
};

struct PostDecExp : UnaExp
{
    PostDecExp(Loc loc, Expression *e);
    Expression *semantic(Scope *sc);
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs, unsigned ret);
};

struct DotExp : UnaExp
{
    Identifier *ident;

    DotExp(Loc loc, Expression *e, Identifier *ident);
    void checkLvalue(Scope *sc);
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs, unsigned ret);
    void toLvalue(IRstate *irs, unsigned *base, IR *property, int *opoff);
};

struct CallExp : UnaExp
{
    Array *arguments;		// Array of Expression's

    CallExp(Loc loc, Expression *e, Array *arguments);
    Expression *semantic(Scope *sc);
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs, unsigned ret);
};

struct AssertExp : UnaExp
{
    AssertExp(Loc loc, Expression *e);
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs, unsigned ret);
};

struct NewExp : UnaExp
{
    Array *arguments;		// Array of Expression's

    NewExp(Loc loc, Expression *e, Array *arguments);
    Expression *semantic(Scope *sc);
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs, unsigned ret);
};

struct NegExp : UnaExp
{
    NegExp(Loc loc, Expression *e);
    void toIR(IRstate *irs, unsigned ret);
};

struct PosExp : UnaExp
{
    PosExp(Loc loc, Expression *e);
    void toIR(IRstate *irs, unsigned ret);
};

struct ComExp : UnaExp
{
    ComExp(Loc loc, Expression *e);
    void toIR(IRstate *irs, unsigned ret);
};

struct NotExp : UnaExp
{
    NotExp(Loc loc, Expression *e);
    void toIR(IRstate *irs, unsigned ret);
    int isBooleanResult();
};

struct DeleteExp : UnaExp
{
    DeleteExp(Loc loc, Expression *e);
    Expression *semantic(Scope *sc);
    void toIR(IRstate *irs, unsigned ret);
};

struct TypeofExp : UnaExp
{
    TypeofExp(Loc loc, Expression *e);
    void toIR(IRstate *irs, unsigned ret);
};

struct VoidExp : UnaExp
{
    VoidExp(Loc loc, Expression *e);
    void toIR(IRstate *irs, unsigned ret);
};

/****************************************************************/

struct CommaExp : BinExp
{
    CommaExp(Loc loc, Expression *e1, Expression *e2);
    void checkLvalue(Scope *sc);
    void toIR(IRstate *irs, unsigned ret);
};

struct ArrayExp : BinExp
{
    ArrayExp(Loc loc, Expression *e1, Expression *e2);
    Expression *semantic(Scope *sc);
    void checkLvalue(Scope *sc);
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs, unsigned ret);
    void toLvalue(IRstate *irs, unsigned *base, IR *property, int *opoff);
};

struct BinAssignExp : BinExp
{
    BinAssignExp(Loc loc, enum TOK op, Expression *e1, Expression *e2);
    Expression *semantic(Scope *sc);
};

struct AssignExp : BinAssignExp
{
    AssignExp(Loc loc, Expression *e1, Expression *e2);
    Expression *semantic(Scope *sc);
    void toIR(IRstate *irs, unsigned ret);
};

struct AddAssignExp : BinAssignExp
{
    AddAssignExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct MinAssignExp : BinAssignExp
{
    MinAssignExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct MulAssignExp : BinAssignExp
{
    MulAssignExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct DivAssignExp : BinAssignExp
{
    DivAssignExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct ModAssignExp : BinAssignExp
{
    ModAssignExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct ShlAssignExp : BinAssignExp
{
    ShlAssignExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct ShrAssignExp : BinAssignExp
{
    ShrAssignExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct UshrAssignExp : BinAssignExp
{
    UshrAssignExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct AndAssignExp : BinAssignExp
{
    AndAssignExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct OrAssignExp : BinAssignExp
{
    OrAssignExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct XorAssignExp : BinAssignExp
{
    XorAssignExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct AddExp : BinExp
{
    AddExp(Loc loc, Expression *e1, Expression *e2);
    Expression *semantic(Scope *sc);
    void toIR(IRstate *irs, unsigned ret);
};

struct MinExp : BinExp
{
    MinExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct MulExp : BinExp
{
    MulExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct DivExp : BinExp
{
    DivExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct ModExp : BinExp
{
    ModExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct ShlExp : BinExp
{
    ShlExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct ShrExp : BinExp
{
    ShrExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct UshrExp : BinExp
{
    UshrExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct AndExp : BinExp
{
    AndExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct OrExp : BinExp
{
    OrExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct XorExp : BinExp
{
    XorExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct OrOrExp : BinExp
{
    OrOrExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct AndAndExp : BinExp
{
    AndAndExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct LessExp : BinExp
{
    LessExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
    int isBooleanResult();
};

struct LessEqualExp : BinExp
{
    LessEqualExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
    int isBooleanResult();
};

struct GreaterExp : BinExp
{
    GreaterExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
    int isBooleanResult();
};

struct GreaterEqualExp : BinExp
{
    GreaterEqualExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
    int isBooleanResult();
};

struct EqualExp : BinExp
{
    EqualExp(Loc loc, Expression *e1, Expression *e2);
    Expression *semantic(Scope *sc);
    void toIR(IRstate *irs, unsigned ret);
    int isBooleanResult();
};

struct NotEqualExp : BinExp
{
    NotEqualExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
    int isBooleanResult();
};

struct IdentityExp : BinExp
{
    IdentityExp(Loc loc, Expression *e1, Expression *e2);
    Expression *semantic(Scope *sc);
    void toIR(IRstate *irs, unsigned ret);
    int isBooleanResult();
};

struct NonIdentityExp : BinExp
{
    NonIdentityExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
    int isBooleanResult();
};

struct InstanceofExp : BinExp
{
    InstanceofExp(Loc loc, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

struct InExp : BinExp
{
    InExp(Loc loc, Expression *e1, Expression *e2);
};

/****************************************************************/

struct CondExp : BinExp
{
    Expression *econd;

    CondExp(Loc loc, Expression *econd, Expression *e1, Expression *e2);
    void toIR(IRstate *irs, unsigned ret);
};

#endif
