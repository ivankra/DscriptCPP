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



#ifndef REGEXP_H
#define REGEXP_H

#include "dchar.h"

/*
	Escape sequences:

	\nnn starts out a 1, 2 or 3 digit octal sequence,
	where n is an octal digit. If nnn is larger than
	0377, then the 3rd digit is not part of the sequence
	and is not consumed.
	For maximal portability, use exactly 3 digits.

	\xXX starts out a 1 or 2 digit hex sequence. X
	is a hex character. If the first character after the \x
	is not a hex character, the value of the sequence is 'x'
	and the XX are not consumed.
	For maximal portability, use exactly 2 digits.

	\uUUUU is a unicode sequence. There are exactly
	4 hex characters after the \u, if any are not, then
	the value of the sequence is 'u', and the UUUU are not
	consumed.

	Character classes:

	[a-b], where a is greater than b, will produce
	an error.
 */

struct Match
{
    int istart;			// index of start of match
    int iend;			// index past end of match
};

struct Range;

struct RegExp : Object
{
    RegExp(dchar *pattern, dchar *attributes, int ref);
    ~RegExp();
    void mark();

    // statics:

    Match text;			// the matched text

    unsigned nparens;		// number of parenthesized matches
    Match *parens;		// array [nparens]

    dchar *input;		// the string to search

    // per instance:

    int ref;			// !=0 means don't make our own copy of pattern
    dchar *pattern;		// source text of the regular expression

    dchar flags[3 + 1];		// source text of the attributes parameter
				// (3 dchars max plus terminating 0)
    int errors;

    unsigned attributes;

    #define REAglobal	  1	// has the g attribute
    #define REAignoreCase 2	// has the i attribute
    #define REAmultiline  4	// if treat as multiple lines separated
				// by newlines, or as a single line
    #define REAdotmatchlf 8	// if . matches \n

    int compile(dchar *pattern, dchar *attributes, int ref);
    int test(dchar *string, int startindex = 0);

    dchar *replace(dchar *format);
    dchar *replace2(dchar *format);
    static dchar *replace3(dchar *format, dchar *input, Match *text,
	unsigned nparens, Match *parens);
    static dchar *replace4(dchar *input, Match *text, dchar *replacement);

private:
    dchar *src;			// current source pointer
    dchar *src_start;		// starting position for match
    dchar *p;			// position of parser in pattern

    char *program;
    OutBuffer *buf;

    void printProgram(char *prog);
    int match(char *prog, char *progend);
    int parseRegexp();
    int parsePiece();
    int parseAtom();
    int parseRange();
    int escape();
    void error(char *msg);
    void optimize();
    int startchars(Range *r, char *prog, char *progend);
};

#endif

