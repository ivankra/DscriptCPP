/* Digital Mars DMDScript source code.
 * Copyright (c) 2000-2001 by Chromium Communications
 * Copyright (c) 2004-2007 by Digital Mars
 * All Rights Reserved, written by Walter Bright
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


// Regular Expressions


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

#include "root.h"
#include "regexp.h"
#include "mem.h"
#include "printf.h"

// Disable debugging printf's
#define PRINTF	1 ||
#define WPRINTF	1 ||

// Opcodes

enum REopcodes
{
    REend,		// end of program
    REchar,		// single character
    REichar,		// single character, case insensitive
    REwchar,		// single wide character
    REiwchar,		// single wide character, case insensitive
    REanychar,		// any character
    REanystar,		// ".*"
    REstring,		// string of characters
    REtestbit,		// any in bitmap, non-consuming
    REbit,		// any in the bit map
    REnotbit,		// any not in the bit map
    RErange,		// any in the string
    REnotrange,		// any not in the string
    REor,		// a | b
    REplus,		// 1 or more
    REstar,		// 0 or more
    REquest,		// 0 or 1
    REnm,		// n..m
    REnmq,		// n..m, non-greedy version
    REbol,		// beginning of line
    REeol,		// end of line
    REparen,		// parenthesized subexpression
    REgoto,		// goto offset

    REwordboundary,
    REnotwordboundary,
    REdigit,
    REnotdigit,
    REspace,
    REnotspace,
    REword,
    REnotword,
    REbackref,
};

// BUG: should this include '$'?
static int isword(dchar c) { return isalnum(c) || c == '_'; }

static unsigned inf = ~0u;

RegExp::RegExp(dchar *pattern, dchar *attributes, int ref)
{
    nparens = 0;
    parens = NULL;
    this->pattern = NULL;
    compile(pattern, attributes, ref);
}

RegExp::~RegExp()
{
    if (!ref)
	mem.free(pattern);
    mem.free(parens);
}

void RegExp::mark()
{
    mem.mark(pattern);
    mem.mark(parens);
    mem.mark(buf);
    mem.mark(program);
    mem.mark(input);
}

// Initialize a regular expression object for a new expression pattern.
//      Compile the regular expression pattern string.
//      Create or re-allocate the match list for parenthetical expressions 
//      Clear or initialize data for new match test

// Returns 1 on success, 0 error

int RegExp::compile(dchar *pattern, dchar *attributes, int ref)
{
    WPRINTF(L"RegExp::compile('%ls', '%ls')\n", pattern, attributes);
    this->attributes = 0;
    this->errors = 0;
    if (attributes)
    {	dchar *p = attributes;

	for ( ; *p; p = Dchar::inc(p))
	{   unsigned att;

	    switch (*p)
	    {
		case 'g': att = REAglobal;	break;
		case 'i': att = REAignoreCase;	break;
		case 'm': att = REAmultiline;	break;
		default:
		    errors++;
		    return 0;		// unrecognized attribute
	    }
	    if (this->attributes & att)
	    {	errors++;
		return 0;		// redundant attribute
	    }
	    this->attributes |= att;
	}
    }

    input = NULL;
    text.istart = 0;
    text.iend = 0;

    if (!this->ref)
	mem.free(this->pattern);
    this->pattern = ref ? pattern : Dchar::dup(pattern);
    this->ref = ref;
    Dchar::cpy(flags, attributes);

    unsigned oldnparens = nparens;
    nparens = 0;
    errors = 0;

    buf = new OutBuffer();
    buf->reserve(Dchar::len(pattern) * 8);
    p = this->pattern;
    parseRegexp();
    if (*p)
    {	error("unmatched ')'");
    }
    if (!errors)
	optimize();
    program = (char *)buf->data;
    buf->data = NULL;
    delete buf;

    if (nparens > oldnparens)
    {
	parens = (Match *)mem.realloc(parens, nparens * sizeof(Match));
    }

    return (errors == 0);
}

// Given a starting point into a string, test for a match to compiled pattern
// If found,
//      text.istart is offset from string+startindex to match
//      text.iend is offset from string+startindex to 1st char after match
//      if the pattern contained parenthetical sub-expressions
//          parens is the start and end offsets for each sub-expression match

int RegExp::test(dchar *string, int startindex)
{   dchar *s;
    dchar *sstart;
    unsigned firstc;

#if defined(UNICODE)
    WPRINTF(L"RegExp::test(string = '%ls', index = %d)\n", string, text.iend);
#else
    //PRINTF("RegExp::test(string = '%s', index = %d)\n", string, text.iend);
#endif // UNICODE
    input = string;
    if (startindex < 0 || startindex > Dchar::len(string))
    {
	text.iend = 0;
	return 0;			// fail
    }
    sstart = string + startindex;
    text.istart = 0;
    text.iend = 0;

    printProgram(program);

    // First character optimization
    firstc = 0;
    if (program[0] == REchar)
    {
	firstc = *(unsigned char *)(program + 1);
	if (attributes & REAignoreCase && isalpha(firstc))
	    firstc = 0;
    }

    for (s = sstart; ; s = Dchar::inc(s))
    {
	if (firstc && Dchar::get(s) != (dchar)firstc)
	{
	    s = Dchar::inc(s);
	    s = Dchar::chr(s, firstc);
	    if (!s)
		break;
	}
	memset(parens, -1, nparens * sizeof(Match));
	src_start = src = s;
	if (match(program, NULL))
	{
	    text.istart = src_start - input;
	    text.iend = src - input;
	    PRINTF("start = %d, end = %d\n", text.istart, text.iend);
	    return 1;
	}
	// If possible match must start at beginning, we are done
	if (program[0] == REbol || program[0] == REanystar)
	{
	    if (attributes & REAmultiline)
	    {
		// Scan for the next \n
		s = Dchar::chr(s, '\n');
		if (!s)
		    break;
	    }
	    else
		break;
	}
	if (!*s)
	    break;
	PRINTF("Starting new try: '%s'\n", s + 1);
    }
    return 0;		// no match
}

void RegExp::printProgram(char *prog)
{
#if 1
    prog = 0;			// fix for /W4
#else
    char *progstart;
    int pc;
    unsigned len;
    unsigned n;
    unsigned m;

    PRINTF("printProgram()\n");
    progstart = prog;
    for (;;)
    {
	pc = prog - progstart;
	PRINTF("%3d: ", pc);

	switch (*prog)
	{
	    case REchar:
		WPRINTF(L"\tREchar '%c'\n", *(unsigned char *)(prog + 1));
		prog += 1 + sizeof(char);
		break;

	    case REichar:
		PRINTF("\tREichar '%c'\n", *(unsigned char *)(prog + 1));
		prog += 1 + sizeof(char);
		break;

	    case REwchar:
		PRINTF("\tREwchar '%c'\n", *(wchar_t *)(prog + 1));
		prog += 1 + sizeof(wchar_t);
		break;

	    case REiwchar:
		PRINTF("\tREiwchar '%c'\n", *(wchar_t *)(prog + 1));
		prog += 1 + sizeof(wchar_t);
		break;

	    case REanychar:
		PRINTF("\tREanychar\n");
		prog++;
		break;

	    case REstring:
		len = *(unsigned *)(prog + 1);
		PRINTF(L"\tREstring x%x, '%c'\n", len,
			*(dchar *)(prog + 1 + sizeof(unsigned)));
		prog += 1 + sizeof(unsigned) + len * sizeof(dchar);
		break;

	    case REtestbit:
		PRINTF("\tREtestbit %d, %d\n",
		    ((unsigned short *)(prog + 1))[0],
		    ((unsigned short *)(prog + 1))[1]);
		len = ((unsigned short *)(prog + 1))[1];
		prog += 1 + 2 * sizeof(unsigned short) + len;
		break;

	    case REbit:
		PRINTF("\tREbit %d, %d\n",
		    ((unsigned short *)(prog + 1))[0],
		    ((unsigned short *)(prog + 1))[1]);
		len = ((unsigned short *)(prog + 1))[1];
		prog += 1 + 2 * sizeof(unsigned short) + len;
		break;

	    case REnotbit:
		PRINTF("\tREnotbit %d, %d\n",
		    ((unsigned short *)(prog + 1))[0],
		    ((unsigned short *)(prog + 1))[1]);
		len = ((unsigned short *)(prog + 1))[1];
		prog += 1 + 2 * sizeof(unsigned short) + len;
		break;

	    case RErange:
		PRINTF("\tRErange %d\n", *(unsigned *)(prog + 1));
		// BUG: REAignoreCase?
		len = *(unsigned *)(prog + 1);
		prog += 1 + sizeof(unsigned) + len;
		break;

	    case REnotrange:
		PRINTF("\tREnotrange %d\n", *(unsigned *)(prog + 1));
		// BUG: REAignoreCase?
		len = *(unsigned *)(prog + 1);
		prog += 1 + sizeof(unsigned) + len;
		break;

	    case REbol:
		PRINTF("\tREbol\n");
		prog++;
		break;

	    case REeol:
		PRINTF("\tREeol\n");
		prog++;
		break;

	    case REor:
		len = ((unsigned *)(prog + 1))[0];
		PRINTF("\tREor %d, pc=>%d\n", len, pc + 1 + sizeof(unsigned) + len);
		prog += 1 + sizeof(unsigned);
		break;

	    case REgoto:
		len = ((unsigned *)(prog + 1))[0];
		PRINTF("\tREgoto %d, pc=>%d\n", len, pc + 1 + sizeof(unsigned) + len);
		prog += 1 + sizeof(unsigned);
		break;

	    case REanystar:
		PRINTF("\tREanystar\n");
		prog++;
		break;

	    case REnm:
	    case REnmq:
		// len, n, m, ()
		len = ((unsigned *)(prog + 1))[0];
		n = ((unsigned *)(prog + 1))[1];
		m = ((unsigned *)(prog + 1))[2];
		PRINTF(L"\tREnm%ls len=%d, n=%u, m=%u, pc=>%d\n", (*prog == REnmq) ? L"q" : L"", len, n, m, pc + 1 + sizeof(unsigned) * 3 + len);
		prog += 1 + sizeof(unsigned) * 3;
		break;

	    case REparen:
		// len, ()
		len = ((unsigned *)(prog + 1))[0];
		n = ((unsigned *)(prog + 1))[1];
		PRINTF("\tREparen len=%d n=%d, pc=>%d\n", len, n, pc + 1 + sizeof(unsigned) * 2 + len);
		prog += 1 + sizeof(unsigned) * 2;
		break;

	    case REend:
		PRINTF("\tREend\n");
		return;

	    case REwordboundary:
		PRINTF("\tREwordboundary\n");
		prog++;
		break;

	    case REnotwordboundary:
		PRINTF("\tREnotwordboundary\n");
		prog++;
		break;

	    case REdigit:
		PRINTF("\tREdigit\n");
		prog++;
		break;

	    case REnotdigit:
		PRINTF("\tREnotdigit\n");
		prog++;
		break;

	    case REspace:
		PRINTF("\tREspace\n");
		prog++;
		break;

	    case REnotspace:
		PRINTF("\tREnotspace\n");
		prog++;
		break;

	    case REword:
		PRINTF("\tREword\n");
		prog++;
		break;

	    case REnotword:
		PRINTF("\tREnotword\n");
		prog++;
		break;

	    case REbackref:
		PRINTF("\tREbackref %d\n", prog[1]);
		prog += 2;
		break;

	    default:
		assert(0);
	}
    }
#endif
}

int RegExp::match(char *prog, char *progend)
{   dchar *srcsave;
    unsigned len;
    unsigned n;
    unsigned m;
    unsigned count;
    char *pop;
    dchar *ss;
    Match *psave;
    unsigned c1;
    unsigned c2;
    int pc;

    pc = prog - program;
#if defined(UNICODE)
    WPRINTF(L"RegExp::match(prog[%d], src = '%ls', end = [%d])\n", pc, src, progend ? progend - program : -1);
#else
    //PRINTF("RegExp::match(prog = %p, src = '%s', end = %p)\n", prog, src, progend);
#endif
    srcsave = src;
    psave = NULL;
    for (;;)
    {
	if (prog == progend)		// if done matching
	{
	    PRINTF("\tprogend\n");
	    return 1;
	}

	//PRINTF("\top = %d\n", *prog);
	switch (*prog)
	{
	    case REchar:
		WPRINTF(L"\tREchar '%c', src = '%c'\n", *(unsigned char *)(prog + 1), Dchar::get(src));
		if (*(unsigned char *)(prog + 1) != Dchar::get(src))
		    goto Lnomatch;
		src = Dchar::inc(src);
		prog += 1 + sizeof(char);
		break;

	    case REichar:
		PRINTF("\tREichar '%c', src = '%c'\n", *(unsigned char *)(prog + 1), Dchar::get(src));
		c1 = *(unsigned char *)(prog + 1);
		c2 = Dchar::get(src);
		if (c1 != c2)
		{
		    if (Dchar::isLower((dchar)c2))
			c2 = Dchar::toUpper((dchar)c2);
		    else
			goto Lnomatch;
		    if (c1 != c2)
			goto Lnomatch;
		}
		src = Dchar::inc(src);
		prog += 1 + sizeof(char);
		break;

	    case REwchar:
		PRINTF("\tREwchar '%c', src = '%c'\n", *(wchar_t *)(prog + 1), Dchar::get(src));
		if (*(wchar_t *)(prog + 1) != Dchar::get(src))
		    goto Lnomatch;
		src = Dchar::inc(src);
		prog += 1 + sizeof(wchar_t);
		break;

	    case REiwchar:
		PRINTF("\tREiwchar '%c', src = '%c'\n", *(wchar_t *)(prog + 1), Dchar::get(src));
		c1 = *(wchar_t *)(prog + 1);
		c2 = Dchar::get(src);
		if (c1 != c2)
		{
		    if (Dchar::isLower((dchar)c2))
			c2 = Dchar::toUpper((dchar)c2);
		    else
			goto Lnomatch;
		    if (c1 != c2)
			goto Lnomatch;
		}
		src = Dchar::inc(src);
		prog += 1 + sizeof(wchar_t);
		break;

	    case REanychar:
		PRINTF("\tREanychar\n");
		if (!*src)
		    goto Lnomatch;
		if (!(attributes & REAdotmatchlf) && *src == '\n')
		    goto Lnomatch;
		src = Dchar::inc(src);
		prog++;
		break;

	    case REstring:
		//WPRINTF(L"\tREstring x%x, '%c'\n", *(unsigned *)(prog + 1),
		//	*(dchar *)(prog + 1 + sizeof(unsigned)));
		len = *(unsigned *)(prog + 1);
		if (memcmp(prog + 1 + sizeof(unsigned), src, len * sizeof(dchar)))
		    goto Lnomatch;
		src += len;
		prog += 1 + sizeof(unsigned) + len * sizeof(dchar);
		break;

	    case REtestbit:
		PRINTF("\tREtestbit %d, %d, '%c', x%02x\n",
		    ((unsigned short *)(prog + 1))[0],
		    ((unsigned short *)(prog + 1))[1], Dchar::get(src), Dchar::get(src));
		len = ((unsigned short *)(prog + 1))[1];
		c1 = Dchar::get(src);
		//PRINTF("[x%02x]=x%02x, x%02x\n", c1 >> 3, ((prog + 1 + 4)[c1 >> 3] ), (1 << (c1 & 7)));
		if (c1 <= ((unsigned short *)(prog + 1))[0] &&
		    !((prog + 1 + 4)[c1 >> 3] & (1 << (c1 & 7))))
		    goto Lnomatch;
		prog += 1 + 2 * sizeof(unsigned short) + len;
		break;

	    case REbit:
		PRINTF("\tREbit %d, %d, '%c'\n",
		    ((unsigned short *)(prog + 1))[0],
		    ((unsigned short *)(prog + 1))[1], Dchar::get(src));
		len = ((unsigned short *)(prog + 1))[1];
		c1 = Dchar::get(src);
		if (c1 > ((unsigned short *)(prog + 1))[0])
		    goto Lnomatch;
		if (!((prog + 1 + 4)[c1 >> 3] & (1 << (c1 & 7))))
		    goto Lnomatch;
		src = Dchar::inc(src);
		prog += 1 + 2 * sizeof(unsigned short) + len;
		break;

	    case REnotbit:
		PRINTF("\tREnotbit %d, %d, '%c'\n",
		    ((unsigned short *)(prog + 1))[0],
		    ((unsigned short *)(prog + 1))[1], Dchar::get(src));
		len = ((unsigned short *)(prog + 1))[1];
		c1 = Dchar::get(src);
		if (c1 <= ((unsigned short *)(prog + 1))[0] &&
		    ((prog + 1 + 4)[c1 >> 3] & (1 << (c1 & 7))))
		    goto Lnomatch;
		src = Dchar::inc(src);
		prog += 1 + 2 * sizeof(unsigned short) + len;
		break;

	    case RErange:
		PRINTF("\tRErange %d\n", *(unsigned *)(prog + 1));
		// BUG: REAignoreCase?
		len = *(unsigned *)(prog + 1);
		if (memchr(prog + 1 + sizeof(unsigned), Dchar::get(src), len) == NULL)
		    goto Lnomatch;
		src = Dchar::inc(src);
		prog += 1 + sizeof(unsigned) + len;
		break;

	    case REnotrange:
		PRINTF("\tREnotrange %d\n", *(unsigned *)(prog + 1));
		// BUG: REAignoreCase?
		len = *(unsigned *)(prog + 1);
		if (memchr(prog + 1 + sizeof(unsigned), Dchar::get(src), len) != NULL)
		    goto Lnomatch;
		src = Dchar::inc(src);
		prog += 1 + sizeof(unsigned) + len;
		break;

	    case REbol:
		PRINTF("\tREbol\n");
		if (src == input)
		    ;
		else if (attributes & REAmultiline &&
			 Dchar::getprev(input, src) == '\n')
		    ;
		else
		    goto Lnomatch;
		prog++;
		break;

	    case REeol:
		PRINTF("\tREeol\n");
		if (*src == 0)
		    ;
		else if (attributes & REAmultiline && *src == '\n')
		    src = Dchar::inc(src);
		else
		    goto Lnomatch;
		prog++;
		break;

	    case REor:
		PRINTF("\tREor %d\n", ((unsigned *)(prog + 1))[0]);
		len = ((unsigned *)(prog + 1))[0];
		pop = prog + 1 + sizeof(unsigned);
		ss = src;
		if (match(pop, progend))
		{
		    if (progend)
		    {	dchar *s;

			s = src;
			if (match(progend, NULL))
			{
			    PRINTF("\tfirst operand matched\n");
			    src = s;
			    return 1;
			}
			else
			{
			    // If second branch doesn't match to end, take first anyway
			    src = ss;
			    if (!match(pop + len, NULL))
			    {
				PRINTF("\tfirst operand matched\n");
				src = s;
				return 1;
			    }
			}
			src = ss;
		    }
		    else
		    {
			PRINTF("\tfirst operand matched\n");
			return 1;
		    }
		}
		prog = pop + len;	// proceed with 2nd branch
		break;

	    case REgoto:
		PRINTF("\tREgoto\n");
		len = ((unsigned *)(prog + 1))[0];
		prog += 1 + sizeof(unsigned) + len;
		break;

	    case REanystar:
		PRINTF("\tREanystar\n");
		prog++;
		for (;;)
		{   dchar *s1;
		    dchar *s2;

		    s1 = src;
		    if (!*src)
			break;
		    if (!(attributes & REAdotmatchlf) && *src == '\n')
			break;
		    src = Dchar::inc(src);
		    s2 = src;

		    // If no match after consumption, but it
		    // did match before, then no match
		    if (!match(prog, NULL))
		    {
			src = s1;
			// BUG: should we save/restore parens[]?
			if (match(prog, NULL))
			{
			    src = s1;		// no match
			    break;
			}
		    }
		    src = s2;
		}
		break;

	    case REnm:
	    case REnmq:
		// len, n, m, ()
		len = ((unsigned *)(prog + 1))[0];
		n = ((unsigned *)(prog + 1))[1];
		m = ((unsigned *)(prog + 1))[2];
		WPRINTF(L"\tREnm%ls len=%d, n=%u, m=%u\n", (*prog == REnmq) ? L"q" : L"", len, n, m);
		pop = prog + 1 + sizeof(unsigned) * 3;
		for (count = 0; count < n; count++)
		{
		    if (!match(pop, pop + len))
			goto Lnomatch;
		}
		if (!psave && count < m)
		{
		    psave = (Match *)alloca(nparens * sizeof(Match));
		}
		if (*prog == REnmq)	// if minimal munch
		{
		    for (; count < m; count++)
		    {   dchar *s1;

			memcpy(psave, parens, nparens * sizeof(Match));
			s1 = src;

			if (match(pop + len, NULL))
			{
			    src = s1;
			    memcpy(parens, psave, nparens * sizeof(Match));
			    break;
			}

			if (!match(pop, pop + len))
			{
			    PRINTF("\tdoesn't match subexpression\n");
			    break;
			}

			// If source is not consumed, don't
			// infinite loop on the match
			if (s1 == src)
			{
			    PRINTF("\tsource is not consumed\n");
			    break;
			}
		    }
		}
		else	// maximal munch
		{
		    for (; count < m; count++)
		    {   dchar *s1;
			dchar *s2;

			memcpy(psave, parens, nparens * sizeof(Match));
			s1 = src;
			if (!match(pop, pop + len))
			{
			    PRINTF("\tdoesn't match subexpression\n");
			    break;
			}
			s2 = src;

			// If source is not consumed, don't
			// infinite loop on the match
			if (s1 == s2)
			{
			    PRINTF("\tsource is not consumed\n");
			    break;
			}

			// If no match after consumption, but it
			// did match before, then no match
			if (!match(pop + len, NULL))
			{
			    src = s1;
			    if (match(pop + len, NULL))
			    {
				src = s1;		// no match
				memcpy(parens, psave, nparens * sizeof(Match));
				break;
			    }
			}
			src = s2;
		    }
		}
		PRINTF("\tREnm len=%d, n=%u, m=%u, DONE count=%d\n", len, n, m, count);
		prog = pop + len;
		break;

	    case REparen:
		// len, ()
		PRINTF("\tREparen\n");
		len = ((unsigned *)(prog + 1))[0];
		n = ((unsigned *)(prog + 1))[1];
		pop = prog + 1 + sizeof(unsigned) * 2;
		ss = src;
		if (!match(pop, pop + len))
		    goto Lnomatch;
		parens[n].istart = ss - input;
		parens[n].iend = src - input;
		prog = pop + len;
		break;

	    case REend:
		PRINTF("\tREend\n");
		return 1;		// successful match

	    case REwordboundary:
		PRINTF("\tREwordboundary\n");
		if (src != input)
		{
		    c1 = Dchar::getprev(input,src);
		    c2 = Dchar::get(src);
		    if (!(
			  (isword((dchar)c1) && !isword((dchar)c2)) ||
			  (!isword((dchar)c1) && isword((dchar)c2))
			 )
		       )
			goto Lnomatch;
		}
		prog++;
		break;

	    case REnotwordboundary:
		PRINTF("\tREnotwordboundary\n");
		if (src == input)
		    goto Lnomatch;
		c1 = Dchar::getprev(input,src);
		c2 = Dchar::get(src);
		if (
		    (isword((dchar)c1) && !isword((dchar)c2)) ||
		    (!isword((dchar)c1) && isword((dchar)c2))
		   )
		    goto Lnomatch;
		prog++;
		break;

	    case REdigit:
		PRINTF("\tREdigit\n");
		if (!Dchar::isDigit(Dchar::get(src)))
		    goto Lnomatch;
		src = Dchar::inc(src);
		prog++;
		break;

	    case REnotdigit:
		PRINTF("\tREnotdigit\n");
		c1 = Dchar::get(src);
		if (!c1 || Dchar::isDigit((dchar)c1))
		    goto Lnomatch;
		src = Dchar::inc(src);
		prog++;
		break;

	    case REspace:
		PRINTF("\tREspace\n");
		if (!isspace(Dchar::get(src)))
		    goto Lnomatch;
		src = Dchar::inc(src);
		prog++;
		break;

	    case REnotspace:
		PRINTF("\tREnotspace\n");
		c1 = Dchar::get(src);
		if (!c1 || isspace(c1))
		    goto Lnomatch;
		src = Dchar::inc(src);
		prog++;
		break;

	    case REword:
		PRINTF("\tREword\n");
		if (!isword(Dchar::get(src)))
		    goto Lnomatch;
		src = Dchar::inc(src);
		prog++;
		break;

	    case REnotword:
		PRINTF("\tREnotword\n");
		c1 = Dchar::get(src);
		if (!c1 || isword((dchar)c1))
		    goto Lnomatch;
		src = Dchar::inc(src);
		prog++;
		break;

	    case REbackref:
		PRINTF("\tREbackref %d\n", prog[1]);
		n = prog[1];
		len = parens[n].iend - parens[n].istart;
		if (attributes & REAignoreCase)
		{
#if defined(linux)
		    if (strncasecmp((char*)src, (char*)(input + parens[n].istart), len))
			goto Lnomatch;
#elif defined(_WIN32)
#if defined(UNICODE)
		    if (_wcsnicmp(src, input + parens[n].istart, len))
#else
		    if (_memicmp(src, input + parens[n].istart, len))
#endif // UNICODE
			goto Lnomatch;
#endif // _WIN32
		}
		else if (memcmp(src, input + parens[n].istart, len * sizeof(dchar)))
		    goto Lnomatch;
		src += len;
		prog += 2;
		break;

	    default:
		assert(0);
	}
    }

Lnomatch:
    WPRINTF(L"\tnomatch pc=%d\n", pc);
    src = srcsave;
    return 0;
}

/* =================== Compiler ================== */

