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

#ifndef SYMBOL_H
#define SYMBOL_H

#include "root.h"
#include "dscript.h"

struct Identifier;
struct Scope;
struct SymbolTable;
struct Expression;
struct Statement;
struct LabelStatement;
struct IR;

struct Symbol : Object
{
    Identifier *ident;
    Symbol *parent;

    Symbol();
    Symbol(Identifier *);
    char *toChars();
    int equals(Object *o);

    virtual void semantic(Scope *sc);
    virtual Symbol *search(Identifier *ident);
    virtual void toBuffer(OutBuffer *buf);
};

// Symbol that generates a scope

struct ScopeSymbol : Symbol
{
    Array *members;		// all Symbol's in this scope
    SymbolTable *symtab;	// member[] sorted into table

    ScopeSymbol();
    ScopeSymbol(Identifier *id);
    Symbol *search(Identifier *ident);
    void multiplyDefined(Symbol *s);
};

// Table of Symbol's

struct SymbolTable : Object
{
    Array members;	// enumerated array of all Symbol's in table

    SymbolTable();
    ~SymbolTable();

    // Look up Identifier. Return Symbol if found, NULL if not.
    Symbol *lookup(Identifier *ident);

    // Insert Symbol in table. Return NULL if already there.
    Symbol *insert(Symbol *s);

    // Look for Symbol in table. If there, return it.
    // If not, insert s and return that.
    Symbol *update(Symbol *s);
};

struct FunctionSymbol : ScopeSymbol
{
    Loc loc;

    Array *parameters;		// array of Identifier's
    Array *topstatements;	// array of TopStatement's

    SymbolTable *labtab;	// symbol table for LabelSymbol's

    IR *code;
    unsigned nlocals;

    FunctionSymbol(Loc loc, Identifier *ident, Array *parameters,
	Array *topstatements);
    void semantic(Scope *sc);
};

struct LabelSymbol : Symbol
{   Loc loc;
    LabelStatement *statement;

    LabelSymbol(Loc loc, Identifier *ident, LabelStatement *statement);
};

#endif
