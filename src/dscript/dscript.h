/* Digital Mars DMDScript source code.
 * Copyright (c) 2000-2002 by Chromium Communications
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

// Dscript is set up to be compiled with 3 compilers:
//	__GNUC__	GNU C++ for linux platforms (use make -f linux.mak)
//	__DMC__		Digital Mars C++ for Win32 platforms (use make -f win32.mak)
//	_MSC_VER	Microsoft VC++ for Win32 platforms (use nmake -f msvc.mak)

#ifndef DSCRIPT_H
#define DSCRIPT_H

#include <stdio.h>
#include <stdarg.h>

#include "gc.h"
#include "printf.h"
#include "dchar.h"
#include "thread.h"
#include "mem.h"

/* =================== Configuration ======================= */

#define INVARIANT	0	// 0: turn off invariant checking
				// 1: turn on invariant checking

#define MAJOR_VERSION	5	// ScriptEngineMajorVersion
#define MINOR_VERSION	5	// ScriptEngineMinorVersion

#ifndef BUILD_VERSION		// if not set by makefile
#define BUILD_VERSION	1	// ScriptEngineBuildVersion
#endif

#define JSCRIPT_CATCH_BUG	1	// emulate Jscript's bug in scoping of
					// catch objects in violation of ECMA
#define JSCRIPT_ESCAPEV_BUG	0	// emulate Jscript's bug where \v is
					// not recognized as vertical tab

// if __istype is supported
#define ISTYPE	(__DMC__ >= 0x773)

// if dynamic_cast works
#if _MSC_VER
#define DYNAMIC_CAST	1
#else
#define DYNAMIC_CAST	0
#endif

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

struct Global : public Mem
{
    char *source_ext;
    char *sym_ext;
    char *copyright;
    char *written;
    int verbose;

    Global();
};

struct Array;
struct Dobject;
struct Dfunction;
struct Dmath;
struct Program;
struct Value;
struct StringTable;
struct FunctionDefinition;

struct CallContext : public Mem
{
    Array *scope;	// current scope chain
    Dobject *variable;	// object for variable instantiation
    Dobject *global;	// global object
    unsigned scoperoot;	// number of entries in scope[] starting from 0
			// to copy onto new scopes
    unsigned globalroot;// number of entries in scope[] starting from 0
			// that are in the "global" context. Always <= scoperoot
    void *lastnamedfunc;  // points to the last named function added as an event
    Program *prog;
    Dobject *callerothis;	// caller's othis
    Dobject *caller;		// caller function
    FunctionDefinition *callerf;

    char value[16];	// place to store exception; must be same size as Value
    unsigned linnum;	// source line number of exception (1 based, 0 if not available)

    int finallyret;	// !=0 if IRfinallyret was executed
    int Interrupt;	// !=0 if cancelled due to interrupt
};

// These are our per-thread global variables

struct ThreadContext
{
    ThreadId threadid;	// identifier of current thread
    Mem mem;		// thread specific storage allocator
    StringTable *stringtable;	// lexer's string table

  // Values from here to the end of the struct are 0'd by dobject_term()
    Program *program;	// associated data

    Dfunction *Dobject_constructor;
    Dobject *Dobject_prototype;

    Dfunction *Dfunction_constructor;
    Dobject *Dfunction_prototype;

    Dfunction *Darray_constructor;
    Dobject *Darray_prototype;

    Dfunction *Dstring_constructor;
    Dobject *Dstring_prototype;

    Dfunction *Dboolean_constructor;
    Dobject *Dboolean_prototype;

    Dfunction *Dnumber_constructor;
    Dobject *Dnumber_prototype;

    Dfunction *Derror_constructor;
    Dobject *Derror_prototype;

    Dfunction *Ddate_constructor;
    Dobject *Ddate_prototype;

    Dfunction *Dregexp_constructor;
    Dobject *Dregexp_prototype;

    Dfunction *Denumerator_constructor;
    Dobject *Denumerator_prototype;

    Dfunction *Dvbarray_constructor;
    Dobject *Dvbarray_prototype;

#define NativeError(n)				\
    Dfunction *D##n##error_constructor;		\
    Dobject *D##n##error_prototype;

NativeError(eval);
NativeError(range);
NativeError(reference);
NativeError(syntax);
NativeError(type);
NativeError(uri);

#undef NativeError

    Dmath *Dmath_object;

