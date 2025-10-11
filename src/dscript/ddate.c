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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "port.h"
#include "root.h"
#include "dscript.h"
#include "value.h"
#include "date.h"
#include "dobject.h"
#include "text.h"

static long msPerMinute = 1000 * 60;	// millseconds per minute

#define DATETOSTRING	1		// use DateToString

/* ===================== Ddate.constructor functions ==================== */

BUILTIN_FUNCTION2(Ddate_, parse, 1)
{

    // ECMA 15.9.4.2
    d_string s;
    d_number n;

    if (argc == 0)
	n = Port::nan;
    else
    {
	s = arglist[0].toString();
	n = parseDateString(cc, s);
    }

    Vnumber::putValue(ret, n);
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_, UTC, 7)
{
    // ECMA 15.9.4.3 - 15.9.4.10

    d_number n;

    d_number year;
    d_number month = 0;
    d_number date = 0;
    d_number hours = 0;
    d_number minutes = 0;
    d_number seconds = 0;
    d_number ms = 0;

    d_time day;
    d_time time = 0;

    switch (argc)
    {
	default:
	case 7:
	    ms = arglist[6].toNumber();
	case 6:
	    seconds = arglist[5].toNumber();
	case 5:
	    minutes = arglist[4].toNumber();
	case 4:
	    hours = arglist[3].toNumber();
	    time = Date::MakeTime(hours, minutes, seconds, ms);
	case 3:
	    date = arglist[2].toNumber();
	case 2:
	    month = arglist[1].toNumber();
	case 1:
	    year = arglist[0].toNumber();

	    if (!Port::isnan(year) && year >= 0 && year <= 99)
		year += 1900;
	    day = Date::MakeDay(year, month, date);
	    n = Date::TimeClip(Date::MakeDate(day,time));
	    break;

	case 0:
	    n = Date::time();
	    break;
    }
    Vnumber::putValue(ret, n);
    return NULL;
}

/* ===================== Ddate_constructor ==================== */

struct Ddate_constructor : Dfunction
{
    Ddate_constructor(ThreadContext *tc);

    void *Construct(CONSTRUCT_ARGS);
    void *Call(CALL_ARGS);
};

Ddate_constructor::Ddate_constructor(ThreadContext *tc)
    : Dfunction(7, tc->Dfunction_prototype)
{
    name = "Date";

    static NativeFunctionData nfd[] =
    {
    #define X(name,len) { &TEXT_##name, #name, Ddate_##name, len }
	X(parse, 1),
	X(UTC, 7),
    #undef X
    };

    DnativeFunction::init(this, nfd, sizeof(nfd) / sizeof(nfd[0]), 0);
}

void *Ddate_constructor::Call(CALL_ARGS)
{
    // ECMA 15.9.2
    // return string as if (new Date()).toString()
    d_string s;
    d_time t;

    GC_LOG();
#if DATETOSTRING
    t = Date::time();
    t = Date::LocalTime(t);
    GC_LOG();
    s = dateToString(cc, t, TFString);
#else
    t = Date::time();
    GC_LOG();
    s = d_string_ctor(Date::ToString(t));
#endif
    Vstring::putValue(ret, s);
    return NULL;
}

void *Ddate_constructor::Construct(CONSTRUCT_ARGS)
{
    // ECMA 15.9.3
    Dobject *o;
    d_number n;

    d_number year;
    d_number month;
    d_number date = 0;
    d_number hours = 0;
    d_number minutes = 0;
    d_number seconds = 0;
    d_number ms = 0;

    d_time day;
    d_time time = 0;

    switch (argc)
    {
	default:
	case 7:
	    ms = arglist[6].toNumber();
	case 6:
	    seconds = arglist[5].toNumber();
	case 5:
	    minutes = arglist[4].toNumber();
	case 4:
	    hours = arglist[3].toNumber();
	    time = Date::MakeTime(hours, minutes, seconds, ms);
	case 3:
	    date = arglist[2].toNumber();
	case 2:
	    month = arglist[1].toNumber();
	    year = arglist[0].toNumber();

	    if (!Port::isnan(year) && year >= 0 && year <= 99)
		year += 1900;
	    day = Date::MakeDay(year, month, date);
	    n = Date::TimeClip(Date::UTC(Date::MakeDate(day,time)));
	    break;

	case 1:
	    arglist[0].toPrimitive(ret, NULL);
	    if (ret->getType() == TypeString)
	    {
		n = parseDateString(cc, ret->x.string);
	    }
	    else
	    {
		n = ret->toNumber();
		n = Date::TimeClip(n);
	    }
	    break;

	case 0:
	    n = Date::time();
	    break;
    }
    GC_LOG();
    o = new(this) Ddate(n);
    Vobject::putValue(ret, o);
    return NULL;
}

