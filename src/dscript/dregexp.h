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

#ifndef DREGEXP_H
#define DREGEXP_H

#include "dscript.h"

struct Value;
struct Dobject;
struct Dfunction;
struct RegExp;

struct Dregexp : Dobject
{
    Value *global;
    Value *ignoreCase;
    Value *multiline;
    Value *lastIndex;
    Value *source;

    RegExp *re;

    Dregexp(d_string pattern, d_string attributes);
    Dregexp(Dobject *prototype);
    void *Call(CALL_ARGS);

    static void *exec(Dobject *othis, Value *ret, unsigned argc, Value *arglist, int rettype);
    static Dregexp *isRegExp(Value *v);

    static void init(ThreadContext *tc);

    static Dfunction *getConstructor();
    static Dobject *getPrototype();
};

// Values for Dregexp.exec.rettype
enum Rettype { EXEC_STRING, EXEC_ARRAY, EXEC_BOOLEAN, EXEC_INDEX };

#endif
