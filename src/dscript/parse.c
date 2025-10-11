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
#include <stdlib.h>

#include "gc.h"
#include "root.h"
#include "lexer.h"
#include "parse.h"
#include "statement.h"
#include "expression.h"
#include "symbol.h"
#include "program.h"
#include "identifier.h"
#include "text.h"

Parser::Parser(const char *sourcename, dchar *base, unsigned length)
    : Lexer(sourcename, base, length, 1)
{
    //PRINTF("Parser::Parser()\n");
    flags = 0;
    lastnamedfunc = NULL;
    nextToken();		// start up the scanner
}

Parser::Parser(const char *sourcename, d_string source)
    : Lexer(sourcename, d_string_ptr(source), d_string_len(source), 0)
{
    //PRINTF("Parser::Parser()\n");
    flags = 0;
    lastnamedfunc = NULL;
    nextToken();		// start up the scanner
}

/**********************************************
 * Clear stack.
 */

void clearstack()
{
    int a[2000];

    memset(a, 0, sizeof(a));
}

/**********************************************
 * Return !=0 on error, and fill in *perrinfo.
 */

int Parser::parseFunctionDefinition(FunctionDefinition **pfd, d_string params, d_string body, ErrInfo *perrinfo)
{
    Parser *p;
    Array parameters;
    Array *topstatements;
    FunctionDefinition *fd = NULL;
    int result;

    GC_LOG();
    p = new Parser("anonymous", params);

    // Parse FormalParameterList
    while (p->token.value != TOKeof)
    {
	if (p->token.value != TOKidentifier)
	{
	    p->error(ERR_FPL_EXPECTED_IDENTIFIER,
		p->token.toDchars());
	    goto Lreturn;
	}
	parameters.push(p->token.ident);
	p->nextToken();
	if (p->token.value == TOKcomma)
	    p->nextToken();
	else if (p->token.value == TOKeof)
	    break;
	else
	{
	    p->error(ERR_FPL_EXPECTED_COMMA,
		p->token.toDchars());
	    goto Lreturn;
	}
    }
    if (p->errinfo.message)
	goto Lreturn;

    delete p;

    // Parse StatementList
    GC_LOG();
    p = new Parser("anonymous", body);
    GC_LOG();
    topstatements = new Array();
    for (;;)
    {	TopStatement *ts;

	if (p->token.value == TOKeof)
	    break;
	ts = p->parseStatement();
	topstatements->push(ts);
    }

    GC_LOG();
    fd = new FunctionDefinition(0, 0, NULL, &parameters, topstatements);
    fd->isanonymous = 1;

Lreturn:
    *pfd = fd;
    *perrinfo = p->errinfo;
    result = (p->errinfo.message != NULL);
    delete p;
    p = NULL;
    //clearstack();
    return result;
}

/**********************************************
 * Return !=0 on error, and fill in *perrinfo.
 */

int Parser::parseProgram(Array **ptopstatements, ErrInfo *perrinfo)
{
    Array *topstatements;

    topstatements = parseTopStatements();
    check(TOKeof);
    //PRINTF("parseProgram done\n");
    *ptopstatements = topstatements;
    *perrinfo = errinfo;
    //clearstack();
    return errinfo.message != NULL;
}

Array *Parser::parseTopStatements()
{
    Array *topstatements;
    TopStatement *ts;

    //PRINTF("parseTopStatements()\n");
    GC_LOG();
    topstatements = new(gc) Array();
    for (;;)
    {
	switch (token.value)
	{
	    case TOKfunction:
		ts = parseFunction(0);
		topstatements->push(ts);
		break;

	    case TOKeof:
		return topstatements;

	    case TOKrbrace:
		return topstatements;

	    default:
		ts = parseStatement();
		topstatements->push(ts);
		break;
	}
    }
}

/***************************
 * flag:
 *	0	Function statement
 *	1	Function literal
 */

