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

#ifndef DATE_H
#define DATE_H

#include "dchar.h"

typedef double d_time;
#define DATE_BUFSIZE (29 + 7 + 1)

struct Date
{
    static char daystr[];
    static char monstr[];

    static double toVarDate(d_time n);
    static d_time VarDateToTime(double d);

    static d_time dmod(d_time n, d_time d);
    static d_time HourFromTime(d_time t);
    static d_time MinFromTime(d_time t);
    static d_time SecFromTime(d_time t);
    static d_time msFromTime(d_time t);
    static d_time TimeWithinDay(d_time t);
    static d_time toInteger(d_time n);
    static int Day(d_time t);
    static int LeapYear(int y);
    static int DaysInYear(int y);
    static int DayFromYear(int y);
    static d_time TimeFromYear(int y);
    static int YearFromTime(d_time t);
    static int inLeapYear(d_time t);
    static int MonthFromTime(d_time t);
    static int DateFromTime(d_time t);
    static int WeekDay(d_time t);
    static d_time LocalTime(d_time t);
    static d_time UTC(d_time t);
    static d_time MakeTime(d_time hour, d_time min, d_time sec, d_time ms);
    static d_time MakeDay(d_time year, d_time month, d_time date);
    static d_time MakeDate(d_time day, d_time time);
    static d_time TimeClip(d_time time);
    static dchar *ToString(d_time time);
    static dchar *ToDateString(d_time time);
    static dchar *ToTimeString(d_time time);
    static dchar *ToLocaleString(d_time time);
    static dchar *ToLocaleDateString(d_time time);
    static dchar *ToLocaleTimeString(d_time time);
    static dchar *ToUTCString(d_time time);
    static d_time parse(dchar *s);

    static d_time time();
    static d_time getLocalTZA();
    static int DaylightSavingTA(d_time);

    static void init();
};

#endif
