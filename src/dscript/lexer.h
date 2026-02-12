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

#ifndef LEXER_H
#define LEXER_H

//#pragma once

#include "root.h"
#include "port.h"
#include "lstring.h"
#include "mem.h"

#include "dscript.h"

struct StringTable;
struct Identifier;

/* Tokens:
	(	)
	[	]
	{	}
	<	>	<=	>=	==	!=
	===     !==
	<<	>>	<<=	>>=	>>>	>>>=
	+	-	+=	-=
	*	/	%	*=	/=	%=
	&	| 	^	&=	|=	^=
	=	!	~
	++	--
	.	:	,
	?	&&	||
 */

enum TOK
{
	TOKreserved,

	// Other
	TOKlparen,	TOKrparen,
	TOKlbracket,	TOKrbracket,
	TOKlbrace,	TOKrbrace,
	TOKcolon,	TOKneg,
	TOKpos,
	TOKsemicolon,	TOKeof,
	TOKarray,	TOKcall,
	TOKarraylit,	TOKobjectlit,
	TOKcomma,	TOKassert,

	// Operators
	TOKless,	TOKgreater,
	TOKlessequal,	TOKgreaterequal,
	TOKequal,	TOKnotequal,
	TOKidentity,	TOKnonidentity,
	TOKshiftleft,	TOKshiftright,
	TOKshiftleftass, TOKshiftrightass,
	TOKushiftright,	TOKushiftrightass,
	TOKplus,	TOKminus,	TOKplusass,	TOKminusass,
	TOKmultiply,	TOKdivide,	TOKpercent,
	TOKmultiplyass,	TOKdivideass,	TOKpercentass,
	TOKand,		TOKor,		TOKxor,
	TOKandass,	TOKorass,	TOKxorass,
	TOKassign,	TOKnot,		TOKtilde,
	TOKplusplus,	TOKminusminus,
	TOKdot,
	TOKquestion,	TOKandand,	TOKoror,

	// Leaf operators
	TOKnumber,	TOKidentifier,	TOKstring,
	TOKregexp,	TOKreal,

	// Keywords
	TOKbreak,	TOKcase,	TOKcontinue,
	TOKdefault,	TOKdelete,	TOKdo,
	TOKelse,	TOKexport,	TOKfalse,
	TOKfor,		TOKfunction,	TOKif,
	TOKimport,	TOKin,		TOKnew,
	TOKnull,	TOKreturn,	TOKswitch,
	TOKthis,	TOKtrue,	TOKtypeof,
	TOKvar,		TOKvoid,	TOKwhile,
	TOKwith,

	// Reserved for ECMA extensions
	TOKcatch,	TOKclass,
	TOKconst,	TOKdebugger,
	TOKenum,	TOKextends,
	TOKfinally,	TOKsuper,
	TOKthrow,	TOKtry,

	// Java keywords reserved for unknown reasons
	TOKabstract,	TOKboolean,
	TOKbyte,	TOKchar,
	TOKdouble,	TOKfinal,
	TOKfloat,	TOKgoto,
	TOKimplements,	TOKinstanceof,
	TOKint,		TOKinterface,
	TOKlong,	TOKnative,
	TOKpackage,	TOKprivate,
	TOKprotected,	TOKpublic,
	TOKshort,	TOKstatic,
	TOKsynchronized, TOKthrows,
	TOKtransient,

	TOKMAX
};

struct Lexer;

struct Token
{
    Token *next;
    dchar *ptr;		// pointer to first character of this token within buffer
    unsigned linnum;
    enum TOK value;
    // Where we saw the last line terminator
    dchar *sawLineTerminator;
    union
    {
	number_t intvalue;
	real_t realvalue;
	Lstring *string;
	Identifier *ident;
    };

    static dchar *tochars[TOKMAX];
    static void *operator new(size_t sz, Lexer *lex);

    void print();
    dchar *toDchars();
    static dchar *toDchars(enum TOK);
};

struct Lexer : Mem
{
    StringTable *stringtable;
    Token *freelist;

    const char *sourcename;	// for error message strings

    dchar *base;		// pointer to start of buffer
    dchar *end;			// past end of buffer
    dchar *p;			// current character
    unsigned currentline;
    Token token;
    OutBuffer stringbuffer;
    int useStringtable;		// use for Identifiers

    ErrInfo errinfo;		// syntax error information

    Lexer(const char *sourcename, dchar *base, unsigned length, int useStringtable);
    ~Lexer();

    static TOK isKeyword(dchar *s, unsigned len);
    static void initKeywords();
    TOK nextToken();
    void insertSemicolon(dchar *loc);
    void rescan();
    void scan(Token *t);
    Token *peek(Token *t);
    unsigned escapeSequence();
    Lstring *string(unsigned quote);
    Lstring *regexp();
    unsigned unicode();
    TOK number(Token *t);
    void error(int, ...);

    static dchar *locToSrcline(dchar *src, Loc loc);
};

#endif