TopStatement *Parser::parseFunction(int flag)
{   Identifier *name;
    Array parameters;
    Array *topstatements;
    FunctionDefinition *f;
    Expression *e = NULL;
    Loc loc;

    //PRINTF("parseFunction()\n");
    loc = currentline;
    nextToken();
    name = NULL;
    if (token.value == TOKidentifier)
    {
	name = token.ident;
	nextToken();

	if (!flag && token.value == TOKdot)
	{
	    // Regard:
	    //	function A.B() { }
	    // as:
	    //	A.B = function() { }
	    // This is not ECMA, but a jscript feature

	    GC_LOG();
	    e = new(gc) IdentifierExpression(loc, name);
	    name = NULL;

	    while (token.value == TOKdot)
	    {
		nextToken();
		if (token.value == TOKidentifier)
		{   GC_LOG();
		    e = new(gc) DotExp(loc, e, token.ident);
		    nextToken();
		}
		else
		{
		    error(ERR_EXPECTED_IDENTIFIER_2PARAM, DTEXT("."), token.toDchars());
		    break;
		}
	    }
	}
    }

    check(TOKlparen);
    if (token.value == TOKrparen)
	nextToken();
    else
    {
	for (;;)
	{
	    if (token.value == TOKidentifier)
	    {
		parameters.push(token.ident);
		nextToken();
		if (token.value == TOKcomma)
		{   nextToken();
		    continue;
		}
		if (!check(TOKrparen))
		    break;
	    }
	    else
		error(ERR_EXPECTED_IDENTIFIER);
	    break;
	}
    }

    check(TOKlbrace);
    topstatements = parseTopStatements();
    check(TOKrbrace);

    GC_LOG();
    f = new(gc) FunctionDefinition(loc, 0, name, &parameters, topstatements);

    lastnamedfunc = f;

    //PRINTF("parseFunction() done\n");
    if (!e)
	return f;

    // Construct:
    //	A.B = function() { }

    GC_LOG();
    Expression *e2 = new(gc) FunctionLiteral(loc, f);

    GC_LOG();
    e = new(gc) AssignExp(loc, e, e2);

    GC_LOG();
    Statement *s = new(gc) ExpStatement(loc, e);

    return s;
}

/*****************************************
 */