/* ===================== Ddate.prototype functions =============== */

void *checkdate(Value *ret, dchar *name, Dobject *othis)
{
    Value::copy(ret, &vundefined);
    ErrInfo errinfo;
    errinfo.code = 5006;
    return Dobject::RuntimeError(&errinfo, ERR_FUNCTION_WANTS_DATE,
	    DTEXT("Date.prototype"), name, d_string_ptr(othis->classname));
}

#define CHECKDATE(name)					\
    if (!othis->isClass(TEXT_Date))			\
    {							\
	return checkdate(ret, name, othis);		\
    }

int getThisTime(Value *ret, Dobject *othis, d_time *n)
{
    *n = othis->value.number;
    Vnumber::putValue(ret, *n);
    return Port::isnan(*n) ? 1 : 0;
}

int getThisLocalTime(Value *ret, Dobject *othis, d_time *n)
{   int isn = 1;

    *n = othis->value.number;
    if (!Port::isnan(*n))
    {	isn = 0;
	*n = Date::LocalTime(*n);
    }
    Vnumber::putValue(ret, *n);
    return isn;
}

BUILTIN_FUNCTION2(Ddate_prototype_, toString, 0)
{
    // ECMA 15.9.5.2
    d_number n;
    d_string s;

    CHECKDATE(d_string_ptr(TEXT_toString));
#if DATETOSTRING
    getThisLocalTime(ret, othis, &n);
    GC_LOG();
    s = dateToString(cc, n, TFString);
#else
    getThisTime(ret, othis, &n);
    GC_LOG();
    s = d_string_ctor(Date::ToString(n));
#endif
    Vstring::putValue(ret, s);
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, toDateString, 0)
{
    // ECMA 15.9.5.3
    d_number n;
    d_string s;

    CHECKDATE(d_string_ptr(TEXT_toDateString));
#if DATETOSTRING
    getThisLocalTime(ret, othis, &n);
    GC_LOG();
    s = dateToString(cc, n, TFDateString);
#else
    getThisTime(ret, othis, &n);
    GC_LOG();
    s = d_string_ctor(Date::ToDateString(n));
#endif
    Vstring::putValue(ret, s);
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, toTimeString, 0)
{
    // ECMA 15.9.5.4
    d_number n;
    d_string s;

    CHECKDATE(d_string_ptr(TEXT_toTimeString));
#if DATETOSTRING
    getThisLocalTime(ret, othis, &n);
    GC_LOG();
    s = dateToString(cc, n, TFTimeString);
#else
    getThisTime(ret, othis, &n);
    GC_LOG();
    s = d_string_ctor(Date::ToTimeString(n));
#endif
    //s = d_string_ctor(Date::ToTimeString(n));
    Vstring::putValue(ret, s);
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, valueOf, 0)
{
    // ECMA 15.9.5.3
    d_number n;

    CHECKDATE(d_string_ptr(TEXT_valueOf));
    getThisTime(ret, othis, &n);
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, getTime, 0)
{
    // ECMA 15.9.5.4
    d_number n;

    CHECKDATE(DTEXT("getTime"));
    getThisTime(ret, othis, &n);
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, getVarDate, 0)
{
    // Jscript compatibility
    d_number n;
    double d;

    CHECKDATE(DTEXT("getVarDate"));
    n = othis->value.number;

    // Convert n to VT_DATE format
    d = Date::toVarDate(n);

    Vdate::putValue(ret, d);
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, getYear, 0)
{
    // ECMA 15.9.5.5
    d_number n;

    CHECKDATE(DTEXT("getYear"));
    if (getThisLocalTime(ret, othis, &n) == 0)
    {
	n = Date::YearFromTime(n) - 1900;
#if 1 // emulate jscript bug
	if (n < 0 || n >= 100)
	    n += 1900;
#endif
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, getFullYear, 0)
{
    // ECMA 15.9.5.6
    d_number n;

    CHECKDATE(DTEXT("getFullYear"));
    if (getThisLocalTime(ret, othis, &n) == 0)
    {
	n = Date::YearFromTime(n);
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, getUTCFullYear, 0)
{
    // ECMA 15.9.5.7
    d_number n;

    CHECKDATE(DTEXT("getUTCFullYear"));
    if (getThisTime(ret, othis, &n) == 0)
    {
	n = Date::YearFromTime(n);
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, getMonth, 0)
{
    // ECMA 15.9.5.8
    d_number n;

    CHECKDATE(DTEXT("getMonth"));
    if (getThisLocalTime(ret, othis, &n) == 0)
    {
	n = Date::MonthFromTime(n);
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, getUTCMonth, 0)
{
    // ECMA 15.9.5.9
    d_number n;

    CHECKDATE(DTEXT("getUTCMonth"));
    if (getThisTime(ret, othis, &n) == 0)
    {
	n = Date::MonthFromTime(n);
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, getDate, 0)
{
    // ECMA 15.9.5.10
    d_number n;

    CHECKDATE(DTEXT("getDate"));
    if (getThisLocalTime(ret, othis, &n) == 0)
    {
	//printf("LocalTime = %.16g\n", n);
	//printf("DaylightSavingTA(n) = %d\n", Date::DaylightSavingTA(n));
	n = Date::DateFromTime(n);
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, getUTCDate, 0)
{
    // ECMA 15.9.5.11
    d_number n;

    CHECKDATE(DTEXT("getUTCDate"));
    if (getThisTime(ret, othis, &n) == 0)
    {
	n = Date::DateFromTime(n);
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, getDay, 0)
{
    // ECMA 15.9.5.12
    d_number n;

    CHECKDATE(DTEXT("getDay"));
    if (getThisLocalTime(ret, othis, &n) == 0)
    {
	n = Date::WeekDay(n);
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, getUTCDay, 0)
{
    // ECMA 15.9.5.13
    d_number n;

    CHECKDATE(DTEXT("getUTCDay"));
    if (getThisTime(ret, othis, &n) == 0)
    {
	n = Date::WeekDay(n);
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, getHours, 0)
{
    // ECMA 15.9.5.14
    d_number n;

    CHECKDATE(DTEXT("getHours"));
    if (getThisLocalTime(ret, othis, &n) == 0)
    {
	n = Date::HourFromTime(n);
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, getUTCHours, 0)
{
    // ECMA 15.9.5.15
    d_number n;

    CHECKDATE(DTEXT("getUTCHours"));
    if (getThisTime(ret, othis, &n) == 0)
    {
	n = Date::HourFromTime(n);
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, getMinutes, 0)
{
    // ECMA 15.9.5.16
    d_number n;

    CHECKDATE(DTEXT("getMinutes"));
    if (getThisLocalTime(ret, othis, &n) == 0)
    {
	n = Date::MinFromTime(n);
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, getUTCMinutes, 0)
{
    // ECMA 15.9.5.17
    d_number n;

    CHECKDATE(DTEXT("getUTCMinutes"));
    if (getThisTime(ret, othis, &n) == 0)
    {
	n = Date::MinFromTime(n);
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, getSeconds, 0)
{
    // ECMA 15.9.5.18
    d_number n;

    CHECKDATE(DTEXT("getSeconds"));
    if (getThisLocalTime(ret, othis, &n) == 0)
    {
	n = Date::SecFromTime(n);
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, getUTCSeconds, 0)
{
    // ECMA 15.9.5.19
    d_number n;

    CHECKDATE(DTEXT("getUTCSeconds"));
    if (getThisTime(ret, othis, &n) == 0)
    {
	n = Date::SecFromTime(n);
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, getMilliseconds, 0)
{
    // ECMA 15.9.5.20
    d_number n;

    CHECKDATE(DTEXT("getMilliseconds"));
    if (getThisLocalTime(ret, othis, &n) == 0)
    {
	n = Date::msFromTime(n);
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, getUTCMilliseconds, 0)
{
    // ECMA 15.9.5.21
    d_number n;

    CHECKDATE(DTEXT("getUTCMilliseconds"));
    if (getThisTime(ret, othis, &n) == 0)
    {
	n = Date::msFromTime(n);
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, getTimezoneOffset, 0)
{
    // ECMA 15.9.5.22
    d_number n;

    CHECKDATE(DTEXT("getTimezoneOffset"));
    if (getThisTime(ret, othis, &n) == 0)
    {
	n = (n - Date::LocalTime(n)) / msPerMinute;
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, setTime, 1)
{
    // ECMA 15.9.5.23
    d_number n;

    CHECKDATE(DTEXT("setTime"));
    if (!argc)
	n = Port::nan;
    else
	n = arglist[0].toNumber();
    n = Date::TimeClip(n);
    othis->value.number = n;
    Vnumber::putValue(ret, n);
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, setMilliseconds, 1)
{
    // ECMA 15.9.5.24

    d_number ms;
    d_time t;
    d_time time;
    d_number n;

    CHECKDATE(DTEXT("setMilliseconds"));
    if (getThisLocalTime(ret, othis, &t) == 0)
    {
	if (!argc)
	    ms = Port::nan;
	else
	    ms = arglist[0].toNumber();
	time = Date::MakeTime(Date::HourFromTime(t), Date::MinFromTime(t), Date::SecFromTime(t), ms);
	n = Date::TimeClip(Date::UTC(Date::MakeDate(Date::Day(t),time)));
	othis->value.number = n;
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, setUTCMilliseconds, 1)
{
    // ECMA 15.9.5.25
    d_number ms;
    d_time t;
    d_time time;
    d_number n;

    CHECKDATE(DTEXT("setUTCMilliseconds"));
    if (getThisTime(ret, othis, &t) == 0)
    {
	if (!argc)
	    ms = Port::nan;
	else
	    ms = arglist[0].toNumber();
	time = Date::MakeTime(Date::HourFromTime(t), Date::MinFromTime(t), Date::SecFromTime(t), ms);
	n = Date::TimeClip(Date::MakeDate(Date::Day(t),time));
	othis->value.number = n;
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, setSeconds, 2)
{
    // ECMA 15.9.5.26
    d_number ms;
    d_number seconds;
    d_time t;
    d_time time;
    d_number n;

    CHECKDATE(DTEXT("setSeconds"));
    if (getThisLocalTime(ret, othis, &t) == 0)
    {
	switch (argc)
	{
	    default:
	    case 2:
		ms = arglist[1].toNumber();
		seconds = arglist[0].toNumber();
		break;

	    case 1:
		ms = Date::msFromTime(t);
		seconds = arglist[0].toNumber();
		break;

	    case 0:
		ms = Date::msFromTime(t);
		seconds = Port::nan;
		break;
	}
	time = Date::MakeTime(Date::HourFromTime(t), Date::MinFromTime(t), seconds, ms);
	n = Date::TimeClip(Date::UTC(Date::MakeDate(Date::Day(t),time)));
	othis->value.number = n;
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, setUTCSeconds, 2)
{
    // ECMA 15.9.5.27
    d_number ms;
    d_number seconds;
    d_time t;
    d_time time;
    d_number n;

    CHECKDATE(DTEXT("setUTCSeconds"));
    if (getThisTime(ret, othis, &t) == 0)
    {
	switch (argc)
	{
	    default:
	    case 2:
		ms = arglist[1].toNumber();
		seconds = arglist[0].toNumber();
		break;

	    case 1:
		ms = Date::msFromTime(t);
		seconds = arglist[0].toNumber();
		break;

	    case 0:
		ms = Date::msFromTime(t);
		seconds = Port::nan;
		break;
	}
	time = Date::MakeTime(Date::HourFromTime(t), Date::MinFromTime(t), seconds, ms);
	n = Date::TimeClip(Date::MakeDate(Date::Day(t),time));
	othis->value.number = n;
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, setMinutes, 3)
{
    // ECMA 15.9.5.28
    d_number ms;
    d_number seconds;
    d_number minutes;
    d_time t;
    d_time time;
    d_number n;

    CHECKDATE(DTEXT("setMinutes"));
    if (getThisLocalTime(ret, othis, &t) == 0)
    {
	switch (argc)
	{
	    default:
	    case 3:
		ms      = arglist[2].toNumber();
		seconds = arglist[1].toNumber();
		minutes = arglist[0].toNumber();
		break;

	    case 2:
		ms      = Date::msFromTime(t);
		seconds = arglist[1].toNumber();
		minutes = arglist[0].toNumber();
		break;

	    case 1:
		ms      = Date::msFromTime(t);
		seconds = Date::SecFromTime(t);
		minutes = arglist[0].toNumber();
		break;

	    case 0:
		ms      = Date::msFromTime(t);
		seconds = Date::SecFromTime(t);
		minutes = Port::nan;
		break;
	}
	time = Date::MakeTime(Date::HourFromTime(t), minutes, seconds, ms);
	n = Date::TimeClip(Date::UTC(Date::MakeDate(Date::Day(t),time)));
	othis->value.number = n;
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, setUTCMinutes, 3)
{
    // ECMA 15.9.5.29
    d_number ms;
    d_number seconds;
    d_number minutes;
    d_time t;
    d_time time;
    d_number n;

    CHECKDATE(DTEXT("setUTCMinutes"));
    if (getThisTime(ret, othis, &t) == 0)
    {
	switch (argc)
	{
	    default:
	    case 3:
		ms      = arglist[2].toNumber();
		seconds = arglist[1].toNumber();
		minutes = arglist[0].toNumber();
		break;

	    case 2:
		ms      = Date::msFromTime(t);
		seconds = arglist[1].toNumber();
		minutes = arglist[0].toNumber();
		break;

	    case 1:
		ms      = Date::msFromTime(t);
		seconds = Date::SecFromTime(t);
		minutes = arglist[0].toNumber();
		break;

	    case 0:
		ms      = Date::msFromTime(t);
		seconds = Date::SecFromTime(t);
		minutes = Port::nan;
		break;
	}
	time = Date::MakeTime(Date::HourFromTime(t), minutes, seconds, ms);
	n = Date::TimeClip(Date::MakeDate(Date::Day(t),time));
	othis->value.number = n;
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, setHours, 4)
{
    // ECMA 15.9.5.30
    d_number ms;
    d_number seconds;
    d_number minutes;
    d_number hours;
    d_time t;
    d_time time;
    d_number n;

    CHECKDATE(DTEXT("setHours"));
    if (getThisLocalTime(ret, othis, &t) == 0)
    {
	switch (argc)
	{
	    default:
	    case 4:
		ms      = arglist[3].toNumber();
		seconds = arglist[2].toNumber();
		minutes = arglist[1].toNumber();
		hours   = arglist[0].toNumber();
		break;

	    case 3:
		ms      = Date::msFromTime(t);
		seconds = arglist[2].toNumber();
		minutes = arglist[1].toNumber();
		hours   = arglist[0].toNumber();
		break;

	    case 2:
		ms      = Date::msFromTime(t);
		seconds = Date::SecFromTime(t);
		minutes = arglist[1].toNumber();
		hours   = arglist[0].toNumber();
		break;

	    case 1:
		ms      = Date::msFromTime(t);
		seconds = Date::SecFromTime(t);
		minutes = Date::MinFromTime(t);
		hours   = arglist[0].toNumber();
		break;

	    case 0:
		ms      = Date::msFromTime(t);
		seconds = Date::SecFromTime(t);
		minutes = Date::MinFromTime(t);
		hours   = Port::nan;
		break;
	}
	time = Date::MakeTime(hours, minutes, seconds, ms);
	n = Date::TimeClip(Date::UTC(Date::MakeDate(Date::Day(t),time)));
	othis->value.number = n;
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, setUTCHours, 4)
{
    // ECMA 15.9.5.31
    d_number ms;
    d_number seconds;
    d_number minutes;
    d_number hours;
    d_time t;
    d_time time;
    d_number n;

    CHECKDATE(DTEXT("setUTCHours"));
    if (getThisTime(ret, othis, &t) == 0)
    {
	switch (argc)
	{
	    default:
	    case 4:
		ms      = arglist[3].toNumber();
		seconds = arglist[2].toNumber();
		minutes = arglist[1].toNumber();
		hours   = arglist[0].toNumber();
		break;

	    case 3:
		ms      = Date::msFromTime(t);
		seconds = arglist[2].toNumber();
		minutes = arglist[1].toNumber();
		hours   = arglist[0].toNumber();
		break;

	    case 2:
		ms      = Date::msFromTime(t);
		seconds = Date::SecFromTime(t);
		minutes = arglist[1].toNumber();
		hours   = arglist[0].toNumber();
		break;

	    case 1:
		ms      = Date::msFromTime(t);
		seconds = Date::SecFromTime(t);
		minutes = Date::MinFromTime(t);
		hours   = arglist[0].toNumber();
		break;

	    case 0:
		ms      = Date::msFromTime(t);
		seconds = Date::SecFromTime(t);
		minutes = Date::MinFromTime(t);
		hours   = Port::nan;
		break;
	}
	time = Date::MakeTime(hours, minutes, seconds, ms);
	n = Date::TimeClip(Date::MakeDate(Date::Day(t),time));
	othis->value.number = n;
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, setDate, 1)
{
    // ECMA 15.9.5.32
    d_number date;
    d_time t;
    d_time day;
    d_number n;

    CHECKDATE(DTEXT("setDate"));
    if (getThisLocalTime(ret, othis, &t) == 0)
    {
	if (!argc)
	    date = Port::nan;
	else
	    date = arglist[0].toNumber();
	day = Date::MakeDay(Date::YearFromTime(t), Date::MonthFromTime(t), date);
	n = Date::TimeClip(Date::UTC(Date::MakeDate(day, Date::TimeWithinDay(t))));
	othis->value.number = n;
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, setUTCDate, 1)
{
    // ECMA 15.9.5.33
    d_number date;
    d_time t;
    d_time day;
    d_number n;

    CHECKDATE(DTEXT("setUTCDate"));
    if (getThisTime(ret, othis, &t) == 0)
    {
	if (!argc)
	    date = Port::nan;
	else
	    date = arglist[0].toNumber();
	day = Date::MakeDay(Date::YearFromTime(t), Date::MonthFromTime(t), date);
	n = Date::TimeClip(Date::MakeDate(day, Date::TimeWithinDay(t)));
	othis->value.number = n;
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, setMonth, 2)
{
    // ECMA 15.9.5.34
    d_number date;
    d_number month;
    d_time t;
    d_time day;
    d_number n;

    CHECKDATE(DTEXT("setMonth"));
    if (getThisLocalTime(ret, othis, &t) == 0)
    {
	switch (argc)
	{   default:
	    case 2:
		month = arglist[0].toNumber();
		date  = arglist[1].toNumber();
		break;

	    case 1:
		month = arglist[0].toNumber();
		date  = Date::DateFromTime(t);
		break;

	    case 0:
		month = Port::nan;
		date  = Date::DateFromTime(t);
		break;
	}
	day = Date::MakeDay(Date::YearFromTime(t), month, date);
	n = Date::TimeClip(Date::UTC(Date::MakeDate(day, Date::TimeWithinDay(t))));
	othis->value.number = n;
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, setUTCMonth, 2)
{
    // ECMA 15.9.5.35
    d_number date;
    d_number month;
    d_time t;
    d_time day;
    d_number n;

    CHECKDATE(DTEXT("setUTCMonth"));
    if (getThisTime(ret, othis, &t) == 0)
    {
	switch (argc)
	{   default:
	    case 2:
		month = arglist[0].toNumber();
		date  = arglist[1].toNumber();
		break;

	    case 1:
		month = arglist[0].toNumber();
		date  = Date::DateFromTime(t);
		break;

	    case 0:
		month = Port::nan;
		date  = Date::DateFromTime(t);
		break;
	}
	day = Date::MakeDay(Date::YearFromTime(t), month, date);
	n = Date::TimeClip(Date::MakeDate(day, Date::TimeWithinDay(t)));
	othis->value.number = n;
	Vnumber::putValue(ret, n);
    }
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, setFullYear, 3)
{
    // ECMA 15.9.5.36
    d_number date;
    d_number month;
    d_number year;
    d_time t;
    d_time day;
    d_number n;

    CHECKDATE(DTEXT("setFullYear"));
    if (getThisLocalTime(ret, othis, &t))
	t = 0;
printf("t = %f\n", t);
    switch (argc)
    {   default:
	case 3:
	    date  = arglist[2].toNumber();
	    month = arglist[1].toNumber();
	    year  = arglist[0].toNumber();
	    break;

	case 2:
	    date  = Date::DateFromTime(t);
	    month = arglist[1].toNumber();
	    year  = arglist[0].toNumber();
	    break;

	case 1:
	    date  = Date::DateFromTime(t);
	    month = Date::MonthFromTime(t);
	    year  = arglist[0].toNumber();
	    break;

	case 0:
	    month = Date::MonthFromTime(t);
	    date  = Date::DateFromTime(t);
	    year  = Port::nan;
	    break;
    }
printf("year = %f, month = %f, date = %f\n", year, month, date);
    day = Date::MakeDay(year, month, date);
printf("day = %f\n", day);
    n = Date::TimeClip(Date::UTC(Date::MakeDate(day, Date::TimeWithinDay(t))));
printf("n = %f\n", n);
    othis->value.number = n;
    Vnumber::putValue(ret, n);
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, setUTCFullYear, 3)
{
    // ECMA 15.9.5.37
    d_number date;
    d_number month;
    d_number year;
    d_time t;
    d_time day;
    d_number n;

    CHECKDATE(DTEXT("setUTCFullYear"));
    getThisTime(ret, othis, &t);
    if (Port::isnan(t))
	t = 0;
    switch (argc)
    {   default:
	case 3:
	    month = arglist[2].toNumber();
	    date  = arglist[1].toNumber();
	    year  = arglist[0].toNumber();
	    break;

	case 2:
	    month = Date::MonthFromTime(t);
	    date  = arglist[1].toNumber();
	    year  = arglist[0].toNumber();
	    break;

	case 1:
	    month = Date::MonthFromTime(t);
	    date  = Date::DateFromTime(t);
	    year  = arglist[0].toNumber();
	    break;

	case 0:
	    month = Date::MonthFromTime(t);
	    date  = Date::DateFromTime(t);
	    year  = Port::nan;
	    break;
    }
    day = Date::MakeDay(year, month, date);
    n = Date::TimeClip(Date::MakeDate(day, Date::TimeWithinDay(t)));
    othis->value.number = n;
    Vnumber::putValue(ret, n);
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, setYear, 1)
{
    // ECMA 15.9.5.38
    d_number date;
    d_number month;
    d_number year;
    d_time t;
    d_time day;
    d_number n;

    CHECKDATE(DTEXT("setYear"));
    if (getThisLocalTime(ret, othis, &t))
	t = 0;
    switch (argc)
    {   default:
	case 1:
	    month = Date::MonthFromTime(t);
	    date  = Date::DateFromTime(t);
	    year  = arglist[0].toNumber();
	    if (0 <= year && year <= 99)
		year += 1900;
	    day = Date::MakeDay(year, month, date);
	    n = Date::TimeClip(Date::UTC(Date::MakeDate(day, Date::TimeWithinDay(t))));
	    break;

	case 0:
	    n = Port::nan;
	    break;
    }
    othis->value.number = n;
    Vnumber::putValue(ret, n);
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, toLocaleString, 0)
{
    // ECMA 15.9.5.39
    d_string s;
    d_time t;

    CHECKDATE(DTEXT("toLocaleString"));
    if (getThisLocalTime(ret, othis, &t))
	t = 0;

    GC_LOG();
    s = dateToString(cc, t, TFLocaleString);
    Vstring::putValue(ret, s);
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, toLocaleDateString, 0)
{
    // ECMA 15.9.5.6
    d_string s;
    d_time t;

    CHECKDATE(DTEXT("toLocaleDateString"));
    if (getThisLocalTime(ret, othis, &t))
	t = 0;

    s = dateToString(cc, t, TFLocaleDateString);
    Vstring::putValue(ret, s);
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, toLocaleTimeString, 0)
{
    // ECMA 15.9.5.7
    d_string s;
    d_time t;

    CHECKDATE(DTEXT("toLocaleTimeString"));
    if (getThisLocalTime(ret, othis, &t))
	t = 0;
    s = dateToString(cc, t, TFLocaleTimeString);
    Vstring::putValue(ret, s);
    return NULL;
}

BUILTIN_FUNCTION2(Ddate_prototype_, toUTCString, 0)
{
    // ECMA 15.9.5.40
    d_string s;
    d_time t;

    CHECKDATE(DTEXT("toUTCString"));
    if (getThisTime(ret, othis, &t))
	t = 0;
    GC_LOG();
    s = d_string_ctor(Date::ToUTCString(t));
    Vstring::putValue(ret, s);
    return NULL;
}

/* ===================== Ddate_prototype ==================== */

struct Ddate_prototype : Ddate
{
    Ddate_prototype(ThreadContext *tc);
};

Ddate_prototype::Ddate_prototype(ThreadContext *tc)
    : Ddate(tc->Dobject_prototype)
{
    Dobject *f = tc->Dfunction_prototype;

    Put(TEXT_constructor, tc->Ddate_constructor, DontEnum);

    static NativeFunctionData nfd[] =
    {
    #define X(name,len) { &TEXT_##name, #name, Ddate_prototype_##name, len }
	X(toString, 0),
	X(toDateString, 0),
	X(toTimeString, 0),
	X(valueOf, 0),
	X(getTime, 0),
	X(getVarDate, 0),
	X(getYear, 0),
	X(getFullYear, 0),
	X(getUTCFullYear, 0),
	X(getMonth, 0),
	X(getUTCMonth, 0),
	X(getDate, 0),
	X(getUTCDate, 0),
	X(getDay, 0),
	X(getUTCDay, 0),
	X(getHours, 0),
	X(getUTCHours, 0),
	X(getMinutes, 0),
	X(getUTCMinutes, 0),
	X(getSeconds, 0),
	X(getUTCSeconds, 0),
	X(getMilliseconds, 0),
	X(getUTCMilliseconds, 0),
	X(getTimezoneOffset, 0),
	X(setTime, 1),
	X(setMilliseconds, 1),
	X(setUTCMilliseconds, 1),
	X(setSeconds, 2),
	X(setUTCSeconds, 2),
	X(setMinutes, 3),
	X(setUTCMinutes, 3),
	X(setHours, 4),
	X(setUTCHours, 4),
	X(setDate, 1),
	X(setUTCDate, 1),
	X(setMonth, 2),
	X(setUTCMonth, 2),
	X(setFullYear, 3),
	X(setUTCFullYear, 3),
	X(setYear, 1),
	X(toLocaleString, 0),
	X(toLocaleDateString, 0),
	X(toLocaleTimeString, 0),
	X(toUTCString, 0),

	// Map toGMTString() onto toUTCString(), per ECMA 15.9.5.41
	{ &TEXT_toGMTString, "toGMTstring", Ddate_prototype_toUTCString, 0 },
    };

    DnativeFunction::init(this, nfd, sizeof(nfd) / sizeof(nfd[0]), 0);
}

/* ===================== Ddate ==================== */

Ddate::Ddate(d_number n)
    : Dobject(Ddate::getPrototype())
{
    classname = TEXT_Date;
    Vnumber::putValue(&value, n);
}

Ddate::Ddate(Dobject *prototype)
    : Dobject(prototype)
{
    classname = TEXT_Date;
    Vnumber::putValue(&value, Port::nan);
}

Dfunction *Ddate::getConstructor()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Ddate_constructor;
}

Dobject *Ddate::getPrototype()
{
    ThreadContext *tc = ThreadContext::getThreadContext();
    assert(tc);
    return tc->Ddate_prototype;
}

void Ddate::init(ThreadContext *tc)
{
    GC_LOG();
    tc->Ddate_constructor = new(&tc->mem) Ddate_constructor(tc);
    GC_LOG();
    tc->Ddate_prototype = new(&tc->mem) Ddate_prototype(tc);

    tc->Ddate_constructor->Put(TEXT_prototype, tc->Ddate_prototype, DontEnum | DontDelete | ReadOnly);
}