int RegExp::parseRegexp()
{   unsigned offset;
    unsigned gotooffset;
    unsigned len1;
    unsigned len2;

    //PRINTF(L"parseRegexp() '%ls'\n", p);
    offset = buf->offset;
    for (;;)
    {
	switch (*p)
	{
	    case ')':
		return 1;

	    case 0:
		buf->writeByte(REend);
		return 1;

	    case '|':
		p++;
		gotooffset = buf->offset;
		buf->writeByte(REgoto);
		buf->write4(0);
		len1 = buf->offset - offset;
		buf->spread(offset, 1 + sizeof(unsigned));
		gotooffset += 1 + sizeof(unsigned);
		parseRegexp();
		len2 = buf->offset - (gotooffset + 1 + sizeof(unsigned));
		buf->data[offset] = REor;
		((unsigned *)(buf->data + offset + 1))[0] = len1;
		((unsigned *)(buf->data + gotooffset + 1))[0] = len2;
		break;

	    default:
		parsePiece();
		break;
	}
    }
}

int RegExp::parsePiece()
{   unsigned offset;
    unsigned len;
    unsigned n;
    unsigned m;
    char op;

    //WPRINTF(L"parsePiece() '%ls'\n", p);
    offset = buf->offset;
    parseAtom();
    switch (*p)
    {
	case '*':
	    // Special optimization: replace .* with REanystar
	    if (buf->offset - offset == 1 &&
		buf->data[offset] == REanychar &&
		p[1] != '?')
	    {
		buf->data[offset] = REanystar;
		p++;
		break;
	    }

	    n = 0;
	    m = inf;
	    goto Lnm;

	case '+':
	    n = 1;
	    m = inf;
	    goto Lnm;

	case '?':
	    n = 0;
	    m = 1;
	    goto Lnm;

	case '{':	// {n} {n,} {n,m}
	    p++;
	    if (!Dchar::isDigit(*p))
		goto Lerr;
	    n = 0;
	    do
	    {
		// BUG: handle overflow
		n = n * 10 + *p - '0';
		p++;
	    } while (Dchar::isDigit(*p));
	    if (*p == '}')		// {n}
	    {	m = n;
		goto Lnm;
	    }
	    if (*p != ',')
		goto Lerr;
	    p++;
	    if (*p == '}')		// {n,}
	    {	m = inf;
		goto Lnm;
	    }
	    if (!Dchar::isDigit(*p))
		goto Lerr;
	    m = 0;			// {n,m}
	    do
	    {
		// BUG: handle overflow
		m = m * 10 + *p - '0';
		p++;
	    } while (Dchar::isDigit(*p));
	    if (*p != '}')
		goto Lerr;
	    goto Lnm;

	Lnm:
	    p++;
	    op = REnm;
	    if (*p == '?')
	    {	op = REnmq;	// minimal munch version
		p++;
	    }
	    len = buf->offset - offset;
	    buf->spread(offset, 1 + sizeof(unsigned) * 3);
	    buf->data[offset] = op;
	    ((unsigned *)(buf->data + offset + 1))[0] = len;
	    ((unsigned *)(buf->data + offset + 1))[1] = n;
	    ((unsigned *)(buf->data + offset + 1))[2] = m;
	    break;
    }
    return 1;

Lerr:
    error("badly formed {n,m}");
    return 0;
}