Statement *Parser::parseStatement()
{   Statement *s;
    Token *t;
    Loc loc;

    //PRINTF("parseStatement()\n");
    loc = currentline;
    switch (token.value)
    {
	case TOKidentifier:
	case TOKthis:
	    // Need to look ahead to see if it is a declaration, label, or expression
	    t = peek(&token);
	    if (t->value == TOKcolon && token.value == TOKidentifier)
	    {	// It's a label
		Identifier *ident;

		ident = token.ident;
		nextToken();
		nextToken();
		s = parseStatement();
		GC_LOG();
		s = new(gc) LabelStatement(loc, ident, s);
	    }
	    else if (t->value == TOKassign ||
		     t->value == TOKdot ||
		     t->value == TOKlbracket)
	    {   Expression *exp;

		exp = parseExpression();
		parseOptionalSemi();
		GC_LOG();
		s = new(gc) ExpStatement(loc, exp);
	    }
	    else
	    {   Expression *exp;

		exp = parseExpression(initial);
		parseOptionalSemi();
		GC_LOG();
		s = new(gc) ExpStatement(loc, exp);
	    }
	    break;

	case TOKreal:
	case TOKstring:
	case TOKdelete:
	case TOKlparen:
	case TOKplusplus:
	case TOKminusminus:
	case TOKplus:
	case TOKminus:
	case TOKnot:
	case TOKtilde:
	case TOKtypeof:
	case TOKnull:
	case TOKnew:
	case TOKtrue:
	case TOKfalse:
	case TOKvoid:
	{   Expression *exp;

	    exp = parseExpression(initial);
	    parseOptionalSemi();
	    GC_LOG();
	    s = new(gc) ExpStatement(loc, exp);
	    break;
	}

	case TOKvar:
	{
	    Identifier *ident;
	    Expression *init;
	    VarDeclaration *v;
	    VarStatement *vs;

	    GC_LOG();
	    vs = new(gc) VarStatement(loc);
	    s = vs;

	    nextToken();
	    for (;;)
	    {	Loc loc = currentline;

		if (token.value != TOKidentifier)
		{
		    errinfo.code = 1010;
		    error(ERR_EXPECTED_IDENTIFIER_PARAM, token.toDchars());
		    break;
		}
		ident = token.ident;
		init = NULL;
		nextToken();
		if (token.value == TOKassign)
		{   unsigned flags_save;

		    nextToken();
		    flags_save = flags;
		    flags &= ~initial;
		    init = parseAssignExp();
		    flags = flags_save;
		}
		GC_LOG();
		v = new(gc) VarDeclaration(loc, ident, init);
		vs->vardecls.push(v);
		if (token.value != TOKcomma)
		    break;
		nextToken();
	    }
	    if (!(flags & inForHeader))
		parseOptionalSemi();
	    break;
	}

	case TOKlbrace:
	{   BlockStatement *bs;

	    nextToken();
	    GC_LOG();
	    bs = new(gc) BlockStatement(loc);
	    while (token.value != TOKrbrace)
	    {
		if (token.value == TOKeof)
		{   /* { */
		    error(ERR_UNTERMINATED_BLOCK);
		    break;
		}
		bs->statements.push(parseStatement());
	    }
	    s = bs;
	    nextToken();

	    // The following is to accommodate the jscript bug:
	    //	if (i) {return(0);}; else ...
	    if (token.value == TOKsemicolon)
		nextToken();

	    break;
	}

	case TOKif:
	{   Expression *condition;
	    Statement *ifbody;
	    Statement *elsebody;

	    nextToken();
	    condition = parseParenExp();
	    ifbody = parseStatement();
	    if (token.value == TOKelse)
	    {
		nextToken();
		elsebody = parseStatement();
	    }
	    else
		elsebody = NULL;
	    GC_LOG();
	    s = new(gc) IfStatement(loc, condition, ifbody, elsebody);
	    break;
	}

	case TOKswitch:
	{   Expression *condition;
	    Statement *body;

	    nextToken();
	    condition = parseParenExp();
	    body = parseStatement();
	    GC_LOG();
	    s = new(gc) SwitchStatement(loc, condition, body);
	    break;
	}

	case TOKcase:
	{   Expression *exp;

	    nextToken();
	    exp = parseExpression();
	    check(TOKcolon);
	    GC_LOG();
	    s = new(gc) CaseStatement(loc, exp);
	    break;
	}

	case TOKdefault:
	    nextToken();
	    check(TOKcolon);
	    GC_LOG();
	    s = new(gc) DefaultStatement(loc);
	    break;

	case TOKwhile:
	{   Expression *condition;
	    Statement *body;

	    nextToken();
	    condition = parseParenExp();
	    body = parseStatement();
	    GC_LOG();
	    s = new(gc) WhileStatement(loc, condition, body);
	    break;
	}

	case TOKsemicolon:
	    nextToken();
	    GC_LOG();
	    s = new(gc) EmptyStatement(loc);
	    break;

	case TOKdo:
	{   Statement *body;
	    Expression *condition;

	    nextToken();
	    body = parseStatement();
	    check(TOKwhile);
	    condition = parseParenExp();
	    parseOptionalSemi();
	    GC_LOG();
	    s = new(gc) DoStatement(loc, body, condition);
	    break;
	}

	case TOKfor:
	{
	    Statement *init;
	    Statement *body;

	    nextToken();
	    flags |= inForHeader;
	    check(TOKlparen);
	    if (token.value == TOKvar)
	    {
		init = parseStatement();
	    }
	    else
	    {	Expression *e;

		e = parseOptionalExpression(noIn);
		GC_LOG();
		init = e ? new(gc) ExpStatement(loc, e) : NULL;
	    }

	    if (token.value == TOKsemicolon)
	    {	Expression *condition;
		Expression *increment;

		nextToken();
		condition = parseOptionalExpression();
		check(TOKsemicolon);
		increment = parseOptionalExpression();
		check(TOKrparen);
		flags &= ~inForHeader;

		body = parseStatement();
		GC_LOG();
		s = new(gc) ForStatement(loc, init, condition, increment, body);
	    }
	    else if (token.value == TOKin)
	    {	Expression *in;
		VarStatement *vs;

		// Check that there's only one VarDeclaration
		// in init.
		if (init->st == VARSTATEMENT)
		{
		    vs = (VarStatement *)init;
		    if (vs->vardecls.dim != 1)
			error(ERR_TOO_MANY_IN_VARS, vs->vardecls.dim);
		}

		nextToken();
		in = parseExpression();
		check(TOKrparen);
		flags &= ~inForHeader;
		body = parseStatement();
		GC_LOG();
		s = new(gc) ForInStatement(loc, init, in, body);
	    }
	    else
	    {
		error(ERR_IN_EXPECTED, token.toDchars());
		s = NULL;
	    }
	    break;
	}

	case TOKwith:
	{   Expression *exp;
	    Statement *body;

	    nextToken();
	    exp = parseParenExp();
	    body = parseStatement();
	    GC_LOG();
	    s = new(gc) WithStatement(loc, exp, body);
	    break;
	}

	case TOKbreak:
	{   Identifier *ident;

	    nextToken();
	    if (token.sawLineTerminator && token.value != TOKsemicolon)
	    {	// Assume we saw a semicolon
		ident = NULL;
	    }
	    else
	    {
		if (token.value == TOKidentifier)
		{   ident = token.ident;
		    nextToken();
		}
		else
		    ident = NULL;
		parseOptionalSemi();
	    }
	    GC_LOG();
	    s = new(gc) BreakStatement(loc, ident);
	    break;
	}

	case TOKcontinue:
	{   Identifier *ident;

	    nextToken();
	    if (token.sawLineTerminator && token.value != TOKsemicolon)
	    {	// Assume we saw a semicolon
		ident = NULL;
	    }
	    else
	    {
		if (token.value == TOKidentifier)
		{   ident = token.ident;
		    nextToken();
		}
		else
		    ident = NULL;
		parseOptionalSemi();
	    }
	    GC_LOG();
	    s = new(gc) ContinueStatement(loc, ident);
	    break;
	}

	case TOKgoto:
	{   Identifier *ident;

	    nextToken();
	    if (token.value != TOKidentifier)
	    {	error(ERR_GOTO_LABEL_EXPECTED, token.toDchars());
		s = NULL;
		break;
	    }
	    ident = token.ident;
	    nextToken();
	    parseOptionalSemi();
	    GC_LOG();
	    s = new(gc) GotoStatement(loc, ident);
	    break;
	}

	case TOKreturn:
	{   Expression *exp;

	    nextToken();
	    if (token.sawLineTerminator && token.value != TOKsemicolon)
	    {	// Assume we saw a semicolon
		GC_LOG();
		s = new(gc) ReturnStatement(loc, NULL);
	    }
	    else
	    {	exp = parseOptionalExpression();
		parseOptionalSemi();
		GC_LOG();
		s = new(gc) ReturnStatement(loc, exp);
	    }
	    break;
	}

	case TOKthrow:
	{   Expression *exp;

	    nextToken();
	    exp = parseExpression();
	    parseOptionalSemi();
	    GC_LOG();
	    s = new(gc) ThrowStatement(loc, exp);
	    break;
	}

	case TOKtry:
	{   Statement *body;
	    Identifier *catchident;
	    Statement *catchbody;
	    Statement *finalbody;

	    nextToken();
	    body = parseStatement();
	    if (token.value == TOKcatch)
	    {
		nextToken();
		check(TOKlparen);
		catchident = NULL;
		if (token.value == TOKidentifier)
		    catchident = token.ident;
		check(TOKidentifier);
		check(TOKrparen);
		catchbody = parseStatement();
	    }
	    else
	    {	catchident = NULL;
		catchbody = NULL;
	    }

	    if (token.value == TOKfinally)
	    {	nextToken();
		finalbody = parseStatement();
	    }
	    else
		finalbody = NULL;

	    if (!catchbody && !finalbody)
	    {	error(ERR_TRY_CATCH_EXPECTED);
		s = NULL;
	    }
	    else
	    {
		GC_LOG();
		s = new(gc) TryStatement(loc, body, catchident, catchbody, finalbody);
	    }
	    break;
	}

	default:
	    error(ERR_STATEMENT_EXPECTED, token.toDchars());
	    nextToken();
	    s = NULL;
	    break;
    }

    //PRINTF("parseStatement() done\n");
    return s;
}



