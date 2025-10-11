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

/* The test vectors in this file are altered from Henry Spencer's regexp
   test code. His copyright notice is:

        Copyright (c) 1986 by University of Toronto.
        Written by Henry Spencer.  Not derived from licensed software.

        Permission is granted to anyone to use this software for any
        purpose on any computer system, and to redistribute it freely,
        subject to the following restrictions:

        1. The author is not responsible for the consequences of use of
                this software, no matter how awful, even if they arise
                from defects in it.

        2. The origin of this software must not be misrepresented, either
                by explicit claim or by omission.

        3. Altered versions must be plainly marked as such, and must not
                be misrepresented as being the original software.


 */


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "dchar.h"
#include "root.h"
#include "mem.h"
#include "regexp.h"

struct TestVectors
{
    char *pattern;
    char *input;
    char *result;
    char *format;
    char *replace;
};

#if defined UNICODE
struct TestVectorsD
{
    dchar *pattern;
    dchar *input;
    dchar *result;
    dchar *format;
    dchar *replace;
};
#else
typedef TestVectors TestVectorsD;
#endif

TestVectors tv[] =
{
    {"(a)\\1",	"abaab","y",	"&",	"aa"},
    {"abc",	"abc",	"y",	"&",	"abc"},
    {"abc",	"xbc",	"n",	"-",	"-"},
    {"abc",	"axc",	"n",	"-",	"-"},
    {"abc",	"abx",	"n",	"-",	"-"},
    {"abc",	"xabcy","y",	"&",	"abc"},
    {"abc",	"ababc","y",	"&",	"abc"},
    {"ab*c",	"abc",	"y",	"&",	"abc"},
    {"ab*bc",	"abc",	"y",	"&",	"abc"},
    {"ab*bc",	"abbc",	"y",	"&",	"abbc"},
    {"ab*bc",	"abbbbc","y",	"&",	"abbbbc"},
    {"ab+bc",	"abbc",	"y",	"&",	"abbc"},
    {"ab+bc",	"abc",	"n",	"-",	"-"},
    {"ab+bc",	"abq",	"n",	"-",	"-"},
    {"ab+bc",	"abbbbc","y",	"&",	"abbbbc"},
    {"ab?bc",	"abbc",	"y",	"&",	"abbc"},
    {"ab?bc",	"abc",	"y",	"&",	"abc"},
    {"ab?bc",	"abbbbc","n",	"-",	"-"},
    {"ab?c",	"abc",	"y",	"&",	"abc"},
    {"^abc$",	"abc",	"y",	"&",	"abc"},
    {"^abc$",	"abcc",	"n",	"-",	"-"},
    {"^abc",	"abcc",	"y",	"&",	"abc"},
    {"^abc$",	"aabc",	"n",	"-",	"-"},
    {"abc$",	"aabc",	"y",	"&",	"abc"},
    {"^",	"abc",	"y",	"&",	""},
    {"$",	"abc",	"y",	"&",	""},
    {"a.c",	"abc",	"y",	"&",	"abc"},
    {"a.c",	"axc",	"y",	"&",	"axc"},
    {"a.*c",	"axyzc","y",	"&",	"axyzc"},
    {"a.*c",	"axyzd","n",	"-",	"-"},
    {"a[bc]d",	"abc",	"n",	"-",	"-"},
    {"a[bc]d",	"abd",	"y",	"&",	"abd"},
    {"a[b-d]e",	"abd",	"n",	"-",	"-"},
    {"a[b-d]e",	"ace",	"y",	"&",	"ace"},
    {"a[b-d]",	"aac",	"y",	"&",	"ac"},
    {"a[-b]",	"a-",	"y",	"&",	"a-"},
    {"a[b-]",	"a-",	"y",	"&",	"a-"},
    {"a[b-a]",	"-",	"c",	"-",	"-"},
    {"a[]b",	"-",	"c",	"-",	"-"},
    {"a[",	"-",	"c",	"-",	"-"},
    {"a]",	"a]",	"y",	"&",	"a]"},
    {"a[]]b",	"a]b",	"y",	"&",	"a]b"},
    {"a[^bc]d",	"aed",	"y",	"&",	"aed"},
    {"a[^bc]d",	"abd",	"n",	"-",	"-"},
    {"a[^-b]c",	"adc",	"y",	"&",	"adc"},
    {"a[^-b]c",	"a-c",	"n",	"-",	"-"},
    {"a[^]b]c",	"a]c",	"n",	"-",	"-"},
    {"a[^]b]c",	"adc",	"y",	"&",	"adc"},
    {"ab|cd",	"abc",	"y",	"&",	"ab"},
    {"ab|cd",	"abcd",	"y",	"&",	"ab"},
    {"()ef",	"def",	"y",	"&-\\1",	"ef-"},
    //  "()*",	"-",	"c",	"-",	"-"},
    {"()*",	"-",	"y",	"-",	"-"},
    {"*a",	"-",	"c",	"-",	"-"},
    //  "^*",	"-",	"c",	"-",	"-"},
    {"^*",	"-",	"y",	"-",	"-"},
    //  "$*",	"-",	"c",	"-",	"-"},
    {"$*",	"-",	"y",	"-",	"-"},
    {"(*)b",	"-",	"c",	"-",	"-"},
    {"$b",	"b",	"n",	"-",	"-"},
    {"a\\",	"-",	"c",	"-",	"-"},
    {"a\\(b",	"a(b",	"y",	"&-\\1",	"a(b-"},
    {"a\\(*b",	"ab",	"y",	"&",	"ab"},
    {"a\\(*b",	"a((b",	"y",	"&",	"a((b"},
    {"a\\\\b",	"a\\b",	"y",	"&",	"a\\b"},
    {"abc)",	"-",	"c",	"-",	"-"},
    {"(abc",	"-",	"c",	"-",	"-"},
    {"((a))",	"abc",	"y",	"&-\\1-\\2",	"a-a-a"},
    {"(a)b(c)",	"abc",	"y",	"&-\\1-\\2",	"abc-a-c"},
    {"a+b+c",	"aabbabc","y",	"&",	"abc"},
    {"a**",	"-",	"c",	"-",	"-"},
    //  "a*?",	"-",	"c",	"-",	"-"},
    {"a*?a",	"aa",	"y",	"&",	"a"},
    //  "(a*)*",	"-",	"c",	"-",	"-"},
    {"(a*)*",	"aaa",	"y",	"-",	"-"},
    //  "(a*)+",	"-",	"c",	"-",	"-"},
    {"(a*)+",	"aaa",	"y",	"-",	"-"},
    //  "(a|)*",	"-",	"c",	"-",	"-"},
    {"(a|)*",	"-",	"y",	"-",	"-"},
    //  "(a*|b)*",	"-",	"c",	"-",	"-"},
    {"(a*|b)*",	"aabb",	"y",	"-",	"-"},
    {"(a|b)*",	"ab",	"y",	"&-\\1",	"ab-b"},
    {"(a+|b)*",	"ab",	"y",	"&-\\1",	"ab-b"},
    {"(a+|b)+",	"ab",	"y",	"&-\\1",	"ab-b"},
    {"(a+|b)?",	"ab",	"y",	"&-\\1",	"a-a"},
    {"[^ab]*",	"cde",	"y",	"&",	"cde"},
    //  "(^)*",	"-",	"c",	"-",	"-"},
    {"(^)*",	"-",	"y",	"-",	"-"},
    //  "(ab|)*",	"-",	"c",	"-",	"-"},
    {"(ab|)*",	"-",	"y",	"-",	"-"},
    {")(",	"-",	"c",	"-",	"-"},
    {"",	"abc",	"y",	"&",	""},
    {"abc",	"",	"n",	"-",	"-"},
    {"a*",	"",	"y",	"&",	""},
    {"([abc])*d", "abbbcd",	"y",	"&-\\1",	"abbbcd-c"},
    {"([abc])*bcd", "abcd",	"y",	"&-\\1",	"abcd-a"},
    {"a|b|c|d|e", "e",	"y",	"&",	"e"},
    {"(a|b|c|d|e)f", "ef",	"y",	"&-\\1",	"ef-e"},
    //  "((a*|b))*", "-",	"c",	"-",	"-"},
    {"((a*|b))*", "aabb", "y",	"-",	"-"},
    {"abcd*efg",	"abcdefg",	"y",	"&",	"abcdefg"},
    {"ab*",	"xabyabbbz",	"y",	"&",	"ab"},
    {"ab*",	"xayabbbz",	"y",	"&",	"a"},
    {"(ab|cd)e",	"abcde",	"y",	"&-\\1",	"cde-cd"},
    {"[abhgefdc]ij",	"hij",	"y",	"&",	"hij"},
    {"^(ab|cd)e",	"abcde",	"n",	"x\\1y",	"xy"},
    {"(abc|)ef",	"abcdef",	"y",	"&-\\1",	"ef-"},
    {"(a|b)c*d",	"abcd",	"y",	"&-\\1",	"bcd-b"},
    {"(ab|ab*)bc",	"abc",	"y",	"&-\\1",	"abc-a"},
    {"a([bc]*)c*",	"abc",	"y",	"&-\\1",	"abc-bc"},
    {"a([bc]*)(c*d)",	"abcd",	"y",	"&-\\1-\\2",	"abcd-bc-d"},
    {"a([bc]+)(c*d)",	"abcd",	"y",	"&-\\1-\\2",	"abcd-bc-d"},
    {"a([bc]*)(c+d)",	"abcd",	"y",	"&-\\1-\\2",	"abcd-b-cd"},
    {"a[bcd]*dcdcde",	"adcdcde",	"y",	"&",	"adcdcde"},
    {"a[bcd]+dcdcde",	"adcdcde",	"n",	"-",	"-"},
    {"(ab|a)b*c",	"abc",	"y",	"&-\\1",	"abc-ab"},
    {"((a)(b)c)(d)",	"abcd",	"y",	"\\1-\\2-\\3-\\4",	"abc-a-b-d"},
    {"[a-zA-Z_][a-zA-Z0-9_]*",	"alpha",	"y",	"&",	"alpha"},
    {"^a(bc+|b[eh])g|.h$",	"abh",	"y",	"&-\\1",	"bh-"},
    {"(bc+d$|ef*g.|h?i(j|k))",	"effgz",	"y",	"&-\\1-\\2",	"effgz-effgz-"},
    {"(bc+d$|ef*g.|h?i(j|k))",	"ij",	"y",	"&-\\1-\\2",	"ij-ij-j"},
    {"(bc+d$|ef*g.|h?i(j|k))",	"effg",	"n",	"-",	"-"},
    {"(bc+d$|ef*g.|h?i(j|k))",	"bcdd",	"n",	"-",	"-"},
    {"(bc+d$|ef*g.|h?i(j|k))",	"reffgz",	"y",	"&-\\1-\\2",	"effgz-effgz-"},
    //    "((((((((((a))))))))))",	"-",	"c",	"-",	"-"},
    {"(((((((((a)))))))))",	"a",	"y",	"&",	"a"},
    {"multiple words of text",	"uh-uh",	"n",	"-",	"-"},
    {"multiple words",	"multiple words, yeah",	"y",	"&",	"multiple words"},
    {"(.*)c(.*)",	"abcde",	"y",	"&-\\1-\\2",	"abcde-ab-de"},
    {"\\((.*), (.*)\\)",	"(a, b)",	"y",	"(\\2, \\1)",	"(b, a)"},
    {"abcd",	"abcd",	"y",	"&-\\&-\\\\&",	"abcd-&-\\abcd"},
    {"a(bc)d",	"abcd",	"y",	"\\1-\\\\1-\\\\\\1",	"bc-\\1-\\bc"},
    {"[k]",			"ab",	"n",	"-",	"-"},
    {"[ -~]*",			"abc",	"y",	"&",	"abc"},
    {"[ -~ -~]*",		"abc",	"y",	"&",	"abc"},
    {"[ -~ -~ -~]*",		"abc",	"y",	"&",	"abc"},
    {"[ -~ -~ -~ -~]*",		"abc",	"y",	"&",	"abc"},
    {"[ -~ -~ -~ -~ -~]*",	"abc",	"y",	"&",	"abc"},
    {"[ -~ -~ -~ -~ -~ -~]*",	"abc",	"y",	"&",	"abc"},
    {"[ -~ -~ -~ -~ -~ -~ -~]*",	"abc",	"y",	"&",	"abc"},
    {"a{2}",	"candy",		"n",	"",	""},
    {"a{2}",	"caandy",		"y",	"&",	"aa"},
    {"a{2}",	"caaandy",		"y",	"&",	"aa"},
    {"a{2,}",	"candy",		"n",	"",	""},
    {"a{2,}",	"caandy",		"y",	"&",	"aa"},
    {"a{2,}",	"caaaaaandy",		"y",	"&",	"aaaaaa"},
    {"a{1,3}",	"cndy",			"n",	"",	""},
    {"a{1,3}",	"candy",		"y",	"&",	"a"},
    {"a{1,3}",	"caandy",		"y",	"&",	"aa"},
    {"a{1,3}",	"caaaaaandy",		"y",	"&",	"aaa"},
    {"e?le?",	"angel",		"y",	"&",	"el"},
    {"e?le?",	"angle",		"y",	"&",	"le"},
    {"\\bn\\w",	"noonday",		"y",	"&",	"no"},
    {"\\wy\\b",	"possibly yesterday",	"y",	"&",	"ly"},
    {"\\w\\Bn",	"noonday",		"y",	"&",	"on"},
    {"y\\B\\w",	"possibly yesterday",	"y",	"&",	"ye"},
    {"\\cJ",	"abc\ndef",		"y",	"&",	"\n"},
    {"\\d",	"B2 is",		"y",	"&",	"2"},
    {"\\D",	"B2 is",		"y",	"&",	"B"},
    {"\\s\\w*",	"foo bar",		"y",	"&",	" bar"},
    {"\\S\\w*",	"foo bar",		"y",	"&",	"foo"},
    {"abc",	"ababc",		"y",	"&",	"abc"},
    {"apple(,)\\sorange\\1",	"apple, orange, cherry, peach",	"y", "&", "apple, orange,"},
    {"(\\w+)\\s(\\w+)",		"John Smith", "y", "\\2, \\1", "Smith, John"},
    {"\\n\\f\\r\\t\\v",		"abc\n\f\r\t\vdef", "y", "&", "\n\f\r\t\v"},
    {".*c",	"abcde",		"y",	"&",	"abc"},
    {"^\\w+((;|=)\\w+)+$", "some=host=tld", "y", "&-\\1-\\2", "some=host=tld-=tld-="},
    {"^\\w+((\\.|-)\\w+)+$", "some.host.tld", "y", "&-\\1-\\2", "some.host.tld-.tld-."},
    {"q(a|b)*q",	"xxqababqyy",		"y",	"&-\\1",	"qababq-b"},
};