int RegExp::parseAtom()
{   char op;
    unsigned offset;
    unsigned c;

    //WPRINTF(L"parseAtom() '%ls'\n", p);
    c = *p;
    switch (c)
    {
	case '*':
	case '+':
	case '?':
	    error("*+? not allowed in atom");
	    p++;
	    return 0;

	case '(':
	    p++;
	    buf->writeByte(REparen);
	    offset = buf->offset;
	    buf->write4(0);		// reserve space for length
	    buf->write4(nparens);
	    nparens++;
	    parseRegexp();
	    *(unsigned *)(buf->data + offset) =
		buf->offset - (offset + sizeof(unsigned) * 2);
	    if (*p != ')')
	    {
		error("')' expected");
		return 0;
	    }
	    p++;
	    break;

	case '[':
	    if (!parseRange())
		return 0;
	    break;

	case '.':
	    p++;
	    buf->writeByte(REanychar);
	    break;

	case '^':
	    p++;
	    buf->writeByte(REbol);
	    break;

	case '$':
	    p++;
	    buf->writeByte(REeol);
	    break;

	case 0:
	    break;

	case '\\':
	    p++;
	    switch (*p)
	    {
		case 0:
		    error("no character past '\\'");
		    return 0;

		case 'b':    op = REwordboundary;	goto Lop;
		case 'B':    op = REnotwordboundary;	goto Lop;
		case 'd':    op = REdigit;		goto Lop;
		case 'D':    op = REnotdigit;		goto Lop;
		case 's':    op = REspace;		goto Lop;
		case 'S':    op = REnotspace;		goto Lop;
		case 'w':    op = REword;		goto Lop;
		case 'W':    op = REnotword;		goto Lop;

		Lop:
		    buf->writeByte(op);
		    p++;
		    break;

		case 'f':
		case 'n':
		case 'r':
		case 't':
		case 'v':
		case 'c':
		case 'x':
		case 'u':
		case '0':
		    c = escape();
		    goto Lbyte;

		case '1': case '2': case '3':
		case '4': case '5': case '6':
		case '7': case '8': case '9':
		    c = *p - '1';
		    if (c < nparens)
		    {	buf->writeByte(REbackref);
			buf->writeByte(c);
		    }
		    else
		    {	error("no matching back reference");
			return 0;
		    }
		    p++;
		    break;

		default:
		    c = Dchar::get(p);
		    p = Dchar::inc(p);
		    goto Lbyte;
	    }
	    break;

	default:
	    c = Dchar::get(p);
	    p = Dchar::inc(p);
	Lbyte:
	    op = REchar;
	    if (attributes & REAignoreCase)
	    {
		if (Dchar::isAlpha((dchar)c))
		{
		    op = REichar;
		    c = Dchar::toUpper((dchar)c);
		}
	    }
	    if (op == REchar && c <= 0xFF)
	    {
		// Look ahead and see if we can make this into
		// an REstring
		dchar *q;
		int len;

		for (q = p; ; q = Dchar::inc(q))
		{   dchar qc = (dchar)Dchar::get(q);

		    switch (qc)
		    {
			case '{':
			case '*':
			case '+':
			case '?':
			    if (q == p)
				goto Lchar;
			    q--;
			    break;

			case '(':	case ')':
			case '|':
			case '[':	case ']':
			case '.':	case '^':
			case '$':	case '\\':
			case '}':	case 0:
			    break;

			default:
			    continue;
		    }
		    break;
		}
		len = q - p;
		if (len > 0)
		{
		    WPRINTF(L"writing string len %d, c = '%c', *p = '%c'\n", len+1, c, *p);
		    buf->reserve(5 + (1 + len) * sizeof(dchar));
		    buf->writeByte(REstring);
		    buf->write4(len + 1);
		    buf->writedchar(c);
		    buf->write(p, len * sizeof(dchar));
		    p = q;
		    break;
		}
	    }
	    if (c & ~0xFF)
	    {
		// Convert to 16 bit opcode
		op = (char)((op == REchar) ? REwchar : REiwchar);
		buf->writeByte(op);
		buf->writeword(c);
	    }
	    else
	    {
	     Lchar:
PRINTF("It's an REchar '%c'\n", c);
		buf->writeByte(op);
		buf->writeByte(c);
	    }
	    break;
    }
    return 1;
}