Expression *Parser::parseOptionalExpression(unsigned flags)
{
    Expression *e;

    if (token.value == TOKsemicolon || token.value == TOKrparen)
	e = NULL;
    else
	e = parseExpression(flags);
    return e;
}

// Follow ECMA 7.8.1 rules for inserting semicolons
void Parser::parseOptionalSemi()
{
    if (token.value != TOKeof &&
	token.value != TOKrbrace &&
	!(token.sawLineTerminator && (flags & inForHeader) == 0)
       )
	check(TOKsemicolon);
}

int Parser::check(enum TOK value)
{
    if (token.value != value)
    {
	errinfo.code = 1002;
	error(ERR_EXPECTED_GENERIC, token.toDchars(), Token::toDchars(value));
	return 0;
    }
    nextToken();
    return 1;
}

/********************************* Expression Parser ***************************/


Expression *Parser::parseParenExp()
{   Expression *e;

    check(TOKlparen);
    e = parseExpression();
    check(TOKrparen);
    return e;
}

Expression *Parser::parsePrimaryExp(int innew)
{   Expression *e;
    Loc loc;

    loc = currentline;
    switch (token.value)
    {
	case TOKthis:
	    GC_LOG();
	    e = new(gc) ThisExpression(loc);
	    nextToken();
	    break;

	case TOKnull:
	    GC_LOG();
	    e = new(gc) NullExpression(loc);
	    nextToken();
	    break;

	case TOKtrue:
	    GC_LOG();
	    e = new(gc) BooleanExpression(loc, 1);
	    nextToken();
	    break;

	case TOKfalse:
	    GC_LOG();
	    e = new(gc) BooleanExpression(loc, 0);
	    nextToken();
	    break;

	case TOKreal:
	    GC_LOG();
	    e = new(gc) RealExpression(loc, token.realvalue);
	    nextToken();
	    break;

	case TOKstring:
	    GC_LOG();
	    e = new(gc) StringExpression(loc, token.string);
	    token.string = NULL;	// release to gc
	    nextToken();
	    break;

	case TOKregexp:
	    GC_LOG();
	    e = new(gc) RegExpLiteral(loc, token.string);
	    token.string = NULL;	// release to gc
	    nextToken();
	    break;

	case TOKidentifier:
	    GC_LOG();
	    e = new(gc) IdentifierExpression(loc, token.ident);
	    token.ident = NULL;		// release to gc
	    nextToken();
	    break;

	case TOKlparen:
	    e = parseParenExp();
	    break;

	case TOKlbracket:
	    e = parseArrayLiteral();
	    break;

	case TOKlbrace:
	    if (flags & initial)
	    {
		error(ERR_OBJ_LITERAL_IN_INITIALIZER);
		nextToken();
		return NULL;
	    }
	    e = parseObjectLiteral();
	    break;

	case TOKfunction:
//	    if (flags & initial)
//		goto Lerror;
	    e = parseFunctionLiteral();
	    break;

	case TOKnew:
	{   Expression *newarg;
	    Array *arguments;

	    nextToken();
	    newarg = parsePrimaryExp(1);
	    arguments = parseArguments();
	    GC_LOG();
	    e = new(gc) NewExp(loc, newarg, arguments);
	    break;
	}

	default:
//	Lerror:
	    error(ERR_EXPECTED_EXPRESSION, token.toDchars());
	    nextToken();
	    return NULL;
    }
    return parsePostExp(e, innew);
}

