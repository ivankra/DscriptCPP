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


#ifndef STATEMENT_H
#define STATEMENT_H


#include "root.h"
#include "mem.h"

struct OutBuffer;
struct Scope;
struct Expression;
struct Statement;
struct LabelSymbol;
struct ScopeStatement;
struct IRstate;
struct Identifier;
struct SymbolTable;
struct IR;
struct Dfunction;
struct Dobject;

// This is necessary because GNU C++ doesn't implement dynamic_cast<> in a way
// that works (crashes at runtime).
enum StatementType
{
    TOPSTATEMENT,
    FUNCTIONDEFINITION,
    EXPSTATEMENT,
    VARSTATEMENT,
};

struct TopStatement : Object
{
#if INVARIANT
#   define TOPSTATEMENT_SIGNATURE	0xBA3FE1F3
    unsigned signature;
    virtual void invariant();
#else
    void invariant() { }
#endif

    Loc loc;
    int done;		// 0: parsed
			// 1: semantic
			// 2: toIR
    enum StatementType st;

    TopStatement(Loc loc);
    virtual void toBuffer(OutBuffer *buf);
    virtual Statement *semantic(Scope *sc);
    virtual void toIR(IRstate *irs);
    void error(Scope *sc, int msgnum, ...);
    virtual TopStatement *ImpliedReturn();	// add return V; onto last statement
};

struct FunctionDefinition : TopStatement
{
    // Maybe the following two should be done with derived classes instead
    int isglobal;		// !=0 if the global anonymous function
    int isanonymous;		// !=0 if anonymous function
    int iseval;			// !=0 if eval function

    Identifier *name;		// NULL for anonymous function
    Array parameters;		// array of Identifier's
    Array *topstatements;	// array of TopStatement's

    Array varnames;		// array of Identifier's
    Array functiondefinitions;
    FunctionDefinition *enclosingFunction;
    int nestDepth;
    int withdepth;		// max nesting of ScopeStatement's

    SymbolTable *labtab;	// symbol table for LabelSymbol's

    IR *code;
    unsigned nlocals;

    FunctionDefinition(Loc loc, int isglobal,
	Identifier *name, Array *parameters, Array *topstatements);
    FunctionDefinition(Array *topstatements);
    Statement *semantic(Scope *sc);
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs);
    void instantiate(Array *scopex, size_t dim, Dobject *actobj, unsigned attributes);
};

struct Statement : TopStatement
{
    LabelSymbol *label;

    Statement(Loc loc);
    Statement *semantic(Scope *sc);
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs);

    virtual unsigned getBreak();
    virtual unsigned getContinue();
    virtual unsigned getGoto();
    virtual unsigned getTarget();
    virtual ScopeStatement *getScope();
};

struct EmptyStatement : Statement
{
    EmptyStatement(Loc loc);
    Statement *semantic(Scope *sc);
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs);
};

struct ExpStatement : Statement
{
    Expression *exp;

    ExpStatement(Loc loc, Expression *exp);
    Statement *semantic(Scope *sc);
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs);
    TopStatement *ImpliedReturn();
};

struct VarDeclaration : Mem		// inherit from Mem to pick up new/delete
{
    Loc loc;
    Identifier *name;
    Expression *init;

    VarDeclaration(Loc loc, Identifier *name, Expression *init);
};

struct VarStatement : Statement
{   Array vardecls;		// array of VarDeclaration's

    VarStatement(Loc loc);
    Statement *semantic(Scope *sc);
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs);
};

struct BlockStatement : Statement
{
    Array statements;

    BlockStatement(Loc loc);
    Statement *semantic(Scope *sc);
    TopStatement *ImpliedReturn();
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs);
};

struct ScopeStatement;

struct LabelStatement : Statement
{
    Identifier *ident;
    Statement *statement;
    unsigned gotoIP;
    unsigned breakIP;
    ScopeStatement *scopeContext;

    LabelStatement(Loc loc, Identifier *ident, Statement *statement);
    Statement *semantic(Scope *sc);
    TopStatement *ImpliedReturn();
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs);
    unsigned getGoto();
    unsigned getBreak();
    unsigned getContinue();
    ScopeStatement *getScope();
};

struct IfStatement : Statement
{
    Expression *condition;
    Statement *ifbody;
    Statement *elsebody;

    IfStatement(Loc loc, Expression *condition,
		Statement *ifbody, Statement *elsebody);
    Statement *semantic(Scope *sc);
    TopStatement *ImpliedReturn();
    void toIR(IRstate *irs);
};

struct DefaultStatement;

struct SwitchStatement : Statement
{
    Expression *condition;
    Statement *body;
    unsigned breakIP;
    ScopeStatement *scopeContext;

    DefaultStatement *swdefault;
    Array *cases;		// array of CaseStatement's