#if 1

struct Range
{
    unsigned maxc;
    unsigned maxb;
    OutBuffer *buf;
    unsigned char *base;

    Range(OutBuffer *buf)
    {
	this->buf = buf;
	this->base = &buf->data[buf->offset];
	maxc = 0;
	maxb = 0;
    }

#if _MSC_VER
    // Disable useless warnings about not inlining setbitmax
#pragma warning (disable : 4710)
#endif

    void setbitmax(unsigned u)
    {   unsigned b;

	if (u > maxc)
	{
	    maxc = u;
	    b = u >> 3;
	    if (b >= maxb)
	    {	unsigned u;

		u = base - buf->data;
		buf->fill0(b - maxb + 1);
		base = buf->data + u;
		maxb = b + 1;
	    }
	}
    }

#undef setbit
    void setbit(unsigned u)
    {
	//PRINTF("setbit x%02x\n", 1 << (u & 7));
	base[u >> 3] |= 1 << (u & 7);
    }

    void setbit2(unsigned u)
    {
	setbitmax(u + 1);
	//PRINTF("setbit2 [x%02x] |= x%02x\n", u >> 3, 1 << (u & 7));
	base[u >> 3] |= 1 << (u & 7);
    }

    int testbit(unsigned u)
    {
	return base[u >> 3] & (1 << (u & 7));
    }

