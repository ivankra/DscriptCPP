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


/* Lexical Analyzer
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>

#include "root.h"
#include "mem.h"
#include "dchar.h"
#include "mutex.h"
#include "stringtable.h"
#include "identifier.h"
#include "lexer.h"
#include "text.h"

#define isoctal(c)	('0' <= (c) && (c) <= '7')
#define isasciidigit(c) ('0' <= (c) && (c) <= '9')
#define isasciilower(c) ('a' <= (c) && (c) <= 'z')
#define isasciiupper(c) ('A' <= (c) && (c) <= 'Z')
#define ishex(c)	(isasciidigit(c) || ('a' <= (c) && (c) <= 'f') || ('A' <= (c) && (c) <= 'F'))

#define STRINGTABLE	1	// 1 = use stringtables

// These should get constructed first, so place them lexically first
//static Mutex freelist_mutex;
//static Mutex stringtable_mutex;

//Token *Lexer::freelist = NULL;

/******************************************************/

dchar *Token::tochars[TOKMAX];

void *Token::operator new(size_t size, Lexer *lex)
{   Token *t;

    if (lex->freelist)
    {
	//freelist_mutex.acquire();
	if (lex->freelist)
	{
	    t = lex->freelist;
	    lex->freelist = t->next;
	    //freelist_mutex.release();
	    return t;
	}
	//freelist_mutex.release();
    }

    return lex->malloc(size);
}

void Token::print()
{
#if defined UNICODE
    WPRINTF(L"%ls\n", toDchars());
#else
    PRINTF("%s\n", toDchars());
#endif
}