Array *Parser::parseArguments()
{
    Array *arguments = NULL;

    if (token.value == TOKlparen)
    {
	GC_LOG();
	arguments = new(gc) Array();
	nextToken();
	if (token.value != TOKrparen)
	{
	    for (;;)
	    {	Expression *arg;

		arg = parseAssignExp();
		arguments->push(arg);
		if (token.value == TOKrparen)
		    break;
		if (!check(TOKcomma))
		    break;
	    }
	}
	nextToken();
    }
    return arguments;
}

Expression *Parser::parseArrayLiteral()
{
    Expression *e;
    Array *elements;
    Loc loc;

    //PRINTF("parseArrayLiteral()\n");
    loc = currentline;
    GC_LOG();
    elements = new(gc) Array();
    check(TOKlbracket);
    if (token.value != TOKrbracket)
    {
	for (;;)
	{
	    if (token.value == TOKcomma)
		// Allow things like [1,2,,,3,]
		// Like Explorer 4, and unlike Netscape, the
		// trailing , indicates another NULL element.
		elements->push(NULL);
	    else if (token.value == TOKrbracket)
	    {
		elements->push(NULL);
		break;
	    }
	    else
	    {	e = parseAssignExp();
		elements->push(e);
		if (token.value != TOKcomma)
		    break;
	    }
	    nextToken();
	}
    }
    check(TOKrbracket);
    GC_LOG();
    e = new(gc) ArrayLiteral(loc, elements);
    return e;
}