    void negate()
    {
	for (unsigned b = 0; b < maxb; b++)
	{
	    base[b] = (unsigned char)~base[b];
	}
    }
};

int RegExp::parseRange()
{   char op;
    int c = -1;		// initialize to keep /W4 happy
    int c2 = -1;	// initialize to keep /W4 happy
    unsigned i;
    unsigned cmax;
    unsigned offset;

    cmax = 0x7F;
    p++;
    op = REbit;
    if (*p == '^')
    {   p++;
	op = REnotbit;
    }
    buf->writeByte(op);
    offset = buf->offset;
    buf->write4(0);		// reserve space for length
    buf->reserve(128 / 8);
    Range r(buf);
    if (op == REnotbit)
	r.setbit2(0);
    switch (*p)
    {
	case ']':
	case '-':
	    c = *p;
	    p = Dchar::inc(p);
	    r.setbit2(c);
	    break;
    }

    enum RangeState { RSstart, RSrliteral, RSdash };
    RangeState rs;

    rs = RSstart;
    for (;;)
    {
	switch (*p)
	{
	    case ']':
		switch (rs)
		{
		    case RSdash:
			r.setbit2('-');
		    case RSrliteral:
			r.setbit2(c);
			break;
		}
		p++;
		break;

	    case '\\':
		p++;
		r.setbitmax(cmax);
		switch (*p)
		{
		    case 'd':
			for (i = '0'; i <= '9'; i++)
			    r.setbit(i);
			goto Lrs;

		    case 'D':
			for (i = 1; i < '0'; i++)
			    r.setbit(i);
			for (i = '9' + 1; i <= cmax; i++)
			    r.setbit(i);
			goto Lrs;

		    case 's':
			for (i = 0; i <= cmax; i++)
			    if (isspace(i))
				r.setbit(i);
			goto Lrs;

		    case 'S':
			for (i = 1; i <= cmax; i++)
			    if (!isspace(i))
				r.setbit(i);
			goto Lrs;

		    case 'w':
			for (i = 0; i <= cmax; i++)
			    if (isword((dchar)i))
				r.setbit(i);
			goto Lrs;

		    case 'W':
			for (i = 1; i <= cmax; i++)
			    if (!isword((dchar)i))
				r.setbit(i);
			goto Lrs;

		    Lrs:
			switch (rs)
			{   case RSdash:
				r.setbit2('-');
			    case RSrliteral:
				r.setbit2(c);
				break;
			}
			rs = RSstart;
			continue;
		}
		c2 = escape();
		goto Lrange;

	    case '-':
		p++;
		if (rs == RSstart)
		    goto Lrange;
		else if (rs == RSrliteral)
		    rs = RSdash;
		else if (rs == RSdash)
		{
		    r.setbit2(c);
		    r.setbit2('-');
		    rs = RSstart;
		}
		continue;

	    case 0:
		error("']' expected");
		return 0;

	    default:
		c2 = Dchar::get(p);
		p = Dchar::inc(p);
	    Lrange:
		switch (rs)
		{   case RSrliteral:
			r.setbit2(c);
		    case RSstart:
			c = c2;
			rs = RSrliteral;
			break;

		    case RSdash:
			if (c > c2)
			{   error("inverted range in character class");
			    return 0;
			}
			r.setbitmax(c2);
			//PRINTF("c = %x, c2 = %x\n",c,c2);
			for (; c <= c2; c++)
			    r.setbit(c);
			rs = RSstart;
			break;
		}
		continue;
	}
	break;
    }
    //PRINTF("maxc = %d, maxb = %d\n",r.maxc,r.maxb);
    ((unsigned short *)(buf->data + offset))[0] = (unsigned short)r.maxc;
    ((unsigned short *)(buf->data + offset))[1] = (unsigned short)r.maxb;
    if (attributes & REAignoreCase)
    {
	// BUG: what about unicode?
	r.setbitmax(0x7F);
	for (c = 'a'; c <= 'z'; c++)
	{
	    if (r.testbit(c))
		r.setbit(c + 'A' - 'a');
	    else if (r.testbit(c + 'A' - 'a'))
		r.setbit(c);
	}
    }
    return 1;
}
#else
int RegExp::parseRange()
{   char op;
    unsigned offset;
    dchar c;
    dchar c1;
    dchar c2;

    p++;
    op = RErange;
    if (*p == '^')
    {   p++;
	op = REnotrange;
    }
    buf->writeByte(op);
    offset = buf->offset;
    buf->write4(0);		// reserve space for length
    switch (*p)
    {
	case ']':
	case '-':
	    buf->writeByte(*p++);
	    break;
    }
    for (;;)
    {
	switch (*p)
	{
	    case ']':
		p++;
		break;

	    case '-':
		p++;
		c2 = p[0];
		if (c2 == ']')		// if "-]"
		{   buf->writeByte('-');
		    continue;
		}
		c1 = p[-2];
		if (c1 > c2)
		{   error("range not in order");
		    return 0;
		}
		for (c1++; c1 <= c2; c1++)
		    buf->writeByte(c1);
		p++;
		continue;

	    case '\\':
		p++;
#if 0 // BUG: should handle these cases
		switch (*p)
		{
		    case 'd':    op = REdigit;
		    case 'D':    op = REnotdigit;
		    case 's':    op = REspace;
		    case 'S':    op = REnotspace;
		    case 'w':    op = REword;
		    case 'W':    op = REnotword;
		}
#endif
		c = escape();
		buf->writeByte(c);
		break;

	    case 0:
		error("']' expected");
		return 0;

	    default:
		buf->writeByte(*p++);
		continue;
	}
	break;
    }
    *(unsigned *)(buf->data + offset) =
	buf->offset - (offset + sizeof(unsigned));
    return 1;
}
#endif // !_MSC_VER

