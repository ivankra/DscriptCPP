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
#include <stdarg.h>
#include <assert.h>
#include <locale.h>
#if defined __DMC__
#include <trace.h>
#endif
#if defined linux
#include <wchar.h>
#endif

#include "mem.h"
#include "printf.h"
#include "date.h"

#include "dscript.h"
#include "program.h"
#include "identifier.h"
#include "lexer.h"
#include "value.h"
#include "dobject.h"

struct IUnknown;
typedef struct tagVARIANT
{
} VARIANTARG, VARIANT;
typedef long HRESULT;

Global global;

Global::Global()
{
    source_ext = "ds";
    sym_ext  = "sym";
    copyright = "Copyright (c) 1999-2007 by Digital Mars";
    written = "written by Walter Bright";
    verbose = 0;
}

void exception(dchar *msg, ...)
{
#if defined linux
    printf("Exception:\n");
    for (; *msg; msg++)
	fputc(*msg, stdout);
    fputc('\n', stdout);
#elif defined UNICODE
    wprintf(L"Exception: %ls\n", msg);
#else
    printf("Exception: %s\n", msg);
#endif
    exit(EXITCODE_RUNTIME_ERROR);
}

dchar *errmsg(int msgnum)
{
#if defined UNICODE
    return ascii2unicode(errmsgtbl[msgnum]);
#else
    return errmsgtbl[msgnum];
#endif
}

void RuntimeErrorx(int msgnum, ...)
{
    va_list ap;

    va_start(ap, msgnum);
#if defined UNICODE
    dchar *format = errmsg(msgnum);
    WPRINTF(L"\n%s", errmsg(ERR_RUNTIME_PREFIX));
    VWPRINTF(format, ap);
    WPRINTF(L"\n");
#else
    printf("\nDMDScript fatal runtime error: ");
    fflush(stdout);
    vprintf(format, ap);
    printf("\n");
    fflush(stdout);
#endif
    va_end( ap );

    exit(EXITCODE_RUNTIME_ERROR);
}

// Dummy so standalone version will link
void Denumerator_init(ThreadContext *tc)
{
}

void Denumerator_add(Dobject *)
{
}

void Dvbarray_init(ThreadContext *tc)
{
}

void Dvbarray_add(Dobject *)
{
}

Dobject *Dvbarray_ctor(Value *)
{
    return NULL;
}

// Dummy so standalone version will link
void Dcomobject_init(ThreadContext *tc)
{
}

void Dcomobject_addCOM(Dobject *)
{
}

void *dcomobject_call(d_string s, CALL_ARGS)
{
    assert(0);
    return NULL;
}

d_boolean dcomobject_isNull(Dcomobject* pObj)
{
    (void)pObj;
    assert(0);
    return FALSE;
}  // dcomobject_isNull

d_boolean dcomobject_areEqual(Dcomobject* pLeft, Dcomobject* pRight)
{
    (void)pLeft;
    (void)pRight;
    assert(0);
    return FALSE;
}

IDispatch* Dobject::toComObject()
{
    return NULL;
}

void ValueToVariant(VARIANTARG *variant, Value *value)
{
}

HRESULT VariantToValue(Value *ret, VARIANTARG *variant, d_string PropertyName, Dcomobject *pParent)
{
    assert(0);
    return 0;
}

d_number parseDateString(CallContext *cc, d_string s)
{
    return Date::parse(d_string_ptr(s));
}

d_string dateToString(CallContext *cc, d_number t, enum TIMEFORMAT tf)
{   dchar *p;

    if (Port::isnan(t))
	p = L"Invalid Date";
    else
    {
	switch (tf)
	{
	    case TFString:
		t = Date::UTC(t);
		p = Date::ToString(t);
		break;
	    case TFDateString:
		t = Date::UTC(t);
		p = Date::ToDateString(t);
		break;
	    case TFTimeString:
		t = Date::UTC(t);
		p = Date::ToTimeString(t);
		break;

	    case TFLocaleString:
		p = Date::ToLocaleString(t);
		break;
	    case TFLocaleDateString:
		p = Date::ToLocaleDateString(t);
		break;
	    case TFLocaleTimeString:
		p = Date::ToLocaleTimeString(t);
		break;

	    default:
		p = NULL;	// suppress spurious warning
		assert(0);
	}
    }
    GC_LOG();
    return d_string_ctor(p);
}