    SwitchStatement(Loc loc, Expression *c, Statement *b);
    Statement *semantic(Scope *sc);
    void toIR(IRstate *irs);
    unsigned getBreak();
    ScopeStatement *getScope();
};

struct CaseStatement : Statement
{
    Expression *exp;
    unsigned caseIP;
    unsigned patchIP;

    CaseStatement(Loc loc, Expression *exp);
    Statement *semantic(Scope *sc);
    void toIR(IRstate *irs);
};

struct DefaultStatement : Statement
{
    unsigned defaultIP;

    DefaultStatement(Loc loc);
    Statement *semantic(Scope *sc);
    void toIR(IRstate *irs);
};

struct DoStatement : Statement
{
    Statement *body;
    Expression *condition;
    unsigned breakIP;
    unsigned continueIP;
    ScopeStatement *scopeContext;

    DoStatement(Loc loc, Statement *b, Expression *c);
    Statement *semantic(Scope *sc);
    TopStatement *ImpliedReturn();
    void toIR(IRstate *irs);
    unsigned getBreak();
    unsigned getContinue();
    ScopeStatement *getScope();
};

struct WhileStatement : Statement
{
    Expression *condition;
    Statement *body;
    unsigned breakIP;
    unsigned continueIP;
    ScopeStatement *scopeContext;

    WhileStatement(Loc loc, Expression *c, Statement *b);
    Statement *semantic(Scope *sc);
    TopStatement *ImpliedReturn();
    void toIR(IRstate *irs);
    unsigned getBreak();
    unsigned getContinue();
    ScopeStatement *getScope();
};

struct ForStatement : Statement
{
    Statement *init;
    Expression *condition;
    Expression *increment;
    Statement *body;
    unsigned breakIP;
    unsigned continueIP;
    ScopeStatement *scopeContext;

    ForStatement(Loc loc, Statement *init, Expression *condition, Expression *increment, Statement *body);
    Statement *semantic(Scope *sc);
    TopStatement *ImpliedReturn();
    void toIR(IRstate *irs);
    unsigned getBreak();
    unsigned getContinue();
    ScopeStatement *getScope();
};

struct ForInStatement : Statement
{
    Statement *init;
    Expression *in;
    Statement *body;
    unsigned breakIP;
    unsigned continueIP;
    ScopeStatement *scopeContext;

    ForInStatement(Loc loc, Statement *init, Expression *in, Statement *body);
    Statement *semantic(Scope *sc);
    TopStatement *ImpliedReturn();
    void toIR(IRstate *irs);
    unsigned getBreak();
    unsigned getContinue();
    ScopeStatement *getScope();
};

struct ScopeStatement : Statement
{
    ScopeStatement *enclosingScope;
    int depth;			// syntactical nesting level of ScopeStatement's
    int npops;			// how many items added to scope chain

    ScopeStatement(Loc loc);
};

struct WithStatement : ScopeStatement
{
    Expression *exp;
    Statement *body;

    WithStatement(Loc loc, Expression *exp, Statement *body);
    Statement *semantic(Scope *sc);
    TopStatement *ImpliedReturn();
    void toIR(IRstate *irs);
};

struct ContinueStatement : Statement
{
    Identifier *ident;
    Statement *target;

    ContinueStatement(Loc loc, Identifier *ident);
    Statement *semantic(Scope *sc);
    void toIR(IRstate *irs);
    unsigned getTarget();
};

struct BreakStatement : Statement
{
    Identifier *ident;
    Statement *target;

    BreakStatement(Loc loc, Identifier *ident);
    Statement *semantic(Scope *sc);
    void toIR(IRstate *irs);
    unsigned getTarget();
};

struct GotoStatement : Statement
{
    Identifier *ident;
    LabelSymbol *label;

    GotoStatement(Loc loc, Identifier *ident);
    Statement *semantic(Scope *sc);
    void toIR(IRstate *irs);
    unsigned getTarget();
};

struct ReturnStatement : Statement
{
    Expression *exp;

    ReturnStatement(Loc loc, Expression *exp);
    Statement *semantic(Scope *sc);
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs);
};

struct ImpliedReturnStatement : Statement
{
    Expression *exp;

    ImpliedReturnStatement(Loc loc, Expression *exp);
    Statement *semantic(Scope *sc);
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs);
};

struct ThrowStatement : Statement
{
    Expression *exp;

    ThrowStatement(Loc loc, Expression *exp);
    Statement *semantic(Scope *sc);
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs);
};

struct TryStatement : ScopeStatement
{
    Statement *body;
    Identifier *catchident;
    Statement *catchbody;
    Statement *finalbody;

    TryStatement(Loc loc, Statement *body,
	Identifier *catchident, Statement *catchbody,
	Statement *finalbody);
    Statement *semantic(Scope *sc);
    void toBuffer(OutBuffer *buf);
    void toIR(IRstate *irs);
};

#endif
