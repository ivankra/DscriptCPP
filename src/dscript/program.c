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

#if _MSC_VER
#include <malloc.h>
#endif

#include "gc.h"

#include "mem.h"
#include "dchar.h"
#include "root.h"
#include "thread.h"
#include "mutex.h"
#include "perftimer.h"

#include "dscript.h"
#include "program.h"
#include "identifier.h"
#include "parse.h"
#include "scope.h"
#include "statement.h"
#include "lexer.h"
#include "ir.h"
#include "dobject.h"
#include "text.h"

#define LOG	0	// log source

int logflag;

Program::Program(char *arg, char *source_ext)
{
    Value::init();

    this->arg = arg;
    errors = 0;
    srcfile = NULL;
    globalfunction = NULL;
    includes = NULL;
    callcontext = NULL;

    // If this is run as a DLL, arg is always NULL
    if (arg)
    {
	FileName *srcfilename;

	srcfilename = FileName::defaultExt(arg, source_ext);
	srcfile = new File(srcfilename);
    }

    initContext();
}

void Program::initContext()
{
    if (callcontext)		// if already done
	return;

    GC_LOG();
    callcontext = new CallContext();

    CallContext *cc = callcontext;
    memset(cc, 0, sizeof(CallContext));

    // Do object inits
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);

#if 0
    {
	unsigned *p;

	WPRINTF(L"tc = %x\n", tc);
	p = (unsigned *)tc->Dregexp_constructor;
	WPRINTF(L"p = %x\n", p);
	if (p)
	    WPRINTF(L"*p = %x, %x, %x, %x\n", p[0], p[1], p[2], p[3]);
    }
#endif

    dobject_init(tc);

    cc->prog = this;

    // Create global object
    GC_LOG();
    cc->global = new Dglobal(0, 0);

    GC_LOG();
    Array *scope = new Array;
    scope->reserve(10);
    scope->push(cc->global);

    cc->variable = cc->global;
    cc->scope = scope;
    cc->scoperoot++;
    cc->globalroot++;
}

Program::~Program()
{
    arg = NULL;
    srcfile = NULL;
}

void Program::setIncludes(Array *includes)
{
    // This creates a copy of the includes[], not a reference to it.
    // A copy is necessary because the caller will later modify includes[].
    GC_LOG();
    this->includes = new Array();
    this->includes->append(includes);
}

void Program::read()
{
    //PRINTF("Program::read() file '%s'\n",srcfile->toChars());
    srcfile->readv();
    srcfile->toUnicode();

    // Read the includes[] files
    if (includes)
    {	unsigned i;
	unsigned char *buffer;
	unsigned char *buf;
	unsigned len = srcfile->len;

	for (i = 0; i < includes->dim; i++)
	{
	    File *f = (File *)includes->data[i];

	    if (!f->buffer)		// if not already read in
	    {	f->readv();		// read it
		f->toUnicode();
	    }
	    len += f->len;
	}

	// Prefix the includes[] files

GC_LOG();
	buffer = (unsigned char *)mem.malloc((len + 1) * sizeof(dchar));
	buf = buffer;
	for (i = 0; i < includes->dim; i++)
	{
	    File *f = (File *)includes->data[i];

	    memcpy(buf, f->buffer, f->len);
	    buf += f->len;
	}
	memcpy(buf, srcfile->buffer, srcfile->len);
	buf += srcfile->len;
	assert(len == (unsigned)(buf - buffer));
	*(dchar *)buf = 0x1A;		// ending sentinal
	srcfile->buffer = buffer;
	srcfile->len = len;

	/* Note: for much faster speed, consider parsing the include files
	   and then prepend the TopStatements. That way, the files are only
	   parsed once no matter how many times they are used.
	   But our purpose is stress testing coverage, not speed, and so we parse
	   over and over.
	 */
    }
}

// Entry point for DLL version

int Program::parse(dchar *program, long length, FunctionDefinition **pfd, ErrInfo *perrinfo)
{   static char progIdentifier[] = "Anonymous";
    int result;

    //PRINTF("Program::parse(dchar *, long)\n");

#if LOG
    // Log to file "ds.log"
    File log("c:\\ds.log");
    log.buffer = (unsigned char *)"\r\n------\r\n";
    log.len = strlen((char *)log.buffer);
    log.ref = 1;
    log.append();

    log.buffer = (unsigned char *) program;
    log.len = length;
    log.ref = 1;
    {	// Convert unicode to ascii
	int i;

	GC_LOG();
	log.buffer = (unsigned char *)mem.malloc(length / sizeof(dchar));
	for (i = 0; i < length / sizeof(dchar); i++)
	{
	    log.buffer[i] = (unsigned char) program[i];
	}
	log.len = length / sizeof(dchar);
	log.ref = 0;
    }
    log.append();
#endif

#if 1
    result = parse_common(progIdentifier, program, length / sizeof(dchar), pfd, perrinfo);
#else
    dchar *pTemp;

    // Allocate buffer, copy and add sentinel
    length /= sizeof(dchar);
    GC_LOG();
    pTemp = (dchar *)mem.malloc_atomic((length + 2) * sizeof(dchar));
    if (0 == pTemp) {
	// Garbage collector is a full trash can
	perrinfo->message = DTEXT("garbage collector is full");
	perrinfo->linnum = 0;
	return 1;
    }

    memcpy(pTemp, program, length * sizeof(dchar));
    pTemp[length] = 0x1A;
    pTemp[length + 1] = '\0';

    result = parse_common(progIdentifier, (dchar *)pTemp, length + 1, pfd, perrinfo);
#endif
    return result;
}

// Entry point for command line version