int localeCompare(CallContext *cc, d_string s1, d_string s2)
{   // no locale support here
    return d_string_cmp(s1, s2);
}

/**************************************************
	Usage:

	    ds
		will run test.ds

	    ds foo
		will run foo.ds

	    ds foo.js
		will run foo.js

	    ds foo1 foo2 foo.bar
		will run foo1.ds, foo2.ds, foo.bar

	The -iinc flag will prefix the source files with the contents of file inc.
	There can be multiple -i's. The include list is reset to empty any time
	a new -i is encountered that is not preceded by a -i.

	    ds -iinc foo
		will prefix foo.ds with inc

	    ds -iinc1 -iinc2 foo bar
		will prefix foo.ds with inc1+inc2, and will prefix bar.ds
		with inc1+inc2

	    ds -iinc1 -iinc2 foo -iinc3 bar
		will prefix foo.ds with inc1+inc2, and will prefix bar.ds
		with inc3

	    ds -iinc1 -iinc2 foo -i bar
		will prefix foo.ds with inc1+inc2, and will prefix bar.ds
		with nothing

 */

int main(int argc, char *argv[])
{
    unsigned i;
    unsigned errors = 0;
    Array includes;
    Array programs;
    char *p;
    Program *m;
    int result;
    int flag;
    ErrInfo errinfo;

    fprintf(stderr, "Digital Mars DMDScript 1.05\nhttp://www.digitalmars.com\n");
#if defined(__DMC_VERSION_STRING__)
    fprintf(stderr, "Compiled by %s\n", __DMC_VERSION_STRING__);
#elif defined(_MSC_VER)
    fprintf(stderr, "Compiled by Microsoft VC++\n");
#elif defined(__GNUC__)
    fprintf(stderr, "Compiled by GNU C++\n");
#endif 
    //fprintf(stderr, "%s\n", global.copyright);
    fprintf(stderr, "%s\n", global.written);

    printf_tostdout();
    mem.init();
    mem.setStackBottom(&argv);
    ThreadContext::init();
    Date::init();
#if defined(UNICODE) 
#if defined(linux)
    setlocale(LC_ALL, "");
#else
    _wsetlocale(LC_ALL, L"");
#endif // !linux
#else
    setlocale(LC_ALL, "");
#endif // !UNICODE

    my_response_expand(&argc, &argv);
    programs.reserve(argc - 1);
    flag = 0;
    for (i = 1; i < (unsigned) argc; i++)
    {
	p = argv[i];
	if (*p == '-')
	{
	    switch (p[1])
	    {
		case 'i':
		    if (flag)			// any intervening files?
			includes.dim = 0;	// yes, reset includes Array
		    flag = 0;
		    if (p[2])
		    {	File *f = new File(p + 2);
			includes.push(f);
		    }
		    break;

		case 'v':
		    global.verbose = 1;
		    break;

		default:
		    error(errmsg(ERR_BAD_SWITCH),p);
		    errors++;
		    break;
	    }
	}
	else
	{
	    flag = 1;
	    m = new Program(p, global.source_ext);
	    m->setIncludes(&includes);
	    programs.push(m);
	}
    }
    if (errors)
	return EXITCODE_INVALID_ARGS;

    if (programs.dim == 0)
    {
	m = new Program("test", global.source_ext);
	programs.push(m);
    }

    fprintf(stderr, "%d source files\n", programs.dim);

    // Initialization
    //Lexer::initKeywords();

    // Read files, parse them
    for (i = 0; i < programs.dim; i++)
    {
	m = (Program *)programs.data[i];
	if (global.verbose)
	    PRINTF("parse   %s:\n", m->srcfile->toChars());
	m->read();
	if (m->parse(&errinfo))
	    exception(errinfo.message);
    }
    mem.fullcollect();

    // Execute
    result = EXIT_SUCCESS;
    for (i = 0; i < programs.dim; i++)
    {
	m = (Program *)programs.data[i];
	if (global.verbose)
	    PRINTF("execute %s:\n", m->srcfile->toChars());
	if (m->execute(argc, argv, &errinfo))
	    exception(errinfo.message);
    }

#if defined __DMC__
    trace_term();
#endif
    mem.fullcollect();
    return result;
}