Expression *Parser::parseObjectLiteral()
{
    Expression *e;
    Array *fields;
    Loc loc;

    //PRINTF("parseObjectLiteral()\n");
    loc = currentline;
    GC_LOG();
    fields = new(gc) Array();
    check(TOKlbrace);
    if (token.value == TOKrbrace)
	nextToken();
    else
    {
	for (;;)
	{   Field *f;
	    Identifier *ident;
	    Expression *e;

	    if (token.value != TOKidentifier)
	    {	error(ERR_EXPECTED_IDENTIFIER);
		break;
	    }
	    ident = token.ident;
	    nextToken();
	    check(TOKcolon);
	    e = parseAssignExp();
	    GC_LOG();
	    f = new(gc) Field(ident,e);
	    fields->push(f);
	    if (token.value != TOKcomma)
		break;
	    nextToken();
	}
	check(TOKrbrace);
    }
    GC_LOG();
    e = new(gc) ObjectLiteral(loc, fields);
    return e;
}

Expression *Parser::parseFunctionLiteral()
{   FunctionDefinition *f;
    Loc loc;

    loc = currentline;
    f = (FunctionDefinition *)parseFunction(1);
    GC_LOG();
    return new(gc) FunctionLiteral(loc, f);
}

Expression *Parser::parsePostExp(Expression *e, int innew)
{   Loc loc;

    for (;;)
    {
	loc = currentline;
	//loc = (Loc)token.ptr;
	switch (token.value)
	{
	    case TOKdot:
		nextToken();
		if (token.value == TOKidentifier)
		{   GC_LOG();
		    e = new(gc) DotExp(loc, e, token.ident);
		}
		else
		{
		    error(ERR_EXPECTED_IDENTIFIER_2PARAM, DTEXT("."), token.toDchars());
		    return e;
		}
		break;

	    case TOKplusplus:
		if (token.sawLineTerminator && !(flags & inForHeader))
		    goto Linsert;
		GC_LOG();
		e = new(gc) PostIncExp(loc, e);
		break;

	    case TOKminusminus:
		if (token.sawLineTerminator && !(flags & inForHeader))
		{
		Linsert:
		    // insert automatic semicolon
		    insertSemicolon(token.sawLineTerminator);
		    return e;
		}
		GC_LOG();
		e = new(gc) PostDecExp(loc, e);
		break;

	    case TOKlparen:
	    {	// function call
		Array *arguments;

		if (innew)
		    return e;
		arguments = parseArguments();
		GC_LOG();
		e = new(gc) CallExp(loc, e, arguments);
		continue;
	    }

	    case TOKlbracket:
	    {	// array dereference
		Expression *index;

		nextToken();
		index = parseExpression();
		check(TOKrbracket);
		GC_LOG();
		e = new(gc) ArrayExp(loc, e, index);
		continue;
	    }

	    default:
		return e;
	}
	nextToken();
    }
}

Expression *Parser::parseUnaryExp()
{   Expression *e;
    Loc loc;

    loc = currentline;
    switch (token.value)
    {
	case TOKplusplus:
	    nextToken();
	    e = parseUnaryExp();
	    GC_LOG();
	    e = new(gc) PreIncExp(loc, e);
	    break;

	case TOKminusminus:
	    nextToken();
	    e = parseUnaryExp();
	    GC_LOG();
	    e = new(gc) PreDecExp(loc, e);
	    break;

	case TOKminus:
	    nextToken();
	    e = parseUnaryExp();
	    GC_LOG();
	    e = new(gc) NegExp(loc, e);
	    break;

	case TOKplus:
	    nextToken();
	    e = parseUnaryExp();
	    GC_LOG();
	    e = new(gc) PosExp(loc, e);
	    break;

	case TOKnot:
	    nextToken();
	    e = parseUnaryExp();
	    GC_LOG();
	    e = new(gc) NotExp(loc, e);
	    break;

	case TOKtilde:
	    nextToken();
	    e = parseUnaryExp();
	    GC_LOG();
	    e = new(gc) ComExp(loc, e);
	    break;

	case TOKdelete:
	    nextToken();
	    e = parsePrimaryExp(0);
	    GC_LOG();
	    e = new(gc) DeleteExp(loc, e);
	    break;

	case TOKtypeof:
	    nextToken();
	    e = parseUnaryExp();
	    GC_LOG();
	    e = new(gc) TypeofExp(loc, e);
	    break;

	case TOKvoid:
	    nextToken();
	    e = parseUnaryExp();
	    GC_LOG();
	    e = new(gc) VoidExp(loc, e);
	    break;

	default:
	    e = parsePrimaryExp(0);
	    break;
    }
    return e;
}

