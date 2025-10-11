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

#ifndef SCOPE_H
#define SCOPE_H

struct Symbol;
struct SymbolTable;
struct ScopeSymbol;
struct Identifier;
struct Program;
struct Statement;
struct SwitchStatement;
struct LabelSymbol;
struct ScopeStatement;
struct FunctionDefinition;

struct Scope
{
    Scope *enclosing;		// enclosing Scope

    dchar *src;			// source text
    Program *program;		// Root module
    ScopeSymbol *scopesym;	// current symbol
    FunctionDefinition *funcdef; // what function we're in
    SymbolTable **plabtab;	// pointer to label symbol table
    unsigned nestDepth;         // static nesting level

    ScopeStatement *scopeContext;	// nesting of scope statements
    Statement *continueTarget;
    Statement *breakTarget;
    SwitchStatement *switchTarget;

    ErrInfo errinfo;		// semantic() puts error messages here

    static Scope *freelist;
    static void *operator new(size_t sz);
    void zero();		// zero out all members

    Scope(Program *program, FunctionDefinition *fd);
    Scope(Scope *enclosing);
    Scope(FunctionDefinition *fd);
    ~Scope();

    dchar *getSource();

    Scope *push();
    Scope *push(FunctionDefinition *fd);
    void pop();

    Symbol *search(Identifier *ident);
    Symbol *insert(Symbol *s);

    // Labels
    LabelSymbol *searchLabel(Identifier *ident);
    LabelSymbol *insertLabel(LabelSymbol *ls);

    // With
    void incwith();
    void decwith();
};

#endif
