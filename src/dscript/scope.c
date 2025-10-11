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

#include "root.h"
#include "mem.h"
#include "identifier.h"
#include "scope.h"
#include "program.h"
#include "statement.h"

#define FREELIST	0	// don't use for multithreaded versions

Scope *Scope::freelist = NULL;

//int nscopes;

void *Scope::operator new(size_t size)
{
#if FREELIST
    Scope *s;

    if (freelist)
    {
	s = freelist;
	freelist = s->enclosing;
	return s;
    }
#endif

    //nscopes++;
    //WPRINTF(L"nscopes = %d\n", nscopes);
    return mem.malloc(size);
}

void Scope::zero()
{
    enclosing = NULL;

    src = NULL;
    program = NULL;
    scopesym = NULL;
    funcdef = NULL;
    plabtab = NULL;
    nestDepth = 0;

    scopeContext = NULL;
    continueTarget = NULL;
    breakTarget = NULL;
    switchTarget = NULL;
}

Scope::Scope(Scope *enclosing)
{
    zero();
    this->program = enclosing->program;
    this->funcdef = enclosing->funcdef;
    this->plabtab = enclosing->plabtab;
    this->nestDepth = enclosing->nestDepth;
    this->enclosing = enclosing;
}

Scope::Scope(Program *program, FunctionDefinition *fd)
{   // Create root scope

    zero();
    this->program = program;
    this->funcdef = fd;
    this->plabtab = &fd->labtab;
}

Scope::Scope(FunctionDefinition *fd)
{   // Create scope for anonymous function fd

    zero();
    this->funcdef = fd;
    this->plabtab = &fd->labtab;
}

Scope::~Scope()
{
    // Help garbage collector
    zero();
}

Scope *Scope::push()
{   Scope *s;

    s = new Scope(this);
    return s;
}

Scope *Scope::push(FunctionDefinition *fd)
{   Scope *s;

    s = push();
    s->funcdef = fd;
    s->plabtab = &fd->labtab;
    return s;
}

void Scope::pop()
{
    if (enclosing && !enclosing->errinfo.message)
	enclosing->errinfo = errinfo;
#if FREELIST
    enclosing = freelist;
    freelist = this;
#endif
    zero();			// aid GC
}


Symbol *Scope::search(Identifier *ident)
{   Symbol *s;
    Scope *sc;

    //PRINTF("Scope::search(%p, '%s')\n", this, ident->toChars());
    for (sc = this; sc; sc = sc->enclosing)
    {
	s = sc->scopesym->search(ident);
	if (s)
	    return s;
    }

    assert(0);
    //error(DTEXT("Symbol '%s' is not defined"), ident->toChars());
    return NULL;
}

Symbol *Scope::insert(Symbol *s)
{
    if (!scopesym->symtab)
	scopesym->symtab = new SymbolTable();
    return scopesym->symtab->insert(s);
}

LabelSymbol *Scope::searchLabel(Identifier *ident)
{
    SymbolTable *st;
    LabelSymbol *ls;

    //WPRINTF(L"Scope::searchLabel('%ls')\n", ident->toDchars());
    assert(plabtab);
    st = *plabtab;
    if (!st)
    {	GC_LOG();
	st = new SymbolTable();
	*plabtab = st;
    }
    ls = (LabelSymbol *)st->lookup(ident);
    return ls;
}

LabelSymbol *Scope::insertLabel(LabelSymbol *ls)
{
    SymbolTable *st;

    //PRINTF("Scope::insertLabel('%s')\n", ls->toChars());
    assert(plabtab);
    st = *plabtab;
    if (!st)
    {	GC_LOG();
	st = new SymbolTable();
	*plabtab = st;
    }
    ls = (LabelSymbol *)st->insert(ls);
    return ls;
}

dchar *Scope::getSource()
{   Scope *sc;

    for (sc = this; sc; sc = sc->enclosing)
    {
	if (sc->src)
	    return sc->src;
    }

    return NULL;
}