Expression *Parser::parseMulExp()
{   Expression *e;
    Expression *e2;
    Loc loc;

    loc = currentline;
    e = parseUnaryExp();
    for (;;)
    {
	switch (token.value)
	{
	    case TOKmultiply:
		nextToken();
		e2 = parseUnaryExp();
		GC_LOG();
		e = new(gc) MulExp(loc, e, e2);
		continue;

	    case TOKregexp:
		// Rescan as if it was a "/"
		rescan();
	    case TOKdivide:
		nextToken();
		e2 = parseUnaryExp();
		GC_LOG();
		e = new(gc) DivExp(loc, e, e2);
		continue;

	    case TOKpercent:
		nextToken();
		e2 = parseUnaryExp();
		GC_LOG();
		e = new(gc) ModExp(loc, e, e2);
		continue;

	    default:
		break;
	}
	break;
    }
    return e;
}

Expression *Parser::parseAddExp()
{   Expression *e;
    Expression *e2;
    Loc loc;

    loc = currentline;
    e = parseMulExp();
    for (;;)
    {
	switch (token.value)
	{
	    case TOKplus:
		nextToken();
		e2 = parseMulExp();
		GC_LOG();
		e = new(gc) AddExp(loc, e, e2);
		continue;

	    case TOKminus:
		nextToken();
		e2 = parseMulExp();
		GC_LOG();
		e = new(gc) MinExp(loc, e, e2);
		continue;

	    default:
		break;
	}
	break;
    }
    return e;
}

Expression *Parser::parseShiftExp()
{   Expression *e;
    Expression *e2;
    Loc loc;

    loc = currentline;
    e = parseAddExp();
    for (;;)
    {
	switch (token.value)
	{
	    case TOKshiftleft:
		nextToken();
		e2 = parseAddExp();
		GC_LOG();
		e = new(gc) ShlExp(loc, e, e2);
		continue;

	    case TOKshiftright:
		nextToken();
		e2 = parseAddExp();
		GC_LOG();
		e = new(gc) ShrExp(loc, e, e2);
		continue;

	    case TOKushiftright:
		nextToken();
		e2 = parseAddExp();
		GC_LOG();
		e = new(gc) UshrExp(loc, e, e2);
		continue;

	    default:
		break;
	}
	break;
    }
    return e;
}

Expression *Parser::parseRelExp()
{   Expression *e;
    Expression *e2;
    Loc loc;

    loc = currentline;
    e = parseShiftExp();
    for (;;)
    {
	switch (token.value)
	{
	    case TOKless:
	        nextToken();
		e2 = parseShiftExp();
		GC_LOG();
		e = new(gc) LessExp(loc, e, e2);
		continue;

	    case TOKlessequal:
		nextToken();
		e2 = parseShiftExp();
		GC_LOG();
		e = new(gc) LessEqualExp(loc, e, e2);
		continue;

	    case TOKgreater:
		nextToken();
		e2 = parseShiftExp();
		GC_LOG();
		e = new(gc) GreaterExp(loc, e, e2);
		continue;

	    case TOKgreaterequal:
		nextToken();
		e2 = parseShiftExp();
		GC_LOG();
		e = new(gc) GreaterEqualExp(loc, e, e2);
		continue;

	    case TOKinstanceof:
		nextToken();
		e2 = parseShiftExp();
		GC_LOG();
		e = new(gc) InstanceofExp(loc, e, e2);
		continue;

	    case TOKin:
		if (flags & noIn)
		    break;		// disallow
		nextToken();
		e2 = parseShiftExp();
		GC_LOG();
		e = new(gc) InExp(loc, e, e2);
		continue;

	    default:
		break;
	}
	break;
    }
    return e;
}

Expression *Parser::parseEqualExp()
{   Expression *e;
    Expression *e2;
    Loc loc;

    loc = currentline;
    e = parseRelExp();
    for (;;)
    {
	switch (token.value)
	{
	    case TOKequal:
		nextToken();
		e2 = parseRelExp();
		GC_LOG();
		e = new(gc) EqualExp(loc, e, e2);
		continue;

	    case TOKnotequal:
		nextToken();
		e2 = parseRelExp();
		GC_LOG();
		e = new(gc) NotEqualExp(loc, e, e2);
		continue;

	    case TOKidentity:
		nextToken();
		e2 = parseRelExp();
		GC_LOG();
		e = new(gc) IdentityExp(loc, e, e2);
		continue;

	    case TOKnonidentity:
		nextToken();
		e2 = parseRelExp();
		GC_LOG();
		e = new(gc) NonIdentityExp(loc, e, e2);
		continue;

	    default:
		break;
	}
	break;
    }
    return e;
}

