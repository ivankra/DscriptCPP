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


#ifndef DATEPARSE_H
#define DATEPARSE_H

#include "dchar.h"

struct DateParse
{
    int year;
    int month;
    int day;
    int hours;
    int minutes;
    int seconds;
    int ms;
    int weekday;
    int ampm;
#define TZCORRECTION_NAN	-30000
    int tzcorrection;

    int parse(dchar *s);

private:
    dchar *s;
    dchar *p;
    int number;
    char *buffer;	// not dchar

    int nextToken();
    int classify();
    int parseString(dchar *s);
    int parseCalendarDate(int n1);
    int parseTimeOfDay(int n1);
};

#endif