int Program::parse(ErrInfo *perrinfo)
{
    //PRINTF("Program::parse()\n");
#if LOG
    // Log to file "ds.log"
    File log("ds.log");
    log.buffer = (unsigned char *) srcfile->buffer;
    log.len = srcfile->len;
    log.ref = 1;
    log.append();
#endif
    return parse_common(srcfile->name->toChars(), (dchar *)srcfile->buffer, srcfile->len / sizeof(dchar), NULL, perrinfo);
}

/**************************************************
 * Two ways of calling this:
 * 1. with text representing group of topstatements (pfd == NULL)
 * 2. with text representing a function name & body (pfd != NULL)
 */

int Program::parse_common(char *progIdentifier, dchar *program, long length, FunctionDefinition **pfd, ErrInfo *perrinfo)
{   Array *topstatements;
    dchar *msg;

    //PRINTF("parse_common()\n");
    Parser p(progIdentifier, program, length);

    topstatements = NULL;
{ //PerfTimer pf(L"parse   ");
    if (p.parseProgram(&topstatements, perrinfo))
    {
	if (topstatements)
	    topstatements->zero();
	return 1;
    }
}
    if (pfd)
    {	// If we are expecting a function, we should have parsed one
	assert(p.lastnamedfunc);
	*pfd = p.lastnamedfunc;
    }

    // Build empty function definition array
    // Make globalfunction an anonymous one (by passing in NULL for name) so
    // it won't get instantiated as a property
    GC_LOG();
    globalfunction = new FunctionDefinition(0, 1, NULL, NULL, NULL);

    // Any functions parsed in topstatements wind up in the global
    // object (cc->global), where they are found by normal property lookups.
    // Any global new top statements only get executed once, and so although
    // the previous group of topstatements gets lost, it does not matter.

    // In essence, globalfunction encapsulates the *last* group of topstatements
    // passed to dscript, and any previous version of globalfunction, along with
    // previous topstatements, gets discarded.

    globalfunction->topstatements = topstatements;

    // If pfd, it is not really necessary to create a global function just
    // so we can do the semantic analysis, we could use p.lastnamedfunc
    // instead if we're careful to insure that p.lastnamedfunc winds up
    // as a property of the global object.

    Scope sc(this, globalfunction);		// create global scope
    sc.src = program;
{ //PerfTimer pf(L"semantic");
    globalfunction->semantic(&sc);
}
    msg = sc.errinfo.message;
    if (msg)					// if semantic() failed
    {
	if (globalfunction->topstatements)
	    globalfunction->topstatements->zero();
	globalfunction->topstatements = NULL;
	globalfunction = NULL;
	*perrinfo = sc.errinfo;
	return 1;
    }

    if (pfd)
	// If expecting a function, that is the only topstatement we should
	// have had
	(*pfd)->toIR(NULL);
    else
    {	//PerfTimer pf(L"toIR    ");
	globalfunction->toIR(NULL);
    }

    // Don't need parse trees anymore, so NULL'ing the pointer allows
    // the garbage collector to find & free them.
    if (globalfunction->topstatements)
	globalfunction->topstatements->zero();
    globalfunction->topstatements = NULL;

    return 0;
}

/*******************************
 * Execute program.
 * Return 0 on success, !=0 on failure.
 */

int Program::execute(int argc, char **argv, ErrInfo *perrinfo)
{
    // ECMA 10.2.1
    //PRINTF("Program::execute(argc = %d, argv = %p)\n", argc, argv);
    //PRINTF("Program::execute()\n");

    initContext();

    Value *locals;
    Value ret;
    Value *result;
    CallContext *cc = callcontext;
    Darray *arguments;
    Dobject *dglobal = cc->global;
    Program *program_save;
    Mem mem;

    // Set argv and argc for execute    
    GC_LOG();
    arguments = new Darray();
    dglobal->Put(TEXT_arguments, arguments, DontDelete | DontEnum);
    arguments->length = argc;
    for (int i = 0; i < argc; i++)
    {
	arguments->Put(i, Dstring::dup(&mem, argv[i]), DontEnum);
    }

    void *p1 = NULL;
    if (globalfunction->nlocals >= 128 ||
	(locals = (Value *) alloca(globalfunction->nlocals * sizeof(Value))) == NULL)
    {
	GC_LOG();
	p1 = mem.malloc(globalfunction->nlocals * sizeof(Value));
	locals = (Value *)p1;
    }

    // Instantiate global variables as properties of global
    // object with 0 attributes
    globalfunction->instantiate(cc->scope, cc->scope->dim, cc->variable, 0);

    cc->scope->reserve(globalfunction->withdepth + 1);

    // The 'this' value is the global object
    //PRINTF("scope = %p, cc->scope->dim = %d\n", cc->scope, cc->scope->dim);
    program_save = getProgram();
    setProgram(this);
    Value::copy(&ret, &vundefined);
    result = (Value *)IR::call(cc, cc->global, globalfunction->code, &ret, locals);
    setProgram(program_save);
    //PRINTF("-Program::execute()\n");
    if (result)
    {
	result->getErrInfo(perrinfo, cc->linnum);
	cc->linnum = 0;
	mem.free(p1);
	return 1;
    }

    mem.free(p1);
    return 0;
}

void Program::toBuffer(OutBuffer *buf)
{
    if (globalfunction)
	globalfunction->toBuffer(buf);
}

/***********************************************
 * Get/Set Program associated with this thread.
 */

Program *Program::getProgram()
{
    ThreadContext *tc;

    tc = ThreadContext::getThreadContext();
    assert(tc != 0);
    return tc->program;
}

void Program::setProgram(Program *p)
{
    ThreadContext *tc;

    tc = ThreadContext::getThreadContext();

    assert(tc != 0);
    tc->program = p;
}