Expression *Parser::parseAndExp()
{   Expression *e;
    Expression *e2;
    Loc loc;

    loc = currentline;
    e = parseEqualExp();
    while (token.value == TOKand)
    {
	nextToken();
	e2 = parseEqualExp();
	GC_LOG();
	e = new(gc) AndExp(loc, e, e2);
    }
    return e;
}

Expression *Parser::parseXorExp()
{   Expression *e;
    Expression *e2;
    Loc loc;

    loc = currentline;
    e = parseAndExp();
    while (token.value == TOKxor)
    {
	nextToken();
	e2 = parseAndExp();
	GC_LOG();
	e = new(gc) XorExp(loc, e, e2);
    }
    return e;
}

Expression *Parser::parseOrExp()
{   Expression *e;
    Expression *e2;
    Loc loc;

    loc = currentline;
    e = parseXorExp();
    while (token.value == TOKor)
    {
	nextToken();
	e2 = parseXorExp();
	GC_LOG();
	e = new(gc) OrExp(loc, e, e2);
    }
    return e;
}

Expression *Parser::parseAndAndExp()
{   Expression *e;
    Expression *e2;
    Loc loc;

    loc = currentline;
    e = parseOrExp();
    while (token.value == TOKandand)
    {
	nextToken();
	e2 = parseOrExp();
	GC_LOG();
	e = new(gc) AndAndExp(loc, e, e2);
    }
    return e;
}

Expression *Parser::parseOrOrExp()
{   Expression *e;
    Expression *e2;
    Loc loc;

    loc = currentline;
    e = parseAndAndExp();
    while (token.value == TOKoror)
    {
	nextToken();
	e2 = parseAndAndExp();
	GC_LOG();
	e = new(gc) OrOrExp(loc, e, e2);
    }
    return e;
}

Expression *Parser::parseCondExp()
{   Expression *e;
    Expression *e1;
    Expression *e2;
    Loc loc;

    loc = currentline;
    e = parseOrOrExp();
    if (token.value == TOKquestion)
    {
	nextToken();
	e1 = parseAssignExp();
	check(TOKcolon);
	e2 = parseAssignExp();
	GC_LOG();
	e = new(gc) CondExp(loc, e, e1, e2);
    }
    return e;
}

Expression *Parser::parseAssignExp()
{   Expression *e;
    Expression *e2;
    Loc loc;

    loc = currentline;
    e = parseCondExp();
    for (;;)
    {
	switch (token.value)
	{

#define X(tok,ector)				\
	    case tok:				\
		nextToken();			\
		e2 = parseAssignExp();		\
		GC_LOG();			\
		e = new(gc) ector(loc, e, e2);	\
		continue;

	    X(TOKassign,             AssignExp);
	    X(TOKplusass,         AddAssignExp);
	    X(TOKminusass,        MinAssignExp);
	    X(TOKmultiplyass,     MulAssignExp);
	    X(TOKdivideass,       DivAssignExp);
	    X(TOKpercentass,      ModAssignExp);
	    X(TOKandass,          AndAssignExp);
	    X(TOKorass,            OrAssignExp);
	    X(TOKxorass,          XorAssignExp);
	    X(TOKshiftleftass,    ShlAssignExp);
	    X(TOKshiftrightass,   ShrAssignExp);
	    X(TOKushiftrightass, UshrAssignExp);

#undef X
	    default:
		break;
	}
	break;
    }
    return e;
}

Expression *Parser::parseExpression(unsigned flags)
{   Expression *e;
    Expression *e2;
    Loc loc;
    unsigned flags_save;

    //PRINTF("Parser::parseExpression()\n");
    flags_save = this->flags;
    this->flags = flags;
    loc = currentline;
    e = parseAssignExp();
    while (token.value == TOKcomma)
    {
	nextToken();
	e2 = parseAssignExp();
	GC_LOG();
	e = new(gc) CommaExp(loc, e, e2);
    }
    this->flags = flags_save;
    return e;
}


/********************************* ***************************/

