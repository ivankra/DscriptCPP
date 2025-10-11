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

#ifndef PARSE_H
#define PARSE_H

#include "root.h"
#include "lexer.h"

struct Expression;
struct Statement;
struct TopStatement;
struct FunctionDefinition;

struct Parser : Lexer
{
    unsigned flags;

    #define normal	0
    #define initial	1

    #define allowIn	0
    #define noIn	2

    // Flag if we're in the for statement header, as
    // automatic semicolon insertion is suppressed inside it.
    #define inForHeader	4

    FunctionDefinition *lastnamedfunc;

    Parser(const char *sourcename, dchar *base, unsigned length);
    Parser(const char *sourcename, d_string source);

    ~Parser() { lastnamedfunc = NULL; }

    static int parseFunctionDefinition(FunctionDefinition **pfd, d_string p, d_string body, ErrInfo *perrinfo);

    int parseProgram(Array **, ErrInfo *perrinfo);
    Array *parseTopStatements();
    TopStatement *parseFunction(int flag);
    Statement *parseStatement();
    void parseOptionalSemi();
    int check(enum TOK value);

    Expression *parseExpression(unsigned flags = 0);
    Expression *parseOptionalExpression(unsigned flags = 0);
    Expression *parseParenExp();
    Array *parseArguments();

    Expression *parseArrayLiteral();
    Expression *parseObjectLiteral();
    Expression *parseFunctionLiteral();

    Expression *parsePrimaryExp(int innew);
    Expression *parseUnaryExp();
    Expression *parsePostExp(Expression *e, int innew);
    Expression *parseMulExp();
    Expression *parseAddExp();
    Expression *parseShiftExp();
    Expression *parseRelExp();
    Expression *parseEqualExp();
    Expression *parseAndExp();
    Expression *parseXorExp();
    Expression *parseOrExp();
    Expression *parseAndAndExp();
    Expression *parseOrOrExp();
    Expression *parseCondExp();
    Expression *parseAssignExp();
};

#endif
