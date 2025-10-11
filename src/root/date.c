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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>

#include "root.h"
#include "port.h"
#include "dateparse.h"
#include "date.h"
#include "printf.h"
#include "mutex.h"

static long msPerDay = 86400000;
static long HoursPerDay = 24;
static long MinutesPerHour = 60;
static long msPerMinute = 60 * 1000;
static long msPerHour = 60 * 60 * 1000;
static long msPerSecond = 1000;

// Number of milliseconds from midnight, Dec. 30, 1899 to Jan. 1, 1970
static double vardateoffset = 2209161600000.0;

#define STRFTIME	1	// 1: use strftime() for formatting
				// 0: roll our own formatting

d_time LocalTZA = 0;

char Date::daystr[] = "SunMonTueWedThuFriSat";
char Date::monstr[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

#if STRFTIME
static Mutex strftime_mutex;	// strftime() is not thread-safe
#else
static char *dayname[] =
{ "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

static char *monthname[] =
{ "January", "February", "March", "April", "May", "June", "July", "August",
  "September", "October", "November", "December" };
#endif

/**********************************
 * Convert from d_time to VT_DATE.
 * VT_DATE is number of days since midnight, Dec. 30, 1899.
 * From MSDN:
 *	"A variant time is stored as an 8-byte real value (double),
 *	representing a date between January 1, 100 and December 31, 9999,
 *	inclusive. The value 2.0 represents January 1, 1900;
 *	3.0 represents January 2, 1900, and so on. Adding 1 to the value
 *	increments the date by a day. The fractional part of the value
 *	represents the time of day. Therefore, 2.5 represents noon on January 1,
 *	1900; 3.25 represents 6:00 A.M. on January 2, 1900, and so on.
 *	Negative numbers represent the dates prior to December 30, 1899."
 */

double Date::toVarDate(d_time n)
{
    double d;

    d = (n + vardateoffset) / msPerDay;
    return d;
}

/**********************************
 * Convert from VT_DATE to d_time.
 */

d_time Date::VarDateToTime(double d)
{
    d_time n;

    n = d * msPerDay - vardateoffset;
    return n;
}

d_time Date::dmod(d_time n, d_time d)
{   d_time r;

    r = fmod(n, d);
    if (r < 0)
	r += d;
    return r;
}

d_time Date::HourFromTime(d_time t)
{
    return dmod(Port::floor(t / msPerHour), HoursPerDay);
}

d_time Date::MinFromTime(d_time t)
{
    return dmod(Port::floor(t / msPerMinute), MinutesPerHour);
}

d_time Date::SecFromTime(d_time t)
{
    return dmod(Port::floor(t / msPerSecond), 60);
}

d_time Date::msFromTime(d_time t)
{
    return dmod(t, msPerSecond);
}

d_time Date::TimeWithinDay(d_time t)
{
    return dmod(t, msPerDay);
}

d_time Date::toInteger(d_time n)
{
    return (n >= 0)
	? Port::floor(n)
	: - Port::floor(-n);
}

int Date::Day(d_time t)
{
    return (int)Port::floor(t / msPerDay);
}

int Date::LeapYear(int y)
{
    return ((y & 3) == 0 &&
	    (y % 100 || (y % 400) == 0));
}

int Date::DaysInYear(int y)
{
    return 365 + LeapYear(y);
}

int Date::DayFromYear(int y)
{
    return (int) (365 * (y - 1970) +
		Port::floor((y - 1969.0) / 4) -
		Port::floor((y - 1901.0) / 100) +
		Port::floor((y - 1601.0) / 400));
}

d_time Date::TimeFromYear(int y)
{
    return (d_time)msPerDay * DayFromYear(y);
}

int Date::YearFromTime(d_time t)
{   int y;

    // Hazard a guess
    y = 1970 + (int) (t / (365.2425 * msPerDay));

    if (TimeFromYear(y) <= t)
    {
	while (TimeFromYear(y + 1) <= t)
	    y++;
    }
    else
    {
	do
	{
	    y--;
	}
	while (TimeFromYear(y) > t);
    }
    return y;
}

int Date::inLeapYear(d_time t)
{
    return LeapYear(YearFromTime(t));
}

int Date::MonthFromTime(d_time t)
{
    int day;
    int month;
    int year;

    year = YearFromTime(t);
    day = Day(t) - DayFromYear(year);

    if (day < 59)
    {
	if (day < 31)
	{   assert(day >= 0);
	    month = 0;
	}
	else
	    month = 1;
    }
    else
    {
	day -= LeapYear(year);
	if (day < 212)
	{
	    if (day < 59)
		month = 1;
	    else if (day < 90)
		month = 2;
	    else if (day < 120)
		month = 3;
	    else if (day < 151)
		month = 4;
	    else if (day < 181)
		month = 5;
	    else
		month = 6;
	}
	else
	{
	    if (day < 243)
		month = 7;
	    else if (day < 273)
		month = 8;
	    else if (day < 304)
		month = 9;
	    else if (day < 334)
		month = 10;
	    else if (day < 365)
		month = 11;
	    else
	    {	assert(0);
		month = -1;	// keep /W4 happy
	    }
	}
    }
    return month;
}

int Date::DateFromTime(d_time t)
{
    int day;
    int leap;
    int month;
    int year;
    int date;

    year = YearFromTime(t);
    day = Day(t) - DayFromYear(year);
    leap = LeapYear(year);
    month = MonthFromTime(t);
    switch (month)
    {
	case 0:	 date = day +   1;		break;
	case 1:	 date = day -  30;		break;
	case 2:	 date = day -  58 - leap;	break;
	case 3:	 date = day -  89 - leap;	break;
	case 4:	 date = day - 119 - leap;	break;
	case 5:	 date = day - 150 - leap;	break;
	case 6:	 date = day - 180 - leap;	break;
	case 7:	 date = day - 211 - leap;	break;
	case 8:	 date = day - 242 - leap;	break;
	case 9:	 date = day - 272 - leap;	break;
	case 10: date = day - 303 - leap;	break;
	case 11: date = day - 333 - leap;	break;
	default:
	    assert(0);
	    date = -1;	// keep /W4 happy
    }
    return date;
}

int Date::WeekDay(d_time t)
{   int w;

    w = ((int)Day(t) + 4) % 7;
    if (w < 0)
	w += 7;
    return w;
}

// Convert from UTC to local time

d_time Date::LocalTime(d_time t)
{
    return t + LocalTZA + DaylightSavingTA(t);
}

// Convert from local time to UTC

d_time Date::UTC(d_time t)
{
    return t - LocalTZA - DaylightSavingTA(t - LocalTZA);
}

d_time Date::MakeTime(d_time hour, d_time min, d_time sec, d_time ms)
{
    if (!Port::isfinite(hour) ||
	!Port::isfinite(min) ||
	!Port::isfinite(sec) ||
	!Port::isfinite(ms))
	return Port::nan;

    hour = toInteger(hour);
    min = toInteger(min);
    sec = toInteger(sec);
    ms = toInteger(ms);

    return hour * msPerHour +
	   min * msPerMinute +
	   sec * msPerSecond +
	   ms;
}

// ECMA 15.9.1.12

d_time Date::MakeDay(d_time year, d_time month, d_time date)
{   d_time t;
    int y;
    int m;
    int leap;
    static int mdays[12] =
    { 0,31,59,90,120,151,181,212,243,273,304,334 };

    if (!Port::isfinite(year) ||
	!Port::isfinite(month) ||
	!Port::isfinite(date))
	return Port::nan;

    year = toInteger(year);
    month = toInteger(month);
    date = toInteger(date);

    y = (int)(year + Port::floor(month / 12));
    m = (int)dmod(month, 12);

    leap = LeapYear(y);
    t = TimeFromYear(y) + (d_time)mdays[m] * msPerDay;
    if (leap && month >= 2)
	t += msPerDay;

    if (YearFromTime(t) != y ||
	MonthFromTime(t) != m ||
	DateFromTime(t) != 1)
	return Port::nan;

    return Day(t) + date - 1;
}

d_time Date::MakeDate(d_time day, d_time time)
{
    if (!Port::isfinite(day) ||
	!Port::isfinite(time))
	return Port::nan;

    return day * msPerDay + time;
}

d_time Date::TimeClip(d_time time)
{
    //message(DTEXT("TimeClip(%g) = %g\n"), time, toInteger(time));
    if (!Port::isfinite(time) ||
	time > 8.64e15 ||
	time < -8.64e15)
	return Port::nan;
    return toInteger(time);
}

dchar *Date::ToString(d_time time)
{
    d_time t;
    dchar sign;
    int hr;
    int mn;
    d_time offset;
    d_time dst;

    // Years are supposed to be -285616 .. 285616, or 7 digits
    // "Tue Apr 02 02:04:57 GMT-0800 1996"
    char buffer[DATE_BUFSIZE];

    if (Port::isnan(time))
	return DTEXT("Invalid Date");

    dst = DaylightSavingTA(time);
    offset = LocalTZA + dst;
    t = time + offset;
    sign = '+';
    if (offset < 0)
    {	sign = '-';
//	offset = -offset;
	offset = -(LocalTZA + dst);
    }

    mn = (int)(offset / msPerMinute);
    hr = mn / 60;
    mn %= 60;

    //message(DTEXT("hr = %d, offset = %g, LocalTZA = %g, dst = %g, + = %g\n"), hr, offset, LocalTZA, dst, LocalTZA + dst);
    SPRINTF(
        buffer, DATE_BUFSIZE, "%.3s %.3s %02d %02d:%02d:%02d GMT%c%02d%02d %d",
	&daystr[WeekDay(t) * 3],
	&monstr[MonthFromTime(t) * 3],
	DateFromTime(t),
	(int)HourFromTime(t), (int)MinFromTime(t), (int)SecFromTime(t),
	sign, hr, mn,
	YearFromTime(t));

    // Ensure no buggy buffer overflows
    assert(strlen(buffer) < sizeof(buffer) / sizeof(buffer[0]));

    return Dchar::dup(buffer);
}

dchar *Date::ToDateString(d_time time)
{
    d_time t;
    d_time offset;
    d_time dst;

    // Years are supposed to be -285616 .. 285616, or 7 digits
    // "Tue Apr 02 1996"
    char buffer[DATE_BUFSIZE];

    if (Port::isnan(time))
	return DTEXT("Invalid Date");

    dst = DaylightSavingTA(time);
    offset = LocalTZA + dst;
    t = time + offset;

    SPRINTF(
        buffer, DATE_BUFSIZE, "%.3s %.3s %02d %d",
	&daystr[WeekDay(t) * 3],
	&monstr[MonthFromTime(t) * 3],
	DateFromTime(t),
	YearFromTime(t));

    // Ensure no buggy buffer overflows
    assert(strlen(buffer) < sizeof(buffer) / sizeof(buffer[0]));

    return Dchar::dup(buffer);
}

dchar *Date::ToTimeString(d_time time)
{
    d_time t;
    dchar sign;
    int hr;
    int mn;
    d_time offset;
    d_time dst;

    // "02:04:57 GMT-0800"
    char buffer[DATE_BUFSIZE];

    if (Port::isnan(time))
	return DTEXT("Invalid Date");

    dst = DaylightSavingTA(time);
    offset = LocalTZA + dst;
    t = time + offset;
    sign = '+';
    if (offset < 0)
    {	sign = '-';
//	offset = -offset;
	offset = -(LocalTZA + dst);
    }

    mn = (int)(offset / msPerMinute);
    hr = mn / 60;
    mn %= 60;

    //message(DTEXT("hr = %d, offset = %g, LocalTZA = %g, dst = %g, + = %g\n"), hr, offset, LocalTZA, dst, LocalTZA + dst);

    SPRINTF(buffer,DATE_BUFSIZE, "%02d:%02d:%02d GMT%c%02d%02d",
	(int)HourFromTime(t), (int)MinFromTime(t), (int)SecFromTime(t),
	sign, hr, mn);

    // Ensure no buggy buffer overflows
    assert(strlen(buffer) < sizeof(buffer) / sizeof(buffer[0]));

    return Dchar::dup(buffer);
}

dchar *Date::ToLocaleString(d_time t)
{
#if STRFTIME
    time_t ltime;
    struct tm *timeptr;
    char buffer[128];
    size_t s;

    ltime = (time_t)(t / 1000);
    timeptr = gmtime(&ltime);
    strftime_mutex.acquire();
    s = strftime(buffer, sizeof(buffer), "%c", timeptr);
    strftime_mutex.release();
    if (s == 0)
    {
	// Error
	return DTEXT("nan");
    }
    return Dchar::dup(buffer);
#else

    // Years are supposed to be -285616 .. 285616, or 7 digits
    // "Tue Apr 02 02:04:57 1996"
    // "Friday, April 13, 02:04:57 2001"
    char buffer[9 + 2 + 9 + 1 + 2 + 2 + 8 + 1 + 7 + 1];

    sprintf(buffer, "%s, %s %d, %02d:%02d:%02d %d",
	dayname[Date::WeekDay(t)],
	monthname[Date::MonthFromTime(t)],
	Date::DateFromTime(t),
	(int)Date::HourFromTime(t), (int)Date::MinFromTime(t), (int)Date::SecFromTime(t),
	Date::YearFromTime(t));

    // Ensure no buggy buffer overflows
    assert(strlen(buffer) < sizeof(buffer) / sizeof(buffer[0]));

    return Dchar::dup(buffer);
#endif
}

dchar *Date::ToLocaleDateString(d_time t)
{
#if STRFTIME
    time_t ltime;
    struct tm *timeptr;
    char buffer[128];
    size_t s;

    ltime = (time_t)(t / 1000);
    timeptr = gmtime(&ltime);
    strftime_mutex.acquire();
    s = strftime(buffer, sizeof(buffer), "%x", timeptr);
    strftime_mutex.release();
    if (s == 0)
    {
	// Error
	return DTEXT("nan");
    }
    return Dchar::dup(buffer);
#else
    // Years are supposed to be -285616 .. 285616, or 7 digits
    // "Friday, April 13, 2001"
    char buffer[9 + 2 + 9 + 1 + 2 + 2 + 7 + 1];

    sprintf(buffer, "%s, %s %d, %d",
	dayname[Date::WeekDay(t)],
	monthname[Date::MonthFromTime(t)],
	Date::DateFromTime(t),
	Date::YearFromTime(t));

    // Ensure no buggy buffer overflows
    assert(strlen(buffer) < sizeof(buffer) / sizeof(buffer[0]));

    return Dchar::dup(buffer);
#endif
}

dchar *Date::ToLocaleTimeString(d_time t)
{
#if STRFTIME
    time_t ltime;
    struct tm *timeptr;
    char buffer[128];
    size_t s;

    ltime = (time_t)(t / 1000);
    timeptr = gmtime(&ltime);
    strftime_mutex.acquire();
    s = strftime(buffer, sizeof(buffer), "%X", timeptr);
    strftime_mutex.release();
    if (s == 0)
    {
	// Error
	return DTEXT("nan");
    }
    return Dchar::dup(buffer);
#else
    // "02:04:57"
    char buffer[20 + 7 + 1];

    sprintf(buffer, "%02d:%02d:%02d",
	(int)Date::HourFromTime(t), (int)Date::MinFromTime(t), (int)Date::SecFromTime(t));

    // Ensure no buggy buffer overflows
    assert(strlen(buffer) < sizeof(buffer) / sizeof(buffer[0]));

    return Dchar::dup(buffer);
#endif
}

dchar *Date::ToUTCString(d_time t)
{
    // Years are supposed to be -285616 .. 285616, or 7 digits
    // "Tue, 02 Apr 1996 02:04:57 GMT"
    char buffer[25 + 7 + 1];

    sprintf(buffer, "%.3s, %02d %.3s %d %02d:%02d:%02d UTC",
	&Date::daystr[Date::WeekDay(t) * 3], Date::DateFromTime(t),
	&Date::monstr[Date::MonthFromTime(t) * 3], Date::YearFromTime(t),
	(int)Date::HourFromTime(t), (int)Date::MinFromTime(t), (int)Date::SecFromTime(t));

    // Ensure no buggy buffer overflows
    assert(strlen(buffer) < sizeof(buffer) / sizeof(buffer[0]));

    return Dchar::dup(buffer);
}


d_time Date::parse(dchar *s)
{
/* Test vectors:

1995-01-24                ;790898400
+ Wed, 16 Jun 94 07:29:35 CST    	     ;771773375
+ Wed, 16 Nov 94 07:29:35 CST 	     ;784992575
+ Mon, 21 Nov 94 07:42:23 CST 	     ;785425343
+ Mon, 21 Nov 94 04:28:18 CST 	     ;785413698
+ Tue, 15 Nov 94 09:15:10 GMT 	     ;784890910
+ Wed, 16 Nov 94 09:39:49 GMT 	     ;784978789
+ Wed, 16 Nov 94 09:23:17 GMT 	     ;784977797
+ Wed, 16 Nov 94 12:39:49 GMT 	     ;784989589
+ Wed, 16 Nov 94 14:03:06 GMT 	     ;784994586
+ Wed, 16 Nov 94 05:30:51 CST 	     ;784985451
+ Thu, 17 Nov 94 03:19:30 CST 	     ;785063970
+ Mon, 21 Nov 94 14:05:32 GMT 	     ;785426732
+ Mon, 14 Nov 94 15:08:49 CST 	     ;784847329
+ Wed, 16 Nov 94 14:48:06 GMT 	     ;784997286
+ Thu, 17 Nov 94 14:22:03 GMT 	     ;785082123
+ Wed, 16 Nov 94 14:36:00 GMT 	     ;784996560
+ Wed, 16 Nov 94 09:23:17 GMT 	     ;784977797
+ Wed, 16 Nov 94 10:01:43 GMT 	     ;784980103
+ Wed, 16 Nov 94 15:03:35 GMT 	     ;784998215
+ Mon, 21 Nov 94 13:55:19 GMT 	     ;785426119
+ Wed, 16 Nov 94 08:46:11 CST 	     ;784997171
+ Wed, 9 Nov 1994 09:50:32 -0500 (EST) ;784392632
+ Thu, 13 Oct 94 10:13:13 -0700	     ;782068393
+ Sat, 19 Nov 1994 16:59:14 +0100      ;785260754
+ Thu, 3 Nov 94 14:10:47 EST 	     ;783889847
+ Thu, 3 Nov 94 21:51:09 EST 	     ;783917469
+ Fri, 4 Nov 94 9:24:52 EST 	     ;783959092
+ Wed, 9 Nov 94 09:38:54 EST 	     ;784391934
+ Mon, 14 Nov 94 13:20:12 EST 	     ;784837212
+ Wed, 16 Nov 94 17:09:13 EST 	     ;785023753
+ Tue, 15 Nov 94 12:27:01 PST 	     ;784931221
+ Fri, 18 Nov 1994 07:34:05 -0600      ;785165645
+ Mon, 21 Nov 94 14:34:28 -0500 	     ;785446468
+ Fri, 18 Nov 1994 12:05:47 -0800 (PST);785189147
+ Fri, 18 Nov 1994 12:36:26 -0800 (PST);785190986
+ Wed, 16 Nov 1994 15:58:58 GMT 	     ;785001538
+ Sun, 06 Nov 94 14:27:40 -0500 	     ;784150060
+ Mon, 07 Nov 94 08:20:13 -0500 	     ;784214413
+ Mon, 07 Nov 94 16:48:42 -0500 	     ;784244922
+ Wed, 09 Nov 94 15:46:16 -0500 	     ;784413976
+ Sun, 6 Nov 1994 02:38:17 -0800 	     ;784118297
+ Tue, 1 Nov 1994 13:53:49 -0500 	     ;783716029
+ Tue, 15 Nov 94 08:31:59 +0100 	     ;784884719
+ Sun, 6 Nov 1994 11:09:12 -0500 (IST) ;784138152
+ Fri, 4 Nov 94 12:52:10 EST 	     ;783971530
+ Mon, 31 Oct 1994 14:17:39 -0500 (EST);783631059
+ Mon, 14 Nov 94 11:25:00 CST 	     ;784833900
+ Mon, 14 Nov 94 13:26:29 CST 	     ;784841189
+ Fri, 18 Nov 94 8:42:47 CST 	     ;785169767
+ Thu, 17 Nov 94 14:32:01 +0900 	     ;785050321
+ Wed, 2 Nov 94 18:16:31 +0100 	     ;783796591
+ Fri, 18 Nov 94 10:46:26 +0100 	     ;785151986
+ Tue, 8 Nov 1994 22:39:28 +0200 	     ;784327168
+ Wed, 16 Nov 1994 10:01:08 -0500 (EST);784998068
+ Wed, 2 Nov 1994 16:59:42 -0800 	     ;783824382
+ Wed, 9 Nov 94 10:00:23 PST 	     ;784404023
+ Fri, 18 Nov 94 17:01:43 PST 	     ;785206903
+ Mon, 14 Nov 1994 14:47:46 -0500      ;784842466
+ Mon, 21 Nov 1994 04:56:04 -0500 (EST);785411764
+ Mon, 21 Nov 1994 11:50:12 -0800      ;785447412
+ Sat, 5 Nov 1994 14:04:16 -0600 (CST) ;784065856
+ Sat, 05 Nov 94 13:10:13 MST 	     ;784066213
+ Wed, 02 Nov 94 10:47:48 -0800 	     ;783802068
+ Wed, 02 Nov 94 13:19:15 -0800 	     ;783811155
+ Thu, 03 Nov 94 15:27:07 -0800 	     ;783905227
+ Fri, 04 Nov 94 09:12:12 -0800 	     ;783969132
+ Wed, 9 Nov 1994 10:13:03 +0000 (GMT) ;784375983
+ Wed, 9 Nov 1994 15:28:37 +0000 (GMT) ;784394917
+ Wed, 2 Nov 1994 17:37:41 +0100 (MET) ;783794261
+ 05 Nov 94 14:22:19 PST 		     ;784074139
+ 16 Nov 94 22:28:20 PST 		     ;785053700
+ Tue, 1 Nov 1994 19:51:15 -0800 	     ;783748275
+ Wed, 2 Nov 94 12:21:23 GMT 	     ;783778883
+ Fri, 18 Nov 94 18:07:03 GMT 	     ;785182023
+ Wed, 16 Nov 1994 11:26:27 -0500      ;785003187
+ Sun, 6 Nov 1994 13:48:49 -0500 	     ;784147729
+ Tue, 8 Nov 1994 13:19:37 -0800 	     ;784329577
+ Fri, 18 Nov 1994 11:01:12 -0800      ;785185272
+ Mon, 21 Nov 1994 00:47:58 -0500      ;785396878
+ Mon, 7 Nov 1994 14:22:48 -0800 (PST) ;784246968
+ Wed, 16 Nov 1994 15:56:45 -0800 (PST);785030205
+ Thu, 3 Nov 1994 13:17:47 +0000 	     ;783868667
+ Wed, 9 Nov 1994 17:32:50 -0500 (EST) ;784420370
+ Wed, 9 Nov 94 16:31:52 PST	     ;784427512
+ Wed, 09 Nov 94 10:41:10 -0800	     ;784406470
+ Wed, 9 Nov 94 08:42:22 MST	     ;784395742
+ Mon, 14 Nov 1994 08:32:13 -0800	     ;784830733
+ Mon, 14 Nov 1994 11:34:32 -0500 (EST);784830872
+ Mon, 14 Nov 94 16:48:09 GMT	     ;784831689
+ Tue, 15 Nov 1994 10:27:33 +0000      ;784895253
+ Wed, 02 Nov 94 13:56:54 MST 	     ;783809814
+ Thu, 03 Nov 94 15:24:45 MST 	     ;783901485
+ Thu, 3 Nov 1994 15:13:53 -0700 (MST) ;783900833
+ Fri, 04 Nov 94 08:15:13 MST 	     ;783962113
+ Thu, 3 Nov 94 18:15:47 EST	     ;783904547
+ Tue, 08 Nov 94 07:02:33 MST 	     ;784303353
+ Thu, 3 Nov 94 18:15:47 EST	     ;783904547
+ Tue, 15 Nov 94 07:26:05 MST 	     ;784909565
+ Wed, 2 Nov 1994 00:00:55 -0600 (CST) ;783756055
+ Sun, 6 Nov 1994 01:19:13 -0600 (CST) ;784106353
+ Mon, 7 Nov 1994 23:16:57 -0600 (CST) ;784271817
+ Tue, 08 Nov 1994 13:21:21 -0600	     ;784322481
+ Mon, 07 Nov 94 13:47:37 PST          ;784244857
+ Tue, 08 Nov 94 11:23:19 PST 	     ;784322599
+ Tue, 01 Nov 1994 11:28:25 -0800      ;783718105
+ Tue, 15 Nov 1994 13:11:47 -0800      ;784933907
+ Tue, 15 Nov 1994 13:18:38 -0800      ;784934318
+ Tue, 15 Nov 1994 0:18:38 -0800 	     ;784887518

*/

    DateParse dp;
    d_time n;
    d_time day;
    d_time time;

    if (dp.parse(s))
    {
	//message(DTEXT("year = %d, month = %d, day = %d\n"), dp.year, dp.month, dp.day);
	//message(DTEXT("%02d:%02d:%02d.%03d\n"), dp.hours, dp.minutes, dp.seconds, dp.ms);
	//message(DTEXT("weekday = %d, ampm = %d, tzcorrection = %d\n"), dp.weekday, dp.ampm, dp.tzcorrection);

	time = MakeTime(dp.hours, dp.minutes, dp.seconds, dp.ms);
	if (dp.tzcorrection == TZCORRECTION_NAN)
	    time -= LocalTZA;
	else
	    time += (d_time)dp.tzcorrection * msPerHour;
	day = MakeDay(dp.year, dp.month - 1, dp.day);
	n = MakeDate(day,time);
	if (dp.tzcorrection == TZCORRECTION_NAN)
	    n -= DaylightSavingTA(n);
	n = TimeClip(n);
    }
    else
    {
	n = Port::nan;		// erroneous date string
    }
    return n;
}

void Date::init()
{
    LocalTZA = Date::getLocalTZA();
    //message(DTEXT("LocalTZA = %g, %g\n"), LocalTZA, LocalTZA / msPerHour);
}

/*
*
* TODO: A more portable way to do this is to use the tzset() function, which
* exists on both windows and linux.
*
*/

#if defined(linux)
#include <time.h>
#include <sys/time.h>

d_time Date::time()
{   struct timeval tv;

    if (gettimeofday(&tv, NULL))
    {	// Some error happened - try time() instead
	return ::time(NULL) * 1000.0;
    }

    return tv.tv_sec * 1000.0 + (tv.tv_usec / 1000);
}

d_time Date::getLocalTZA()
{
    time_t t;

    ::time(&t);
    localtime(&t);	// this will set timezone
    return -(timezone * 1000);
}

/*
 * Get daylight savings time adjust for time dt.
 */

int Date::DaylightSavingTA(d_time dt)
{
    struct tm *tmp;
    time_t t;

    t = (time_t) (dt / msPerSecond);	// need range check
    tmp = localtime(&t);
    if (tmp->tm_isdst > 0)
	// BUG: Assume daylight savings time is plus one hour.
	return 60L * 60L * 1000L;

    return 0;
}

#elif defined(_WIN32)

#undef TEXT
#include <windows.h>
#include <time.h>

static double SYSTEMTIME2d_time(SYSTEMTIME *st, d_time t)
{
    d_time n;
    d_time day;
    d_time time;

    if (st->wYear)
    {
	time = Date::MakeTime(st->wHour, st->wMinute, st->wSecond, st->wMilliseconds);
	day = Date::MakeDay(st->wYear, st->wMonth - 1, st->wDay);
    }
    else
    {	// wDayOfWeek is weekday, wDay is which week in the month
	int year;
	int wd;
	int mday;
	int month;
	d_time x;

	year = Date::YearFromTime(t);
	month = st->wMonth - 1;
	x = Date::MakeDay(year, month, 1);
	wd = Date::WeekDay(Date::MakeDate(x, 0));

	mday = (7 - wd + st->wDayOfWeek);
	if (mday >= 7)
	    mday -= 7;
	mday += (st->wDay - 1) * 7 + 1;
	//message(DTEXT("month = %d, wDayOfWeek = %d, wDay = %d, mday = %d\n"), st->wMonth, st->wDayOfWeek, st->wDay, mday);

	day = Date::MakeDay(year, month, mday);
	time = 0;
    }
    n = Date::MakeDate(day,time);
    n = Date::TimeClip(n);
    return n;
}

double Date::time()
{
#if 1
    SYSTEMTIME st;
    double n;

    GetSystemTime(&st);		// get time in UTC
    n = SYSTEMTIME2d_time(&st, 0);
    return n;
#else
    return ::time(NULL) * msPerSecond;
#endif // !1
}

d_time Date::getLocalTZA()
{
    d_time t;
    DWORD r;
    TIME_ZONE_INFORMATION tzi;

    r = GetTimeZoneInformation(&tzi);
    switch (r)
    {
	case TIME_ZONE_ID_STANDARD:
	case TIME_ZONE_ID_DAYLIGHT:
	    //message(DTEXT("bias = %d\n"), tzi.Bias);
	    //message(DTEXT("standardbias = %d\n"), tzi.StandardBias);
	    //message(DTEXT("daylightbias = %d\n"), tzi.DaylightBias);
	    t = -(tzi.Bias + tzi.StandardBias) * (60.0 * 1000.0);
	    break;

	default:
	    t = 0;
	    break;
    }

    return t;
}

/*
 * Get daylight savings time adjust for time dt.
 */

int Date::DaylightSavingTA(d_time dt)
{
    int t;
    DWORD r;
    TIME_ZONE_INFORMATION tzi;
    d_time ts;
    d_time td;

    r = GetTimeZoneInformation(&tzi);
    t = 0;
    switch (r)
    {
	case TIME_ZONE_ID_STANDARD:
	case TIME_ZONE_ID_DAYLIGHT:
	    if (tzi.StandardDate.wMonth == 0 ||
		tzi.DaylightDate.wMonth == 0)
		break;

	    ts = SYSTEMTIME2d_time(&tzi.StandardDate, dt);
	    td = SYSTEMTIME2d_time(&tzi.DaylightDate, dt);

	    if (td <= dt && dt <= ts)
	    {
		t = -tzi.DaylightBias * (60 * 1000);
		//message(DTEXT("DST is in effect, %d\n"), t);
	    }
	    else
	    {
		//message(DTEXT("no DST\n"));
	    }
	    break;
    }
    return t;
}

#endif // _WIN32