dchar *Token::toDchars()
{   dchar *p;
    // buffer[] is only for debugging, so no matter that it's static and
    // not threadsafe
    static dchar buffer[3 + 3 * sizeof(value) + 1];

    p = buffer;
    switch (value)
    {
	case TOKnumber:
#if defined UNICODE
	    SWPRINTF(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%lld", intvalue);
#else
	    sprintf(buffer, "%lld", intvalue);
#endif
	    break;

	case TOKreal:
#if defined UNICODE
	{
	    long l = (long)realvalue;
	    if (l == realvalue)
		SWPRINTF(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%ld", l);
	    else
		SWPRINTF(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%g", realvalue);
	}
#else
	    sprintf(buffer, "%g", realvalue);
#endif
	    break;

	case TOKstring:
	case TOKregexp:
	{   OutBuffer buf;

	    buf.writedchar('"');
	    buf.writedstring(string->toDchars());
	    buf.writedchar('"');
	    buf.writedchar(0);
	    p = (dchar *)buf.data;
	    buf.data = NULL;
	    break;
	}
	case TOKidentifier:
	    p = ident->toDchars();
	    break;

	default:
	    p = toDchars(value);
	    break;
    }
    return p;
}

dchar *Token::toDchars(enum TOK value)
{   dchar *p;
    static dchar buffer[3 + 3 * sizeof(value) + 1];

    p = tochars[value];
    if (!p)
    {
#if defined UNICODE
	SWPRINTF(buffer, sizeof(buffer) / sizeof(buffer[0]), L"TOK%d", value);
#else
	sprintf(buffer,"TOK%d",value);
#endif
	p = buffer;
    }
    return p;
}

/*******************************************************************/

Lexer::Lexer(const char *sourcename, dchar *base, unsigned length, int useStringtable)
{
    //PRINTF("Lexer::Lexer(%p,%d)\n",base,length);
    memset(&token,0,sizeof(token));
    this->useStringtable = useStringtable;
    this->sourcename = sourcename;
    this->base = base;
    this->end  = base + length;
    p = base;
    currentline = 1;
    freelist = NULL;

    // Share a per-thread StringTable for keywords and Identifier's
    ThreadContext *tc = ThreadContext::getThreadContext();
    stringtable = tc->stringtable;
    if (!stringtable)
    {
	GC_LOG();
	tc->stringtable = new(&tc->mem) StringTable(1009);
	stringtable = tc->stringtable;
	initKeywords();
    }
    gc = tc->mem.gc;
    assert(gc);
}

Lexer::~Lexer()
{
    //WPRINTF(L"~Lexer()\n");
#if 0
    if (stringtable)
    {	delete stringtable;
	stringtable = NULL;
    }
#endif
    freelist = NULL;
    sourcename = NULL;
    base = NULL;
    end = NULL;
    p = NULL;
}

void Lexer::error(int msgnum, ...)
{
    unsigned linnum = 1;
    dchar *s;
    dchar *slinestart;
    dchar *slineend;
    OutBuffer buf;

    //FuncLog funclog(L"Lexer.error()");
    //WPRINTF(L"TEXT START ------------\n%ls\nTEXT END ------------------\n", base);

    // Find the beginning of the line
    slinestart = base;
    for (s = base; s != p; s = Dchar::inc(s))
    {
	if (*s == '\n')
	{   linnum++;
	    slinestart = s + 1;
	}
    }

    // Find the end of the line
    for (;;)
    {
	switch (*s)
	{
	    case '\n':
	    case 0:
	    case 0x1A:
		break;
	    default:
		s = Dchar::inc(s);
		continue;
	}
	break;
    }
    slineend = s;

    buf.reset();
#if defined UNICODE
    buf.writedstring(sourcename);
    buf.printf(L"(%d) : Error: ", linnum);
#else
    buf.printf("%s(%d) : Error: ", sourcename, linnum);
#endif

    va_list ap;
    va_start(ap, msgnum);
    dchar *format = errmsg(msgnum);
    buf.vprintf(format, ap);
    va_end(ap);
    buf.writedchar(0);

    if (!errinfo.message)
    {	unsigned len;

	errinfo.message = (dchar *)buf.data;
	errinfo.linnum = linnum;
	errinfo.charpos = p - slinestart;

	len = slineend - slinestart;
	errinfo.srcline = (dchar *)this->malloc((len + 1) * sizeof(dchar));
	memcpy(errinfo.srcline, slinestart, len * sizeof(dchar));
	errinfo.srcline[len] = 0;
    }
    buf.data = NULL;

    // Consume input until the end
    while (*p != 0x1A && *p != 0)
	p++;
    token.next = NULL;		// dump any lookahead

#if 0
#if defined UNICODE
    WPRINTF(L"%ls\n", errinfo.message);
#else
    PRINTF("%s\n", errinfo.message);
#endif
    fflush(stdout);

    exit(EXIT_FAILURE);
#endif
}

/************************************************
 * Given source text, convert loc to a string for the corresponding line.
 */

dchar *Lexer::locToSrcline(dchar *src, Loc loc)
{
    dchar *slinestart;
    dchar *slineend;
    dchar *s;
    unsigned linnum = 1;
    unsigned len;

    if (!src)
	return NULL;
    slinestart = src;
    for (s = src; ; s = Dchar::inc(s))
    {
	switch (*s)
	{
	    case '\n':
		if (linnum == loc)
		{
		    slineend = s;
		    break;
		}
		slinestart = s + 1;
		linnum++;
		continue;

	    case 0:
	    case 0x1A:
		slineend = s;
		break;

	    default:
		continue;
	}
	break;
    }

    // Remove trailing \r's
    while (slinestart < slineend && slineend[-1] == '\r')
	--slineend;

    len = slineend - slinestart;
    s = (dchar *)mem.malloc_atomic((len + 1) * sizeof(dchar));
    s[len] = 0;
    return (dchar *)memcpy(s, slinestart, len * sizeof(dchar));
}


TOK Lexer::nextToken()
{   Token *t;

    if (token.next)
    {
	t = token.next;
	memcpy(&token,t,sizeof(Token));
	//freelist_mutex.acquire();
	t->next = freelist;
	freelist = t;
	//freelist_mutex.release();
    }
    else
    {
	scan(&token);
    }
    //token.print();
    return token.value;
}

Token *Lexer::peek(Token *ct)
{   Token *t;

    if (ct->next)
	t = ct->next;
    else
    {
	t = new(this) Token();
	scan(t);
	t->next = NULL;
	ct->next = t;
    }
    return t;
}

void Lexer::insertSemicolon(dchar *loc)
{
    // Push current token back into the input, and
    // create a new current token that is a semicolon
    Token *t;

    t = new(this) Token();
    memcpy(t,&token,sizeof(Token));
    token.next = t;
    token.value = TOKsemicolon;
    token.ptr = loc;
    token.sawLineTerminator = NULL;
}

/**********************************
 * Horrible kludge to support disambiguating TOKregexp from TOKdivide.
 * The idea is, if we are looking for a TOKdivide, and find instead
 * a TOKregexp, we back up and rescan.
 */

void Lexer::rescan()
{
    token.next = NULL;	// no lookahead
			// should put on freelist
    p = token.ptr + 1;
}


/****************************
 * Turn next token in buffer into a token.
 */

void Lexer::scan(Token *t)
{   unsigned c;
#if STRINGTABLE
    StringValue *sv;
#endif
    //Identifier *id;

    t->sawLineTerminator = NULL;
    for (;;)
    {
	t->ptr = p;
	//t->linnum = currentline;
	//WPRINTF(L"p = %x, *p = x%02x, '%c'\n",p,*p,*p);
	switch (Dchar::get(p))
	{
	    case 0:
	    case 0x1A:
		t->value = TOKeof;			// end of file
		return;

	    case ' ':
	    case '\t':
	    case '\v':
	    case '\f':
	    case 0xA0:				// no-break space
		p = Dchar::inc(p);
		continue;			// skip white space

	    case '\n':				// line terminator
		currentline++;
	    case '\r':
		t->sawLineTerminator = p;
		p = Dchar::inc(p);
		continue;

	    case '"':
	    case '\'':
		t->string = string(Dchar::get(p));
		t->value = TOKstring;
		return;

	    case '0':  	case '1':   case '2':   case '3':   case '4':
	    case '5':  	case '6':   case '7':   case '8':   case '9':
		t->value = number(t);
		return;

	    case 'a':  	case 'b':   case 'c':   case 'd':   case 'e':
	    case 'f':  	case 'g':   case 'h':   case 'i':   case 'j':
	    case 'k':  	case 'l':   case 'm':   case 'n':   case 'o':
	    case 'p':  	case 'q':   case 'r':   case 's':   case 't':
	    case 'u':  	case 'v':   case 'w':   case 'x':   case 'y':
	    case 'z':
	    case 'A':  	case 'B':   case 'C':   case 'D':   case 'E':
	    case 'F':  	case 'G':   case 'H':   case 'I':   case 'J':
	    case 'K':  	case 'L':   case 'M':   case 'N':   case 'O':
	    case 'P':  	case 'Q':   case 'R':   case 'S':   case 'T':
	    case 'U':  	case 'V':   case 'W':   case 'X':   case 'Y':
	    case 'Z':
	    case '_':
	    case '$':
	    Lidentifier:
	    {
		do
		{
		    p = Dchar::inc(p);
		    c = Dchar::get(p);
		    if (c == '\\' && p[1] == 'u')
		    {
		Lidentifier2:
			stringbuffer.reset();
			stringbuffer.write(t->ptr, (p - t->ptr) * sizeof(dchar));
			p++;
			stringbuffer.writedchar(unicode());
			for (;;)
			{
			    c = Dchar::get(p);
			    if (c == '\\' && p[1] == 'u')
			    {
				p++;
				stringbuffer.writedchar(unicode());
			    }
			    else if (isalnum(c) || c == '_' || c == '$')
			    {	stringbuffer.writedchar(c);
				p = Dchar::inc(p);
			    }
			    else
			    {
				t->value = isKeyword((dchar *)stringbuffer.data, stringbuffer.offset / sizeof(dchar));
				if (t->value)
				    return;
				if (useStringtable)
				{
				    sv = stringtable->update((dchar *)stringbuffer.data, stringbuffer.offset / sizeof(dchar));
				    t->ident = (Identifier *)(&sv->lstring);
				}
				else
				    t->ident = (Identifier *)Lstring::ctor((dchar *)stringbuffer.data, stringbuffer.offset / sizeof(dchar));
				t->value = TOKidentifier;
				return;
			    }
			}
		    }
		} while (isalnum(c) || c == '_' || c == '$');   // This should be isalnum -- any Unicode letter is allowed
		t->value = isKeyword((dchar *)t->ptr, p - t->ptr);
		if (t->value)
		    return;
		if (useStringtable)
		{
		    sv = stringtable->update((dchar *)t->ptr, p - t->ptr);
		    t->ident = (Identifier *)(&sv->lstring);
		}
		else
		    t->ident = (Identifier *)Lstring::ctor((dchar *)t->ptr, p - t->ptr);
		t->value = TOKidentifier;
		return;
	    }

	    case '/':
		p = Dchar::inc(p);
		c = Dchar::get(p);
		if (c == '=')
		{
		    p = Dchar::inc(p);
		    t->value = TOKdivideass;
		    return;
		}
		else if (c == '*')
		{
		    p = Dchar::inc(p);
		    for ( ; ; p = Dchar::inc(p))
		    {
			c = Dchar::get(p);
		Lcomment:
			switch (c)
			{
			    case '*':
				p = Dchar::inc(p);
				c = Dchar::get(p);
				if (c == '/')
				{
				    p = Dchar::inc(p);
				    break;
				}
				goto Lcomment;

			    case '\n':
				currentline++;
			    case '\r':
				t->sawLineTerminator = p;
				continue;

			    case 0:
			    case 0x1A:
				error(ERR_BAD_C_COMMENT);
				t->value = TOKeof;
				return;

			    default:
				continue;
			}
			break;
		    }
		    continue;
		}
		else if (c == '/')
		{
		    for (;;)
		    {
			p = Dchar::inc(p);
			switch (*p)
			{
			    case '\n':
				currentline++;
			    case '\r':
				t->sawLineTerminator = p;
				break;

			    case 0:
			    case 0x1A:			// end of file
				t->value = TOKeof;
				return;

			    default:
				continue;
			}
			break;
		    }
		    p = Dchar::inc(p);
		    continue;
		}
		else if ((t->string = regexp()) != NULL)
		    t->value = TOKregexp;
		else
		    t->value = TOKdivide;
		return;

	    case '.':
		dchar *q;
		q = Dchar::inc(p);
		c = Dchar::get(q);
		if (Dchar::isDigit((dchar)c))
		    t->value = number(t);
		else
		{   t->value = TOKdot;
		    p = q;
		}
		return;

	    case '&':
		p = Dchar::inc(p);
		c = Dchar::get(p);
		if (c == '=')
		{
		    p = Dchar::inc(p);
		    t->value = TOKandass;
		}
		else if (c == '&')
		{
		    p = Dchar::inc(p);
		    t->value = TOKandand;
		}
		else
		    t->value = TOKand;
		return;

	    case '|':
		p = Dchar::inc(p);
		c = Dchar::get(p);
		if (c == '=')
		{
		    p = Dchar::inc(p);
		    t->value = TOKorass;
		}
		else if (c == '|')
		{
		    p = Dchar::inc(p);
		    t->value = TOKoror;
		}
		else
		    t->value = TOKor;
		return;

	    case '-':
		p = Dchar::inc(p);
		c = Dchar::get(p);
		if (c == '=')
		{
		    p = Dchar::inc(p);
		    t->value = TOKminusass;
		}
		else if (c == '-')
		{
		    p = Dchar::inc(p);

		    // If the last token in the file is --> then
		    // treat it as EOF. This is to accept broken
		    // scripts that forgot to protect the closing -->
		    // with a // comment.
		    if (*p == '>')
		    {
			// Scan ahead to see if it's the last token
			dchar *q;

			q = p;
			for (;;)
			{
			    switch (*++q)
			    {
				case 0:
				case 0x1A:
				    t->value = TOKeof;
				    p = q;
				    return;

				case ' ':
				case '\t':
				case '\v':
				case '\f':
				case '\n':
				case '\r':
				case 0xA0:		// no-break space
				    continue;
			    }
			    break;
			}
		    }
		    t->value = TOKminusminus;
		}
		else
		    t->value = TOKminus;
		return;

	    case '+':
		p = Dchar::inc(p);
		c = Dchar::get(p);
		if (c == '=')
		{   p = Dchar::inc(p);
		    t->value = TOKplusass;
		}
		else if (c == '+')
		{   p = Dchar::inc(p);
		    t->value = TOKplusplus;
		}
		else
		    t->value = TOKplus;
		return;

	    case '<':
		p = Dchar::inc(p);
		c = Dchar::get(p);
		if (c == '=')
		{   p = Dchar::inc(p);
		    t->value = TOKlessequal;
		}
		else if (c == '<')
		{   p = Dchar::inc(p);
		    c = Dchar::get(p);
		    if (c == '=')
		    {   p = Dchar::inc(p);
			t->value = TOKshiftleftass;
		    }
		    else
			t->value = TOKshiftleft;
		}
		else if (c == '!' && p[1] == '-' && p[2] == '-')
		{   // Special comment to end of line
		    p += 2;
		    for (;;)
		    {
			p = Dchar::inc(p);
			switch (*p)
			{
			    case '\n':
				currentline++;
			    case '\r':
				t->sawLineTerminator = p;
				break;

			    case 0:
			    case 0x1A:			// end of file
				error(ERR_BAD_HTML_COMMENT);
				t->value = TOKeof;
				return;

			    default:
				continue;
			}
			break;
		    }
		    p = Dchar::inc(p);
		    continue;
		}
		else
		    t->value = TOKless;
		return;

	    case '>':
		p = Dchar::inc(p);
		c = Dchar::get(p);
		if (c == '=')
		{   p = Dchar::inc(p);
		    t->value = TOKgreaterequal;
		}
		else if (c == '>')
		{   p = Dchar::inc(p);
		    c = Dchar::get(p);
		    if (c == '=')
		    {   p = Dchar::inc(p);
			t->value = TOKshiftrightass;
		    }
		    else if (c == '>')
		    {	p = Dchar::inc(p);
			c = Dchar::get(p);
			if (c == '=')
			{   p = Dchar::inc(p);
			    t->value = TOKushiftrightass;
			}
			else
			    t->value = TOKushiftright;
		    }
		    else
			t->value = TOKshiftright;
		}
		else
		    t->value = TOKgreater;
		return;

#define SINGLE(c,tok) case c: p = Dchar::inc(p); t->value = tok; return;

	    SINGLE('(',	TOKlparen)
	    SINGLE(')', TOKrparen)
	    SINGLE('[', TOKlbracket)
	    SINGLE(']', TOKrbracket)
	    SINGLE('{', TOKlbrace)
	    SINGLE('}', TOKrbrace)
	    SINGLE('~', TOKtilde)
	    SINGLE('?', TOKquestion)
	    SINGLE(',', TOKcomma)
	    SINGLE(';', TOKsemicolon)
	    SINGLE(':', TOKcolon)

#undef SINGLE

#define DOUBLE(c1,tok1,c2,tok2)		\
	    case c1:			\
		p = Dchar::inc(p);	\
		c = Dchar::get(p);	\
		if (c == c2)		\
		{   p = Dchar::inc(p);	\
		    t->value = tok2;	\
		}			\
		else			\
		    t->value = tok1;	\
		return;

	    DOUBLE('*', TOKmultiply, '=', TOKmultiplyass)
	    DOUBLE('%', TOKpercent, '=', TOKpercentass)
	    DOUBLE('^', TOKxor, '=', TOKxorass)

#undef DOUBLE

#define TRIPLE(c1,tok1,c2,tok2,c3,tok3)		\
	    case c1:				\
		p = Dchar::inc(p);		\
		c = Dchar::get(p);		\
		if (c == c2)			\
		{   p = Dchar::inc(p);		\
		    c = Dchar::get(p);		\
		    if (c == c3)		\
		    {	p = Dchar::inc(p);	\
			t->value = tok3;	\
		    }				\
		    else			\
			t->value = tok2;	\
		}				\
		else				\
		    t->value = tok1;		\
		return;

	    TRIPLE('=', TOKassign, '=', TOKequal, '=', TOKidentity)
	    TRIPLE('!', TOKnot, '=', TOKnotequal, '=', TOKnonidentity)

#undef TRIPLE

	    case '\\':
		if (p[1] == 'u')
		{
		    // \uXXXX starts an identifier
		    goto Lidentifier2;
		}
	    default:
		c = Dchar::get(p);
		if (isalpha(c))         // This should be isalpha -- any Unicode letter
		    goto Lidentifier;
		else
		{
		    errinfo.code = 1014;
		    if (isprint(c))
			error(ERR_BAD_CHAR_C, c);
		    else
			error(ERR_BAD_CHAR_X, c);
		}
		continue;
	}
    }
}

/*******************************************
 * Parse escape sequence.
 */

unsigned Lexer::escapeSequence()
{   unsigned c;
    int n;

    c = Dchar::get(p);
    p = Dchar::inc(p);
    switch (c)
    {
	case '\'':
	case '"':
	case '?':
	case '\\':
		break;
	case 'a':
		c = 7;
		break;
	case 'b':
		c = 8;
		break;
	case 'f':
		c = 12;
		break;
	case 'n':
		c = 10;
		break;
	case 'r':
		c = 13;
		break;
	case 't':
		c = 9;
		break;
#if !JSCRIPT_ESCAPEV_BUG
	case 'v':
		c = 11;
		break;
#endif
	case 'x':
		c = Dchar::get(p);
		p = Dchar::inc(p);
		if (ishex(c))
		{   unsigned v;

		    n = 0;
		    v = 0;
		    for (;;)
		    {
			if (Dchar::isDigit((dchar)c))
			    c -= '0';
			else if (isasciilower(c))  // Do NOT use islower() -- locale issues
			    c -= 'a' - 10;
			else    // 'A' <= c && c <= 'Z'
			    c -= 'A' - 10;
			v = v * 16 + c;
			c = Dchar::get(p);
			if (++n >= 2 || !ishex(c))
			    break;
			p = Dchar::inc(p);
		    }
		    if (n == 1)
			error(ERR_BAD_HEX_SEQUENCE);
		    c = v;
		}
		else
		    error(ERR_UNDEFINED_ESC_SEQUENCE,c);
		break;

	default:
		if (isoctal(c))
		{   unsigned v;

		    n = 0;
		    v = 0;
		    for (;;)
		    {
			v = v * 8 + (c - '0');
			c = Dchar::get(p);
			if (++n >= 3 || !isoctal(c))
			    break;
			p = Dchar::inc(p);
		    }
		    c = v;
		}
		// Don't throw error, just accept it
		//error("undefined escape sequence \\%c\n",c);
		break;
    }
    return c;
}

/**************************************
 */

Lstring *Lexer::string(unsigned quote)
{   unsigned c;

    stringbuffer.reset();
    p = Dchar::inc(p);
    for (;;)
    {
	c = Dchar::get(p);
	switch (c)
	{
	    case '"':
	    case '\'':
		p = Dchar::inc(p);
	        if (c == quote)
		{
		Ldone:
#if 0
		    // Use a string pool to:
		    // 1) reduce memory consumption
		    // 2) increase likelihood that matching strings
		    //    will share a pointer (for fast comparison)
		    StringValue *sv;
		    sv = stringtable->update((dchar *)stringbuffer.data, stringbuffer.offset / sizeof(dchar));
		    return &sv->lstring;
#else
		    GC_LOG();
		    return Lstring::ctor((dchar *)stringbuffer.data, stringbuffer.offset / sizeof(dchar));
#endif
		}
		break;

	    case '\\':
		p = Dchar::inc(p);
		if (*p == 'u')
		{
		    stringbuffer.writedchar(unicode());
		    continue;
		}
		c = escapeSequence();
		break;

	    case '\n':
	    case '\r':
		p = Dchar::inc(p);
		error(ERR_STRING_NO_END_QUOTE);
		break;

	    case 0:
	    case 0x1A:
		error(ERR_UNTERMINATED_STRING);
		goto Ldone;

	    default:
		p = Dchar::inc(p);
		break;
	}
	stringbuffer.writedchar(c);
    }
}

/**************************************
 * Scan regular expression. Return NULL with buffer
 * pointer intact if it is not a regexp.
 */

Lstring *Lexer::regexp()
{   unsigned c;
    dchar *s;

    /*
	RegExpLiteral:  RegExpBody RegExpFlags
	  RegExpFlags:
	      empty
	   |  RegExpFlags ContinuingIdentifierCharacter
	  RegExpBody:  / RegExpFirstChar RegExpChars /
	  RegExpFirstChar: 
	      OrdinaryRegExpFirstChar
	   |  \ NonTerminator
	  OrdinaryRegExpFirstChar:  NonTerminator except \ | / | *
	  RegExpChars:
	      empty
	   |  RegExpChars RegExpChar
	  RegExpChar:
	      OrdinaryRegExpChar
	   |  \ NonTerminator
	  OrdinaryRegExpChar: NonTerminator except \ | /
     */

    stringbuffer.reset();
    stringbuffer.writedchar('/');
    s = (dchar *)p;

    // Do RegExpBody
    for (;;)
    {
	c = Dchar::get(s);
	s = Dchar::inc(s);
	switch (c)
	{
	    case '\\':
		if (!stringbuffer.offset)
		    return NULL;
		stringbuffer.writedchar(c);
		c = Dchar::get(s);
		switch (c)
		{
		    case '\r':
		    case '\n':			// new line
		    case 0:			// end of file
		    case 0x1A:			// end of file
			return NULL;		// not a regexp
		}
		stringbuffer.writedchar(c);
		s = Dchar::inc(s);
		continue;

	    case '/':
		if (!stringbuffer.offset)
		    return NULL;
		stringbuffer.writedchar(c);
		break;

	    case '\r':
	    case '\n':			// new line
	    case 0:			// end of file
	    case 0x1A:			// end of file
		return NULL;		// not a regexp

	    case '*':
		if (!stringbuffer.offset)
		    return NULL;
	    default:
		stringbuffer.writedchar(c);
		continue;
	}
	break;
    }

    // Do RegExpFlags
    for (;;)
    {
	c = Dchar::get(s);
	if (isalnum(c) || c == '_' || c == '$')
	{
	    s = Dchar::inc(s);
	    stringbuffer.writedchar(c);
	}
	else
	    break;
    }

    // Finish pattern & return it
    p = (dchar *)s;
    GC_LOG();
    return Lstring::ctor((dchar *)stringbuffer.data, stringbuffer.offset / sizeof(dchar));
}

/***************************************
 */

unsigned Lexer::unicode()
{
    unsigned value;
    unsigned n;
    unsigned c;

    value = 0;
    p = Dchar::inc(p);
    for (n = 0; n < 4; n++)
    {
	c = Dchar::get(p);
	if (!ishex(c))
	{   error(ERR_BAD_U_SEQUENCE);
	    break;
	}
	p = Dchar::inc(p);
	if (Dchar::isDigit((dchar)c))
	    c -= '0';
	else if (isasciilower(c))   // Do NOT use islower() -- locale issues
	    c -= 'a' - 10;
	else    // 'A' <= c && c <= 'Z'
	    c -= 'A' - 10;
	value <<= 4;
	value |= c;
    }
    return value;
}

/********************************************
 * Read a number.
 */

TOK Lexer::number(Token *t)
{   dchar *start;
    number_t intvalue;
    double realvalue;
    int base = 10;
    unsigned c;
    unsigned len;
    unsigned u;

    stringbuffer.reset();
    start = p;
    for (;;)
    {
	c = Dchar::get(p);
	p = Dchar::inc(p);
	switch (c)
	{
	    case '0':
#if 1		// ECMA grammar implies that numbers with leading 0
		// like 015 are illegal. But other scripts allow them.
		if (p - start == 1)		// if leading 0
		    base = 8;
#endif
	    case '1': case '2': case '3': case '4': case '5':
	    case '6': case '7':
		break;

	    case '8': case '9':			// decimal digits
		if (base == 8)			// and octal base
		    base = 10;			// means back to decimal base
		break;

	    default:
		p--;
	    Lnumber:
#if defined UNICODE
		len = p - start;
		stringbuffer.reserve(len + 1);
		for (u = 0; u < len; u++)
		{
		    stringbuffer.writebyte(start[u]);
		}
#else
		stringbuffer.write(start, p - start);
#endif
		stringbuffer.writebyte(0);
		errno = 0;
		intvalue = Port::strtoull((char *)stringbuffer.data, NULL, base);
		if (errno == ERANGE)
		{   char *s;
		    int v;
#if defined(__DMC__)
		    long double realvalue;
#endif

		    realvalue = 0;
		    if (base == 0)
			base = 10;
		    for (s = (char *)stringbuffer.data; *s; s++)
		    {
			v = *s;
			if ('0' <= v && v <= '9')
			    v -= '0';
			else if ('a' <= v && v <= 'f')
			    v -= ('a' - 10);
			else if ('A' <= v && v <= 'F')
			    v -= ('A' - 10);
			else
			    assert(0);
			realvalue *= base;
			realvalue += v;
		    }
		    t->realvalue = realvalue;
		}
		else
		{
		    t->realvalue = Port::ull_to_double(intvalue);
		}
		return TOKreal;

	    case 'x':
	    case 'X':
		if (p - start != 2 || !ishex(*p))
		    goto Lerr;
		do
		    p = Dchar::inc(p);
		while (ishex(*p));
		start += 2;
		base = 16;
		goto Lnumber;

	    case '.':
		while (Dchar::isDigit(*p))
		    p = Dchar::inc(p);
		if (*p == 'e' || *p == 'E')
		{   p++;
		    goto Lexponent;
		}
		goto Ldouble;

	    case 'e':
	    case 'E':
	    Lexponent:
		if (*p == '+' || *p == '-')
		    p = Dchar::inc(p);
		if (!Dchar::isDigit(*p))
		    goto Lerr;
		do
		    p = Dchar::inc(p);
		while (Dchar::isDigit(*p));
		goto Ldouble;

	    Ldouble:
		// convert double
#if defined UNICODE
		len = p - start;
		stringbuffer.reserve(len + 1);
		for (u = 0; u < len; u++)
		{
		    stringbuffer.writebyte(start[u]);
		}
#else
		stringbuffer.write(start, p - start);
#endif
		stringbuffer.writebyte(0);
		errno = 0;
		realvalue = strtod((char *)stringbuffer.data, NULL);
		t->realvalue = realvalue;
		return TOKreal;
	}
    }

Lerr:
    error(ERR_UNRECOGNIZED_N_LITERAL);
    return TOKeof;
}

enum TOK Lexer::isKeyword(dchar *s, unsigned len)
{
    // The main reason for doing it like this is to avoid the overhead
    // of putting it in the GC space.

  if (s[0] >= 'a')
    switch (len)
    {
	case 2:
	    if (s[0] == 'i')
	    {
		if (s[1] == 'f')
		    return TOKif;
		if (s[1] == 'n')
		    return TOKin;
	    }
	    else if (s[0] == 'd' && s[1] == 'o')
		return TOKdo;
	    break;

	case 3:
	    switch (s[0])
	    {
		case 'f':
		    if (s[1] == 'o' && s[2] == 'r')
			return TOKfor;
		    break;
		case 'i':
		    if (s[1] == 'n' && s[2] == 't')
			return TOKint;
		    break;
		case 'n':
		    if (s[1] == 'e' && s[2] == 'w')
			return TOKnew;
		    break;
		case 't':
		    if (s[1] == 'r' && s[2] == 'y')
			return TOKtry;
		    break;
		case 'v':
		    if (s[1] == 'a' && s[2] == 'r')
			return TOKvar;
		    break;
	    }
	    break;

	case 4:
	    switch (s[0])
	    {
		case 'b':
		    if (s[1] == 'y' && s[2] == 't' && s[3] == 'e')
			return TOKbyte;
		    break;
		case 'c':
		    if (s[1] == 'a' && s[2] == 's' && s[3] == 'e')
			return TOKcase;
		    if (s[1] == 'h' && s[2] == 'a' && s[3] == 'r')
			return TOKchar;
		    break;
		case 'e':
		    if (s[1] == 'l' && s[2] == 's' && s[3] == 'e')
			return TOKelse;
		    if (s[1] == 'n' && s[2] == 'u' && s[3] == 'm')
			return TOKenum;
		    break;
		case 'g':
		    if (s[1] == 'o' && s[2] == 't' && s[3] == 'o')
			return TOKgoto;
		    break;
		case 'l':
		    if (s[1] == 'o' && s[2] == 'n' && s[3] == 'g')
			return TOKlong;
		    break;
		case 'n':
		    if (s[1] == 'u' && s[2] == 'l' && s[3] == 'l')
			return TOKnull;
		    break;
		case 't':
		    if (s[1] == 'h' && s[2] == 'i' && s[3] == 's')
			return TOKthis;
		    if (s[1] == 'r' && s[2] == 'u' && s[3] == 'e')
			return TOKtrue;
		    break;
		case 'w':
		    if (s[1] == 'i' && s[2] == 't' && s[3] == 'h')
			return TOKwith;
		    break;
		case 'v':
		    if (s[1] == 'o' && s[2] == 'i' && s[3] == 'd')
			return TOKvoid;
		    break;
	    }
	    break;

	case 5:
	    switch (s[0])
	    {
		case 'b':
		    if (memcmp(s + 1, L"reak", 4 * sizeof(dchar)) == 0)
			return TOKbreak;
		    break;
		case 'c':
		    if (memcmp(s + 1, L"atch", 4 * sizeof(dchar)) == 0)
			return TOKcatch;
		    if (memcmp(s + 1, L"lass", 4 * sizeof(dchar)) == 0)
			return TOKclass;
		    if (memcmp(s + 1, L"onst", 4 * sizeof(dchar)) == 0)
			return TOKconst;
		    break;
		case 'f':
		    if (memcmp(s + 1, L"alse", 4 * sizeof(dchar)) == 0)
			return TOKfalse;
		    if (memcmp(s + 1, L"inal", 4 * sizeof(dchar)) == 0)
			return TOKfinal;
		    if (memcmp(s + 1, L"loat", 4 * sizeof(dchar)) == 0)
			return TOKfloat;
		    break;
		case 's':
		    if (memcmp(s + 1, L"hort", 4 * sizeof(dchar)) == 0)
			return TOKshort;
		    if (memcmp(s + 1, L"uper", 4 * sizeof(dchar)) == 0)
			return TOKsuper;
		    break;
		case 't':
		    if (memcmp(s + 1, L"hrow", 4 * sizeof(dchar)) == 0)
			return TOKthrow;
		    break;
		case 'w':
		    if (memcmp(s + 1, L"hile", 4 * sizeof(dchar)) == 0)
			return TOKwhile;
		    break;
	    }
	    break;

	case 6:
	    switch (s[0])
	    {
		case 'd':
		    if (memcmp(s + 1, L"elete", 5 * sizeof(dchar)) == 0)
			return TOKdelete;
		    if (memcmp(s + 1, L"ouble", 5 * sizeof(dchar)) == 0)
			return TOKdouble;
		    break;
		case 'e':
		    if (memcmp(s + 1, L"xport", 5 * sizeof(dchar)) == 0)
			return TOKexport;
		    break;
		case 'i':
		    if (memcmp(s + 1, L"mport", 5 * sizeof(dchar)) == 0)
			return TOKimport;
		    break;
		case 'n':
		    if (memcmp(s + 1, L"ative", 5 * sizeof(dchar)) == 0)
			return TOKnative;
		    break;
		case 'p':
		    if (memcmp(s + 1, L"ublic", 5 * sizeof(dchar)) == 0)
			return TOKpublic;
		    break;
		case 'r':
		    if (memcmp(s + 1, L"eturn", 5 * sizeof(dchar)) == 0)
			return TOKreturn;
		    break;
		case 's':
		    if (memcmp(s + 1, L"tatic", 5 * sizeof(dchar)) == 0)
			return TOKstatic;
		    if (memcmp(s + 1, L"witch", 5 * sizeof(dchar)) == 0)
			return TOKswitch;
		    break;
		case 't':
		    if (memcmp(s + 1, L"rows", 5 * sizeof(dchar)) == 0)
			return TOKthrows;
		    if (memcmp(s + 1, L"ypeof", 5 * sizeof(dchar)) == 0)
			return TOKtypeof;
		    break;
	    }
	    break;

	case 7:
	    switch (s[0])
	    {
		case 'b':
		    if (memcmp(s + 1, L"oolean", 6 * sizeof(dchar)) == 0)
			return TOKboolean;
		    break;
		case 'd':
		    if (memcmp(s + 1, L"efault", 6 * sizeof(dchar)) == 0)
			return TOKdefault;
		    break;
		case 'e':
		    if (memcmp(s + 1, L"xtends", 6 * sizeof(dchar)) == 0)
			return TOKextends;
		    break;
		case 'f':
		    if (memcmp(s + 1, L"inally", 6 * sizeof(dchar)) == 0)
			return TOKfinally;
		    break;
		case 'p':
		    if (memcmp(s + 1, L"ackage", 6 * sizeof(dchar)) == 0)
			return TOKpackage;
		    if (memcmp(s + 1, L"rivate", 6 * sizeof(dchar)) == 0)
			return TOKprivate;
		    break;
	    }
	    break;

	case 8:
	    switch (s[0])
	    {
		case 'a':
		    if (memcmp(s + 1, L"bstract", 7 * sizeof(dchar)) == 0)
			return TOKabstract;
		    break;
		case 'c':
		    if (memcmp(s + 1, L"ontinue", 7 * sizeof(dchar)) == 0)
			return TOKcontinue;
		    break;
		case 'd':
		    if (memcmp(s + 1, L"ebugger", 7 * sizeof(dchar)) == 0)
			return TOKdebugger;
		    break;
		case 'f':
		    if (memcmp(s + 1, L"unction", 7 * sizeof(dchar)) == 0)
			return TOKfunction;
		    break;
	    }
	    break;

	case 9:
	    switch (s[0])
	    {
		// BUG: the following are 9 chars
		case 'i':
		    if (memcmp(s + 1, L"nterface", 8 * sizeof(dchar)) == 0)
			return TOKinterface;
		    break;
		case 'p':
		    if (memcmp(s + 1, L"rotected", 8 * sizeof(dchar)) == 0)
			return TOKprotected;
		    break;
		case 't':
		    if (memcmp(s + 1, L"ransient", 8 * sizeof(dchar)) == 0)
			return TOKtransient;
		    break;
	    }
	    break;

	case 10:
	    if (s[0] == 'i')
	    {
		if (memcmp(s + 1, L"mplements", 9 * sizeof(dchar)) == 0)
		    return TOKimplements;
		if (memcmp(s + 1, L"nstanceof", 9 * sizeof(dchar)) == 0)
		    return TOKinstanceof;
	    }
	    break;

	case 12:
	    if (memcmp(s, L"synchronized", 12 * sizeof(dchar)) == 0)
		return TOKsynchronized;
	    break;
    }
    return TOKreserved;		// not a keyword
}

/****************************************
 */

struct Keyword
{   dchar *name;
    enum TOK value;
};

static Keyword keywords[] =
{
//    {	DTEXT(""),		TOK		},

    {	DTEXT("break"),		TOKbreak	},
    {	DTEXT("case"),		TOKcase		},
    {	DTEXT("continue"),	TOKcontinue	},
    {	DTEXT("default"),	TOKdefault	},
    {	DTEXT("delete"),	TOKdelete	},
    {	DTEXT("do"),		TOKdo		},
    {	DTEXT("else"),		TOKelse		},
    {	DTEXT("export"),	TOKexport	},
    {	DTEXT("false"),		TOKfalse	},
    {	DTEXT("for"),		TOKfor		},
    {	DTEXT("function"),	TOKfunction	},
    {	DTEXT("if"),		TOKif		},
    {	DTEXT("import"),	TOKimport	},
    {	DTEXT("in"),		TOKin		},
    {	DTEXT("new"),		TOKnew		},
    {	DTEXT("null"),		TOKnull		},
    {	DTEXT("return"),	TOKreturn	},
    {	DTEXT("switch"),	TOKswitch	},
    {	DTEXT("this"),		TOKthis		},
    {	DTEXT("true"),		TOKtrue		},
    {	DTEXT("typeof"),	TOKtypeof	},
    {	DTEXT("var"),		TOKvar		},
    {	DTEXT("void"),		TOKvoid		},
    {	DTEXT("while"),		TOKwhile	},
    {	DTEXT("with"),		TOKwith		},

    {	DTEXT("catch"),		TOKcatch	},
    {	DTEXT("class"),		TOKclass	},
    {	DTEXT("const"),		TOKconst	},
    {	DTEXT("debugger"),	TOKdebugger	},
    {	DTEXT("enum"),		TOKenum		},
    {	DTEXT("extends"),	TOKextends	},
    {	DTEXT("finally"),	TOKfinally	},
    {	DTEXT("super"),		TOKsuper	},
    {	DTEXT("throw"),		TOKthrow	},
    {	DTEXT("try"),		TOKtry		},

    {	DTEXT("abstract"),	TOKabstract	},
    {	DTEXT("boolean"),	TOKboolean	},
    {	DTEXT("byte"),		TOKbyte		},
    {	DTEXT("char"),		TOKchar		},
    {	DTEXT("double"),	TOKdouble	},
    {	DTEXT("final"),		TOKfinal	},
    {	DTEXT("float"),		TOKfloat	},
    {	DTEXT("goto"),		TOKgoto		},
    {	DTEXT("implements"),	TOKimplements	},
    {	DTEXT("instanceof"),	TOKinstanceof	},
    {	DTEXT("int"),		TOKint		},
    {	DTEXT("interface"),	TOKinterface	},
    {	DTEXT("long"),		TOKlong		},
    {	DTEXT("native"),	TOKnative	},
    {	DTEXT("package"),	TOKpackage	},
    {	DTEXT("private"),	TOKprivate	},
    {	DTEXT("protected"),	TOKprotected	},
    {	DTEXT("public"),	TOKpublic	},
    {	DTEXT("short"),		TOKshort	},
    {	DTEXT("static"),	TOKstatic	},
    {	DTEXT("synchronized"),	TOKsynchronized	},
    {	DTEXT("throws"),	TOKthrows	},
    {	DTEXT("transient"),	TOKtransient	},
};

void Lexer::initKeywords()
{   //StringValue *sv;
    unsigned u;
    enum TOK v;

    for (u = 0; u < sizeof(keywords) / sizeof(keywords[0]); u++)
    {	dchar *s;

	//PRINTF("keyword[%d] = '%s'\n",u, keywords[u].name);
	s = keywords[u].name;
	v = keywords[u].value;
	//GC_LOG();
	//sv = stringtable->insert(s, Dchar::len(s));
	//sv->intvalue = (int)v;

	//PRINTF("tochars[%d] = '%s'\n",v, s);
	Token::tochars[v] = s;
    }

    Token::tochars[TOKreserved]		= DTEXT("reserved");
    Token::tochars[TOKeof]		= DTEXT("EOF");
    Token::tochars[TOKlbrace]		= DTEXT("{");
    Token::tochars[TOKrbrace]		= DTEXT("}");
    Token::tochars[TOKlparen]		= DTEXT("(");
    Token::tochars[TOKrparen]		= DTEXT(")");
    Token::tochars[TOKlbracket]		= DTEXT("[");
    Token::tochars[TOKrbracket]		= DTEXT("]");
    Token::tochars[TOKcolon]		= DTEXT(":");
    Token::tochars[TOKsemicolon]	= DTEXT(";");
    Token::tochars[TOKcomma]		= DTEXT(",");
    Token::tochars[TOKor]		= DTEXT("|");
    Token::tochars[TOKorass]		= DTEXT("|=");
    Token::tochars[TOKxor]		= DTEXT("^");
    Token::tochars[TOKxorass]		= DTEXT("^=");
    Token::tochars[TOKassign]		= DTEXT("=");
    Token::tochars[TOKless]		= DTEXT("<");
    Token::tochars[TOKgreater]		= DTEXT(">");
    Token::tochars[TOKlessequal]	= DTEXT("<=");
    Token::tochars[TOKgreaterequal]	= DTEXT(">=");
    Token::tochars[TOKequal]		= DTEXT("==");
    Token::tochars[TOKnotequal]		= DTEXT("!=");
    Token::tochars[TOKidentity]		= DTEXT("===");
    Token::tochars[TOKnonidentity]	= DTEXT("!==");
    Token::tochars[TOKshiftleft]	= DTEXT("<<");
    Token::tochars[TOKshiftright]	= DTEXT(">>");
    Token::tochars[TOKushiftright]	= DTEXT(">>>");
    Token::tochars[TOKplus]		= DTEXT("+");
    Token::tochars[TOKplusass]		= DTEXT("+=");
    Token::tochars[TOKminus]		= DTEXT("-");
    Token::tochars[TOKminusass]		= DTEXT("-=");
    Token::tochars[TOKmultiply]		= DTEXT("*");
    Token::tochars[TOKmultiplyass]	= DTEXT("*=");
    Token::tochars[TOKdivide]		= DTEXT("/");
    Token::tochars[TOKdivideass]	= DTEXT("/=");
    Token::tochars[TOKpercent]		= DTEXT("%");
    Token::tochars[TOKpercentass]	= DTEXT("%=");
    Token::tochars[TOKand]		= DTEXT("&");
    Token::tochars[TOKandass]		= DTEXT("&=");
    Token::tochars[TOKdot]		= DTEXT(".");
    Token::tochars[TOKquestion]		= DTEXT("?");
    Token::tochars[TOKtilde]		= DTEXT("~");
    Token::tochars[TOKnot]		= DTEXT("!");
    Token::tochars[TOKandand]		= DTEXT("&&");
    Token::tochars[TOKoror]		= DTEXT("||");
    Token::tochars[TOKplusplus]		= DTEXT("++");
    Token::tochars[TOKminusminus]	= DTEXT("--");
    Token::tochars[TOKcall]		= DTEXT("CALL");
}

