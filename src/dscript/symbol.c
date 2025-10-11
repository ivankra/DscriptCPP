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
#include <assert.h>

#include "symbol.h"
#include "identifier.h"
#include "statement.h"
#include "scope.h"
#include "text.h"

/****************************** Symbol ******************************/

Symbol::Symbol()
{
    this->ident = NULL;
    this->parent = NULL;
}

Symbol::Symbol(Identifier *ident)
{
    this->ident = ident;
    this->parent = NULL;
}

int Symbol::equals(Object *o)
{   Symbol *s;

    if (this == o)
	return TRUE;
#if DYNAMIC_CAST
    s = dynamic_cast<Symbol *>(o);
    if (s && ident->equals(s->ident))
	return TRUE;
#else
    assert(0);
#endif
    return FALSE;
}

char *Symbol::toChars()
{
    return ident ? (char*)"__ident" : (char*)"__anonymous";
}

void Symbol::semantic(Scope *sc)
{
    error(errmsg(ERR_NO_SEMANTIC_ROUTINE), toChars());
}

Symbol *Symbol::search(Identifier *ident)
{
    assert(0);
    //error(DTEXT("%s.%s is undefined"),toChars(), ident->toChars());
    return this;
}

void Symbol::toBuffer(OutBuffer *buf)
{
    buf->write(this);
}

/********************************* ScopeSymbol ****************************/

ScopeSymbol::ScopeSymbol()
    : Symbol()
{
    members = NULL;
    symtab = NULL;
}

ScopeSymbol::ScopeSymbol(Identifier *id)
    : Symbol(id)
{
    members = NULL;
    symtab = NULL;
}

Symbol *ScopeSymbol::search(Identifier *ident)
{   Symbol *s;

    //PRINTF("ScopeSymbol::search(%s, '%s')\n", toChars(), ident->toChars());
    // Look in symbols declared in this module
    s = symtab ? symtab->lookup(ident) : NULL;
    if (s)
	PRINTF("\ts = '%s.%s'\n",toChars(),s->toChars());
    return s;
}

void ScopeSymbol::multiplyDefined(Symbol *s)
{
    error(errmsg(ERR_REDEFINED_OBJECT_SYMBOL), toChars(), s->toChars());
}

/****************************** SymbolTable ******************************/

/* This is only used for statement labels, a rare occurrence, so we just
 * use a linear array.
 */

SymbolTable::SymbolTable()
{
}

SymbolTable::~SymbolTable()
{
    members.zero();
}

Symbol *SymbolTable::lookup(Identifier *ident)
{
    for (unsigned i = 0; i < members.dim; i++)
    {
	Symbol *s = (Symbol *)members.data[i];

	if (s->ident->equals(ident))
	    return s;
    }
    return NULL;
}

Symbol *SymbolTable::insert(Symbol *s)
{
    for (unsigned i = 0; i < members.dim; i++)
    {
	Symbol *sm = (Symbol *)members.data[i];

	if (sm->ident->equals(s->ident))
	    return NULL;	// already in table
    }
    members.push(s);
    return s;
}

Symbol *SymbolTable::update(Symbol *s)
{
    for (unsigned i = 0; i < members.dim; i++)
    {
	Symbol *sm = (Symbol *)members.data[i];

	if (sm->ident->equals(s->ident))
	{   members.data[i] = (void *)s;
	    return s;
	}
    }
    members.push(s);
    return s;
}

/****************************** FunctionSymbol ******************************/

FunctionSymbol::FunctionSymbol(Loc loc, Identifier *ident,
	Array *parameters,
	Array *topstatements)
	: ScopeSymbol(ident)
{
    this->loc = loc;
    this->parameters = parameters;
    this->topstatements = topstatements;
    labtab = NULL;
    code = NULL;
    nlocals = 0;
}

void FunctionSymbol::semantic(Scope *sc)
{
}

/****************************** LabelSymbol ******************************/

LabelSymbol::LabelSymbol(Loc loc, Identifier *ident, LabelStatement *statement)
	: Symbol(ident)
{
    this->loc = loc;
    this->statement = statement;
}


