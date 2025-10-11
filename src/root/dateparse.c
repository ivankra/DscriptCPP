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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if _MSC_VER
#include <malloc.h>
#endif

#include "dchar.h"

#include "dateparse.h"
#include "root.h"

enum DP
{
    DPerr,
    DPweekday,
    DPmonth,
    DPnumber,
    DPend,
    DPcolon,
    DPminus,
    DPslash,
    DPampm,
    DPplus,
    DPtz,
    DPdst,
    DPdsttz,
};

int DateParse::nextToken()
{   int nest;
    unsigned c;
    char *b;
    int result = DPerr;

    //message(DTEXT("DateParse::nextToken()\n"));
    for (;;)
    {
	//message(DTEXT("\t*p = '%c'\n"), *p);
	switch (Dchar::get(p))
	{
	    case 0:	result = DPend;   goto ret_inc;
	    case ':':	result = DPcolon; goto ret_inc;
	    case '+':	result = DPplus;  goto ret_inc;
	    case '-':	result = DPminus; goto ret_inc;
	    case '/':	result = DPslash; goto ret_inc;
#if defined(DATE_DOT_DELIM)
	    case '.':	result = DPslash; goto ret_inc;
#endif
	    ret_inc:
		p = Dchar::inc(p);
		goto Lret;

	    case ' ':
	    case '\n':
	    case '\r':
	    case '\t':
	    case ',':
#if !defined(DATE_DOT_DELIM)
	    case '.':
#endif
		p = Dchar::inc(p);
		break;

	    case '(':		// comment
		nest = 1;
		for (;;)
		{
		    p = Dchar::inc(p);
		    switch (Dchar::get(p))
		    {
			case 0:
			    goto Lret;		// error

			case '(':
			    nest++;
			    break;

			case ')':
			    if (--nest == 0)
				goto Lendofcomment;
			    break;
		    }
		}
	    Lendofcomment:
		p = Dchar::inc(p);
		break;

	    default:
		number = 0;
		for (;;)
		{
		    c = Dchar::get(p);
		    if (!(c >= '0' && c <= '9'))
			break;
		    result = DPnumber;
		    number = number * 10 + (c - '0');
		    p = Dchar::inc(p);
		}
		if (result == DPnumber)
		    goto Lret;

		b = buffer;
		while (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z')
		{
		    if (c < 'a')		// if upper case
			c += 'a' - 'A';		// to lower case
		    *b++ = (char) c;
		    do
		    {
			p = Dchar::inc(p);
			c = Dchar::get(p);
		    } while (c == '.');		// ignore embedded '.'s
		}
		*b = 0;
		result = classify();
		goto Lret;
	}
    }
Lret:
    return result;
}

int DateParse::classify()
{
    struct DateID
    {
	char *name;
	short tok;
	short value;
    };

    static DateID dateidtab[] =
    {
	{"january",	DPmonth,	1},
	{"february",	DPmonth,	2},
	{"march",	DPmonth,	3},
	{"april",	DPmonth,	4},
	{"may",		DPmonth,	5},
	{"june",	DPmonth,	6},
	{"july",	DPmonth,	7},
	{"august",	DPmonth,	8},
	{"september",	DPmonth,	9},
	{"october",	DPmonth,	10},
	{"november",	DPmonth,	11},
	{"december",	DPmonth,	12},
	{"jan",		DPmonth,	1},
	{"feb",		DPmonth,	2},
	{"mar",		DPmonth,	3},
	{"apr",		DPmonth,	4},
	{"jun",		DPmonth,	6},
	{"jul",		DPmonth,	7},
	{"aug",		DPmonth,	8},
	{"sep",		DPmonth,	9},
	{"sept",	DPmonth,	9},
	{"oct",		DPmonth,	10},
	{"nov",		DPmonth,	11},
	{"dec",		DPmonth,	12},

	{"sunday",	DPweekday,	1},
	{"monday",	DPweekday,	2},
	{"tuesday",	DPweekday,	3},
	{"tues",	DPweekday,	3},
	{"wednesday",	DPweekday,	4},
	{"wednes",	DPweekday,	4},
	{"thursday",	DPweekday,	5},
	{"thur",	DPweekday,	5},
	{"thurs",	DPweekday,	5},
	{"friday",	DPweekday,	6},
	{"saturday",	DPweekday,	7},

	{"sun",		DPweekday,	1},
	{"mon",		DPweekday,	2},
	{"tue",		DPweekday,	3},
	{"wed",		DPweekday,	4},
	{"thu",		DPweekday,	5},
	{"fri",		DPweekday,	6},
	{"sat",		DPweekday,	7},

	{"am",		DPampm,		1},
	{"pm",		DPampm,		2},

	{"gmt",		DPtz,		+000},
	{"ut",		DPtz,		+000},
	{"utc",		DPtz,		+000},
	{"wet",		DPtz,		+000},
	{"z",		DPtz,		+000},
	{"wat",		DPtz,		+100},
	{"a",		DPtz,		+100},
	{"at",		DPtz,		+200},
	{"b",		DPtz,		+200},
	{"c",		DPtz,		+300},
	{"ast",		DPtz,		+400},
	{"d",		DPtz,		+400},
	{"est",		DPtz,		+500},
	{"e",		DPtz,		+500},
	{"cst",		DPtz,		+600},
	{"f",		DPtz,		+600},
	{"mst",		DPtz,		+700},
	{"g",		DPtz,		+700},
	{"pst",		DPtz,		+800},
	{"h",		DPtz,		+800},
	{"yst",		DPtz,		+900},
	{"i",		DPtz,		+900},
	{"ahst",	DPtz,		+1000},
	{"cat",		DPtz,		+1000},
	{"hst",		DPtz,		+1000},
	{"k",		DPtz,		+1000},
	{"nt",		DPtz,		+1100},
	{"l",		DPtz,		+1100},
	{"idlw",	DPtz,		+1200},
	{"m",		DPtz,		+1200},

	{"cet",		DPtz,		-100},
	{"fwt",		DPtz,		-100},
	{"met",		DPtz,		-100},
	{"mewt",	DPtz,		-100},
	{"swt",		DPtz,		-100},
	{"n",		DPtz,		-100},
	{"eet",		DPtz,		-200},
	{"o",		DPtz,		-200},
	{"bt",		DPtz,		-300},
	{"p",		DPtz,		-300},
	{"zp4",		DPtz,		-400},
	{"q",		DPtz,		-400},
	{"zp5",		DPtz,		-500},
	{"r",		DPtz,		-500},
	{"zp6",		DPtz,		-600},
	{"s",		DPtz,		-600},
	{"wast",	DPtz,		-700},
	{"t",		DPtz,		-700},
	{"cct",		DPtz,		-800},
	{"u",		DPtz,		-800},
	{"jst",		DPtz,		-900},
	{"v",		DPtz,		-900},
	{"east",	DPtz,		-1000},
	{"gst",		DPtz,		-1000},
	{"w",		DPtz,		-1000},
	{"x",		DPtz,		-1100},
	{"idle",	DPtz,		-1200},
	{"nzst",	DPtz,		-1200},
	{"nzt",		DPtz,		-1200},
	{"y",		DPtz,		-1200},

	{"bst",		DPdsttz,	000},
	{"adt",		DPdsttz,	+400},
	{"edt",		DPdsttz,	+500},
	{"cdt",		DPdsttz,	+600},
	{"mdt",		DPdsttz,	+700},
	{"pdt",		DPdsttz,	+800},
	{"ydt",		DPdsttz,	+900},
	{"hdt",		DPdsttz,	+1000},
	{"mest",	DPdsttz,	-100},
	{"mesz",	DPdsttz,	-100},
	{"sst",		DPdsttz,	-100},
	{"fst",		DPdsttz,	-100},
	{"wadt",	DPdsttz,	-700},
	{"eadt",	DPdsttz,	-1000},
	{"nzdt",	DPdsttz,	-1200},

	{"dst",		DPdst,		0},
    };
    size_t len;

    //message(DTEXT("DateParse::classify('%s')\n"), buffer);

    // We don't need to do dchar's, because none of the strings we
    // look for have special characters in them
    len = strlen(buffer) + 1;

    // Do a linear search. Yes, it would be faster with a binary
    // one.
    for (unsigned i = 0; i < sizeof(dateidtab) / sizeof(dateidtab[0]); i++)
    {
	if (memcmp(dateidtab[i].name, buffer, len) == 0)
	{
	    number = dateidtab[i].value;
	    return dateidtab[i].tok;
	}
    }
    return DPerr;
}

int DateParse::parseString(dchar *s)
{
    int n1;
    int dp;
    dchar *psave;
    int result;

    //message(DTEXT("DateParse::parseString('%ls')\n"), s);
    this->s = s;
    p = s;
    dp = nextToken();
    for (;;)
    {
	//message(DTEXT("\tdp = %d\n"), dp);
	switch (dp)
	{
	    case DPend:
		result = 1;
	    Lret:
		return result;

	    case DPerr:
	    case_error:
		//message(DTEXT("\terror\n"));
	    default:
		result = 0;
		goto Lret;

	    case DPminus:
		break;			// ignore spurious '-'

	    case DPweekday:
		weekday = number;
		break;

	    case DPmonth:		// month day, [year]
		month = number;
		dp = nextToken();
		if (dp == DPnumber)
		{
		    day = number;
		    psave = p;
		    dp = nextToken();
		    if (dp == DPnumber)
		    {
			n1 = number;
			dp = nextToken();
			if (dp == DPcolon)
			{   // back up, not a year
			    p = psave;
			}
			else
			{   year = n1;
			    continue;
			}
			break;
		    }
		}
		continue;

	    case DPnumber:
		n1 = number;
		dp = nextToken();
		switch (dp)
		{
		    case DPend:
			year = n1;
			break;

		    case DPminus:
		    case DPslash:	// n1/ ? ? ?
			dp = parseCalendarDate(n1);
			if (dp == DPerr)
			    goto case_error;
			break;

		   case DPcolon:	// hh:mm [:ss] [am | pm]
			dp = parseTimeOfDay(n1);
			if (dp == DPerr)
			    goto case_error;
			break;

		   case DPampm:
			hours = n1;
			minutes = 0;
			seconds = 0;
			ampm = number;
			break;

		    case DPmonth:
			day = n1;
			month = number;
			dp = nextToken();
			if (dp == DPnumber)
			{   // day month year
			    year = number;
			    dp = nextToken();
			}
			break;

		    default:
			year = n1;
			break;
		}
		continue;
	}
	dp = nextToken();
    }
}

int DateParse::parseCalendarDate(int n1)
{
    int n2;
    int n3;
    int dp;

    //message(DTEXT("DateParse::parseCalendarDate(%d)\n"), n1);
    dp = nextToken();
    if (dp == DPmonth)	// day/month
    {
	day = n1;
	month = number;
	dp = nextToken();
	if (dp == DPnumber)
	{   // day/month year
	    year = number;
	    dp = nextToken();
	}
	else if (dp == DPminus || dp == DPslash)
	{   // day/month/year
	    dp = nextToken();
	    if (dp != DPnumber)
		goto case_error;
	    year = number;
	    dp = nextToken();
	}
	return dp;
    }
    if (dp != DPnumber)
	goto case_error;
    n2 = number;
    //message(DTEXT("\tn2 = %d\n"), n2);
    dp = nextToken();
    if (dp == DPminus || dp == DPslash)
    {
	dp = nextToken();
	if (dp != DPnumber)
	    goto case_error;
	n3 = number;
	//message(DTEXT("\tn3 = %d\n"), n3);
	dp = nextToken();

	// case1: year/month/day
	// case2: month/day/year
	int case1, case2;

	case1 = (n1 > 12 ||
		 (n2 >= 1 && n2 <= 12) &&
		 (n3 >= 1 && n3 <= 31));
	case2 = ((n1 >= 1 && n1 <= 12) &&
		 (n2 >= 1 && n2 <= 31) ||
		 n3 > 31);
	if (case1 == case2)
	    goto case_error;
	if (case1)
	{
	    year = n1;
	    month = n2;
	    day = n3;
	}
	else
	{
	    month = n1;
	    day = n2;
	    year = n3;
	}
    }
    else
    {   // must be month/day
	month = n1;
	day = n2;
    }
    return dp;

case_error:
    return DPerr;
}

int DateParse::parseTimeOfDay(int n1)
{
    int dp;
    int sign;

    // 12am is midnight
    // 12pm is noon

    //message(DTEXT("DateParse::parseTimeOfDay(%d)\n"), n1);
    hours = n1;
    dp = nextToken();
    if (dp != DPnumber)
	goto case_error;
    minutes = number;
    dp = nextToken();
    if (dp == DPcolon)
    {
	dp = nextToken();
	if (dp != DPnumber)
	    goto case_error;
	seconds = number;
	dp = nextToken();
    }
    else
	seconds = 0;

    if (dp == DPampm)
    {
	ampm = number;
	dp = nextToken();
    }
    else if (dp == DPplus || dp == DPminus)
    {
    Loffset:
	sign = (dp == DPminus) ? -1 : 1;
	dp = nextToken();
	if (dp != DPnumber)
	    goto case_error;
	tzcorrection = -sign * number;
	dp = nextToken();
    }
    else if (dp == DPtz)
    {
	tzcorrection = number;
	dp = nextToken();
	if (number == 0 && (dp == DPplus || dp == DPminus))
	    goto Loffset;
	if (dp == DPdst)
	{   tzcorrection += 100;
	    dp = nextToken();
	}
    }
    else if (dp == DPdsttz)
    {
	tzcorrection = number;
	dp = nextToken();
    }

    return dp;

case_error:
    return DPerr;
}

int DateParse::parse(dchar *s)
{
    buffer = (char *) alloca((Dchar::len(s) + 1) * sizeof(char));

    year = ~0;
    month = 0;
    day = 0;
    hours = 0;
    minutes = 0;
    seconds = 0;
    ms = 0;
    weekday = 0;
    ampm = 0;
    tzcorrection = TZCORRECTION_NAN;

    //message(DTEXT("DateParse::parse('%s')\n"), s);
    if (!parseString(s))
	goto Lerror;

#if defined(DATE_OR_TIME)
    if (year == ~0)
        year = 0;
    else
#endif
    if (
	year == ~0 ||
	(month < 1 || month > 12) ||
	(day < 1 || day > 31) ||
	(hours < 0 || hours > 23) ||
	(minutes < 0 || minutes > 59) ||
	(seconds < 0 || seconds > 59) ||
	(tzcorrection != TZCORRECTION_NAN &&
	 ((tzcorrection < -1200 || tzcorrection > 1200) ||
	  (tzcorrection % 100)))
	)
    {
     Lerror:
	return 0;
    }

    if (ampm)
    {	if (hours > 12)
	    goto Lerror;
	if (hours < 12)
	{
	    if (ampm == 2)	// if P.M.
		hours += 12;
	}
	else if (ampm == 1)	// if 12am
	{
	    //hours = 24;	// which is midnight
	    hours = 0;		// which is midnight
	}
    }

    if (tzcorrection != TZCORRECTION_NAN)
	tzcorrection /= 100;

    if (year >= 0 && year <= 99)
	year += 1900;

    return 1;
}


#if 0

void main()
{
    DateParse dp;

    //dp.parse("March 10, 1959 12:00 -800");
    //dp.parse("Tue Apr 02 02:04:57 GMT-0800 1996");
    //dp.parse(L"March 14, -1980 21:14:50");
    //dp.parse("Tue Apr 02 02:04:57 1996");
    //dp.parse("Tue, 02 Apr 1996 02:04:57 G.M.T.");
    //dp.parse("December 31, 3000");
    //dp.parse(L"Wed, 31 Dec 1969 16:00:00 GMT");
    dp.parse(L"1/1/1999 12:30 AM");

    message(DTEXT("year = %d, month = %d, day = %d\n"), dp.year, dp.month, dp.day);
    message(DTEXT("%02d:%02d:%02d.%03d\n"), dp.hours, dp.minutes, dp.seconds, dp.ms);
    message(DTEXT("weekday = %d, ampm = %d, tzcorrection = %d\n"), dp.weekday, dp.ampm, dp.tzcorrection);
}

#endif