int main(int argc, char *argv[])
{   RegExp *r;
    int i;
    int a;
    unsigned c;
    int start;
    int end;
    TestVectorsD tvd;

    if (argc > 1)
    {
	start = atoi(argv[1]);
	end = start + 1;
    }
    else
    {
	start = 0;
	end = sizeof(tv) / sizeof(tv[0]);
    }

    mem.init();
    r = new RegExp(DTEXT(""), DTEXT(""), 1);

    for (a = start; a < end; a++)
    {
	printf("tv[%d]: pattern='%s' input='%s' result=%s format='%s' replace='%s'\n",
	    a, tv[a].pattern, tv[a].input, tv[a].result, tv[a].format, tv[a].replace);

#if defined UNICODE
	tvd.pattern = Dchar::dup(tv[a].pattern);
	tvd.input   = Dchar::dup(tv[a].input);
	tvd.result  = Dchar::dup(tv[a].result);
	tvd.format  = Dchar::dup(tv[a].format);
	tvd.replace = Dchar::dup(tv[a].replace);
#else
	tvd = tv[a];
#endif
	c = tvd.result[0];

	i = r->compile(tvd.pattern, DTEXT(""), 1);
	printf("\tcompile() = %d\n", i);
	assert((c == 'c') ? !i : i);

	if (c != 'c')
	{
	    i = r->test(tvd.input);
	    printf("\ttest() = %d\n", i);
	    fflush(stdout);
	    assert((c == 'y') ? i : !i);

	    if (c == 'y')
	    {	dchar *s;

		s = r->replace(tvd.format);
		if (Dchar::cmp(s, tvd.replace))
		{
#if defined UNICODE
		    wprintf(L"replaced string = '%ls', should be '%ls'\n", s, tvd.replace);
#else
		    printf("replaced string = '%s'\n", s);
#endif
		    assert(0);
		}
	    }
	}
    }

    //delete r;
    mem.fullcollect();
    printf("Success\n");
    return EXIT_SUCCESS;
}