    // These go into the global object
    Dfunction *Dglobal_eval;
    Dfunction *Dglobal_parseInt;
    Dfunction *Dglobal_parseFloat;
    Dfunction *Dglobal_escape;
    Dfunction *Dglobal_unescape;
    Dfunction *Dglobal_isNaN;
    Dfunction *Dglobal_isFinite;
    Dfunction *Dglobal_decodeURI;
    Dfunction *Dglobal_decodeURIComponent;
    Dfunction *Dglobal_encodeURI;
    Dfunction *Dglobal_encodeURIComponent;
    Dfunction *Dglobal_print;
    Dfunction *Dglobal_println;
    Dfunction *Dglobal_ScriptEngine;
    Dfunction *Dglobal_ScriptEngineBuildVersion;
    Dfunction *Dglobal_ScriptEngineMajorVersion;
    Dfunction *Dglobal_ScriptEngineMinorVersion;

    static void init();
    static ThreadContext *getThreadContext();
};


extern Global global;

// Aliases for dscript primitive types
typedef unsigned	d_boolean;
typedef double		d_number;
typedef int		d_int32;
typedef unsigned	d_uint32;
typedef unsigned short	d_uint16;

#define ASCIZ	0	// 1 if use null-terminated strings
			// 0 if use Lstring's

#if ASCIZ
typedef dchar*			d_string;
#define d_string_null		((d_string)NULL)
#define d_string_len(s)		(Dchar::len(s))
#define d_string_ptr(s)		(s)
#define d_string_hash(s)	(Vstring::calcHash(s))
#define d_string_ctor(p)	(p)
#define d_string_cmp(s1, s2)	(Dchar::cmp(s1, s2))
#define d_string_setLen(s,len)
#else
struct Lstring;
typedef Lstring*		d_string;
#define d_string_null		((Lstring *)NULL)
#define d_string_len(s)		(s->len())
#define d_string_ptr(s)		(s->string)
#define d_string_hash(s)	(Vstring::calcHash(s))
#define d_string_ctor(p)	(GC_LOG(),(Lstring::ctor(p)))
#define d_string_cmp(s1, s2)	(Lstring::cmp(s1, s2))
#define d_string_setLen(s,len)	((s)->length = (len))
#endif

#if _MSC_VER
typedef unsigned __int64 number_t;
#else
typedef unsigned long long number_t;
#endif
typedef double real_t;

typedef unsigned Loc;		// file location (line number)

struct ErrInfo
{
    dchar *message;		// error message (NULL if no error)
    dchar *srcline;		// string of source line (NULL if not known)
    unsigned linnum;		// source line number (1 based, 0 if not available)
    int charpos;		// character position (1 based, 0 if not available)
    int code;			// error code (0 if not known)

    ErrInfo() { message = NULL; srcline = NULL; linnum = 0; charpos = 0; code = 0; }
};

#define TRUE	1
#define FALSE	0

dchar *errmsg(int msgnum);
void RuntimeErrorx(int msgnum, ...);
void dobject_init(ThreadContext *tc);
void dobject_term(ThreadContext *tc);
int StringToIndex(d_string name, d_uint32 *index);
void exception(dchar *msg, ...);
int my_response_expand(int *pargc, char ***pargv);

// There are so many places where this must line up, make it a macro
#define CONSTRUCT_ARGS CallContext *cc, Value *ret, unsigned argc, Value *arglist
#define CALL_ARGS      CallContext *cc, Dobject *othis, Value *ret, unsigned argc, Value *arglist

// Hash function for unsigned
#define HASH(u)		((u) ^ 0x55555555)
//#define HASH(u)		(u)

extern void Denumerator_init(ThreadContext *tc);
extern void Denumerator_add(Dobject *);
extern void Dvbarray_init(ThreadContext *tc);
extern void Dvbarray_add(Dobject *);
extern Dobject *Dvbarray_ctor(Value *);
extern void Dcomobject_init(ThreadContext *tc);
extern void Dcomobject_addCOM(Dobject *);

extern d_number parseDateString(CallContext *cc, d_string s);

enum TIMEFORMAT
{
	TFString,
	TFDateString,
	TFTimeString,
	TFLocaleString,
	TFLocaleDateString,
	TFLocaleTimeString,
	TFUTCString,
};

extern d_string dateToString(CallContext *cc, d_number t, enum TIMEFORMAT tf);
extern int localeCompare(CallContext *cc, d_string s1, d_string s2);

#if _MSC_VER
// Disable useless warnings about unreferenced formal parameters
#pragma warning (disable : 4100)
#endif

enum EXITCODE
{
	EXITCODE_INIT_ERROR = 1,
	EXITCODE_INVALID_ARGS = 2,
	EXITCODE_RUNTIME_ERROR = 3,
	EXITCODE_FILE_NOT_FOUND = 4
};

extern int logflag;	// used for debugging

#endif