void RegExp::error(char *msg)
{
    (void) msg;			// satisfy /W4
    p += Dchar::len(p);	// advance to terminating 0
    PRINTF("RegExp.compile() error: %s\n", msg);
    errors++;
}

// p is following the \ char
int RegExp::escape()
{   int c;
    int i;

    c = *p;		// none of the cases are multibyte
    switch (c)
    {
	case 'b':    c = '\b';	break;
	case 'f':    c = '\f';	break;
	case 'n':    c = '\n';	break;
	case 'r':    c = '\r';	break;
	case 't':    c = '\t';	break;
	case 'v':    c = '\v';	break;

	// BUG: Perl does \a and \e too, should we?

	case 'c':
	    p = Dchar::inc(p);
	    c = Dchar::get(p);
	    // Note: we are deliberately not allowing Unicode letters
	    if (!(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')))
	    {   error("letter expected following \\c");
		return 0;
	    }
	    c &= 0x1F;
	    break;

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	    c -= '0';
	    for (i = 0; i < 2; i++)
	    {
		p++;
		if ('0' <= *p && *p <= '7')
		{   c = c * 8 + (*p - '0');
		    // Treat overflow as if last
		    // digit was not an octal digit
		    if (c >= 0xFF)
		    {	c >>= 3;
			return c;
		    }
		}
		else
		    return c;
	    }
	    break;

	case 'x':
	    c = 0;
	    for (i = 0; i < 2; i++)
	    {
		p++;
		if ('0' <= *p && *p <= '9')
		    c = c * 16 + (*p - '0');
		else if ('a' <= *p && *p <= 'f')
		    c = c * 16 + (*p - 'a' + 10);
		else if ('A' <= *p && *p <= 'F')
		    c = c * 16 + (*p - 'A' + 10);
		else if (i == 0)	// if no hex digits after \x
		{
		    // Not a valid \xXX sequence
		    return 'x';
		}
		else
		    return c;
	    }
	    break;

	case 'u':
	    c = 0;
	    for (i = 0; i < 4; i++)
	    {
		p++;
		if ('0' <= *p && *p <= '9')
		    c = c * 16 + (*p - '0');
		else if ('a' <= *p && *p <= 'f')
		    c = c * 16 + (*p - 'a' + 10);
		else if ('A' <= *p && *p <= 'F')
		    c = c * 16 + (*p - 'A' + 10);
		else
		{
		    // Not a valid \uXXXX sequence
		    p -= i;
		    return 'u';
		}
	    }
	    break;

	default:
	    c = Dchar::get(p);
	    p = Dchar::inc(p);
	    return c;
    }
    p++;
    return c;
}

/* ==================== optimizer ======================= */

void RegExp::optimize()
{   char *prog;

    PRINTF("RegExp::optimize()\n");
    prog = (char *)buf->data;
    for (;;)
    {
	switch (*prog)
	{
	    case REend:
	    case REanychar:
	    case REanystar:
	    case REbackref:
	    case REeol:
	    case REchar:
	    case REichar:
	    case REwchar:
	    case REiwchar:
	    case REstring:
	    case REtestbit:
	    case REbit:
	    case REnotbit:
	    case RErange:
	    case REnotrange:
	    case REwordboundary:
	    case REnotwordboundary:
	    case REdigit:
	    case REnotdigit:
	    case REspace:
	    case REnotspace:
	    case REword:
	    case REnotword:
		return;

	    case REbol:
		prog++;
		continue;

	    case REor:
	    case REnm:
	    case REnmq:
	    case REparen:
	    case REgoto:
	    {
		OutBuffer bitbuf;
		unsigned offset;
		Range r(&bitbuf);

		offset = prog - (char *)buf->data;
		if (startchars(&r, prog, NULL))
		{
		    PRINTF("\tfilter built\n");
		    buf->spread(offset, 1 + 4 + r.maxb);
		    buf->data[offset] = REtestbit;
		    ((unsigned short *)(buf->data + offset + 1))[0] = (unsigned short)r.maxc;
		    ((unsigned short *)(buf->data + offset + 1))[1] = (unsigned short)r.maxb;
		    memcpy(buf->data + offset + 1 + 4, r.base, r.maxb);
		}
		return;
	    }
	    default:
		assert(0);
	}
    }
}

/////////////////////////////////////////
// OR the leading character bits into r.
// Limit the character range from 0..7F,
// match() will allow through anything over maxc.
// Return 1 if success, 0 if we can't build a filter or
// if there is no point to one.

int RegExp::startchars(Range *r, char *prog, char *progend)
{   unsigned c;
    unsigned maxc;
    unsigned maxb;
    unsigned len;
    unsigned b;
    unsigned n;
    unsigned m;
    char *pop;

    //PRINTF("RegExp::startchars(prog = %p, progend = %p)\n", prog, progend);
    for (;;)
    {
	if (prog == progend)
	    return 1;
	switch (*prog)
	{
	    case REchar:
		c = *(unsigned char *)(prog + 1);
		if (c <= 0x7F)
		    r->setbit2(c);
		return 1;

	    case REichar:
		c = *(unsigned char *)(prog + 1);
		if (c <= 0x7F)
		{   r->setbit2(c);
		    r->setbit2(Dchar::toLower((dchar)c));
		}
		return 1;

	    case REwchar:
	    case REiwchar:
		return 1;

	    case REanychar:
		return 0;		// no point

	    case REstring:
		PRINTF("\tREstring %d, '%c'\n", *(unsigned *)(prog + 1),
			*(dchar *)(prog + 1 + sizeof(unsigned)));
		len = *(unsigned *)(prog + 1);
		assert(len);
		c = *(dchar *)(prog + 1 + sizeof(unsigned));
		if (c <= 0x7F)
		    r->setbit2(c);
		return 1;

	    case REtestbit:
	    case REbit:
		maxc = ((unsigned short *)(prog + 1))[0];
		maxb = ((unsigned short *)(prog + 1))[1];
		if (maxc <= 0x7F)
		    r->setbitmax(maxc);
		else
		    maxb = r->maxb;
		for (b = 0; b < maxb; b++)
		    r->base[b] |= *(char *)(prog + 1 + 4 + b);
		return 1;

	    case REnotbit:
		maxc = ((unsigned short *)(prog + 1))[0];
		maxb = ((unsigned short *)(prog + 1))[1];
		if (maxc <= 0x7F)
		    r->setbitmax(maxc);
		else
		    maxb = r->maxb;
		for (b = 0; b < maxb; b++)
		    r->base[b] |= ~*(char *)(prog + 1 + 4 + b);
		return 1;

	    case REbol:
	    case REeol:
		return 0;

	    case REor:
		len = ((unsigned *)(prog + 1))[0];
		pop = prog + 1 + sizeof(unsigned);
		return startchars(r, pop, progend) &&
		       startchars(r, pop + len, progend);

	    case REgoto:
		len = ((unsigned *)(prog + 1))[0];
		prog += 1 + sizeof(unsigned) + len;
		break;

	    case REanystar:
		return 0;

	    case REnm:
	    case REnmq:
		// len, n, m, ()
		len = ((unsigned *)(prog + 1))[0];
		n = ((unsigned *)(prog + 1))[1];
		m = ((unsigned *)(prog + 1))[2];
		pop = prog + 1 + sizeof(unsigned) * 3;
		if (!startchars(r, pop, pop + len))
		    return 0;
		if (n)
		    return 1;
		prog = pop + len;
		break;

	    case REparen:
		// len, ()
		len = ((unsigned *)(prog + 1))[0];
		n = ((unsigned *)(prog + 1))[1];
		pop = prog + 1 + sizeof(unsigned) * 2;
		return startchars(r, pop, pop + len);

	    case REend:
		return 0;

	    case REwordboundary:
	    case REnotwordboundary:
		return 0;

	    case REdigit:
		r->setbitmax('9');
		for (c = '0'; c <= '9'; c++)
		    r->setbit(c);
		return 1;

	    case REnotdigit:
		r->setbitmax(0x7F);
		for (c = 0; c <= '0'; c++)
		    r->setbit(c);
		for (c = '9' + 1; c <= r->maxc; c++)
		    r->setbit(c);
		return 1;

	    case REspace:
		r->setbitmax(0x7F);
		for (c = 0; c <= r->maxc; c++)
		    if (isspace(c))
			r->setbit(c);
		return 1;

	    case REnotspace:
		r->setbitmax(0x7F);
		for (c = 0; c <= r->maxc; c++)
		    if (!isspace(c))
			r->setbit(c);
		return 1;

	    case REword:
		r->setbitmax(0x7F);
		for (c = 0; c <= r->maxc; c++)
		    if (isword((dchar)c))
			r->setbit(c);
		return 1;

	    case REnotword:
		r->setbitmax(0x7F);
		for (c = 0; c <= r->maxc; c++)
		    if (!isword((dchar)c))
			r->setbit(c);
		return 1;

	    case REbackref:
		return 0;

	    default:
		assert(0);
	}
    }
}

/* ==================== replace ======================= */

// After locating a match via RegExp::test, create a string based on
//      a passed format string that references the match data

// This version of replace() uses:
//	&	replace with the match
//	\n	replace with the nth parenthesized match, n is 1..9
//	\c	replace with char c

dchar *RegExp::replace(dchar *format)
{
    OutBuffer buf;
    dchar *result;
    unsigned c;

    buf.reserve((Dchar::len(format) + 1) * sizeof(dchar));
    for (; ; format = Dchar::inc(format))
    {
	c = Dchar::get(format);
	switch (c)
	{
	    case 0:
		break;

	    case '&':
		buf.write(input + text.istart, (text.iend - text.istart) * sizeof(dchar));
		continue;

	    case '\\':
		format = Dchar::inc(format);
		c = Dchar::get(format);
		if (c >= '1' && c <= '9')
		{   unsigned i;

		    i = c - '1';
		    if (i < nparens)
			buf.write(input + parens[i].istart,
				(parens[i].iend - parens[i].istart) * sizeof(dchar));
		}
		else if (!c)
		    break;
		else
		    buf.writedchar(c);
		continue;

	    default:
		buf.writedchar(c);
		continue;
	}
	break;
    }
    buf.writedchar(0);
    result = (dchar *)buf.data;
    buf.data = NULL;
    return result;
}

// ECMA v3 15.5.4.11
// This version of replace uses:
//	$$	$
//	$&	The matched substring.
//	$`	The portion of string that precedes the matched substring.
//	$'	The portion of string that follows the matched substring.
//	$n	The nth capture, where n is a single digit 1-9
//		and $n is not followed by a decimal digit.
//		If n<=m and the nth capture is undefined, use the empty
//		string instead. If n>m, the result is implementation-
//		defined.
//	$nn	The nnth capture, where nn is a two-digit decimal
//		number 01-99.
//		If n<=m and the nth capture is undefined, use the empty
//		string instead. If n>m, the result is implementation-
//		defined.
//
//	Any other $ are left as is.

dchar *RegExp::replace2(dchar *format)
{
    return replace3(format, input, &text, nparens, parens);
}

// Static version that doesn't require a RegExp object to be created

dchar *RegExp::replace3(dchar *format, dchar *input, Match *text,
	unsigned nparens, Match *parens)
{
    OutBuffer buf;
    dchar *result;
    unsigned c;
    unsigned c2;
    int istart;
    int iend;
    int i;

//    WPRINTF(L"replace3(format = '%s', input = '%s', text = %p, nparens = %d, parens = %p\n",
//	format, input, text, nparens, parens);
    buf.reserve((Dchar::len(format) + 1) * sizeof(dchar));
    for (; ; format = Dchar::inc(format))
    {
	c = Dchar::get(format);
      L1:
	if (c == 0)
	    break;
	if (c != '$')
	{
	    buf.writedchar(c);
	    continue;
	}
	format = Dchar::inc(format);
	c = Dchar::get(format);
	switch (c)
	{
	    case 0:
		buf.writedchar('$');
		break;

	    case '&':
		istart = text->istart;
		iend = text->iend;
		goto Lstring;

	    case '`':
		istart = 0;
		iend = text->istart;
		goto Lstring;

	    case '\'':
		istart = text->iend;
		iend = Dchar::len(input);
		goto Lstring;

	    Lstring:
		buf.write(input + istart,
			  (iend - istart) * sizeof(dchar));
		continue;

	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
		c2 = format[1];
		if (c2 >= '0' && c2 <= '9')
		{   i = (c - '0') * 10 + (c2 - '0');
		    format = Dchar::inc(format);
		}
		else
		    i = c - '0';
		if (i == 0)
		{
		    buf.writedchar('$');
		    buf.writedchar(c);
		    c = c2;
		    goto L1;
		}

		i -= 1;
		if (i < (int)nparens)
		{   istart = parens[i].istart;
		    iend = parens[i].iend;
		    goto Lstring;
		}
		continue;

	    default:
		buf.writedchar('$');
		buf.writedchar(c);
		continue;
	}
	break;
    }
    buf.writedchar(0);		// terminate string
    result = (dchar *)buf.data;
    buf.data = NULL;
    return result;
}

////////////////////////////////////////////////////////
// Return a string that is [input] with [text] replaced by [replacement].

dchar *RegExp::replace4(dchar *input, Match *text, dchar *replacement)
{
    int input_len;
    int replacement_len;
    int result_len;
    dchar *result;

    input_len = Dchar::len(input);
    replacement_len = Dchar::len(replacement);
    result_len = input_len - (text->iend - text->istart) + replacement_len;
    result = (dchar *)mem.malloc((result_len + 1) * sizeof(dchar));
    memcpy(result, input, text->istart * sizeof(dchar));
    memcpy(result + text->istart, replacement, replacement_len * sizeof(dchar));
    memcpy(result + text->istart + replacement_len,
	input + text->iend,
	(input_len - text->iend) * sizeof(dchar));
    result[result_len] = 0;
    return result;
}

