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

#ifndef PROGRAM_H
#define PROGRAM_H

#include "root.h"
#include "symbol.h"
#include "ir.h"

struct Identifier;
struct FunctionDefinition;
struct IR;
struct Dglobal;
struct ErrInfo;

struct Program : Object
{
    const char *arg;	// original argument name
    File *srcfile;	// input source file
    Array *includes;	// array of #include'd File's
    unsigned errors;	// if any errors in file

    CallContext *callcontext;

    FunctionDefinition *globalfunction;

    // Locale info
    unsigned long lcid;	// current locale
    dchar *slist;	// list separator

    Program(char *arg, char *source_ext);
    ~Program();

    void initContext();
    void setIncludes(Array *includes);
    void read();	// read file
    int parse(ErrInfo *perrinfo);	// syntactic parse
    int parse(dchar *program, long length, FunctionDefinition **pfd, ErrInfo *perrinfo);
    int parse_common(char *progIdentifier, dchar *program, long length, FunctionDefinition **pfd, ErrInfo *perrinfo);
    int execute(int argc, char **argv, ErrInfo *perrinfo);	// execute program
    void toBuffer(OutBuffer *buf);

    static Program *getProgram();
    static void setProgram(Program *p);
};

#endif
