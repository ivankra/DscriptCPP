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


#include "dchar.h"
#include "port.h"
#include "printf.h"

// A simple class to time execution of a block of code

struct PerfTimer
{
    dchar *msg;
    ulonglong starttime;

    PerfTimer(dchar *msg)
    {
	this->msg = msg;
	starttime = Port::read_timer();
    }

    ~PerfTimer()
    {
	ulonglong elapsedtime = Port::read_timer() - starttime;
	WPRINTF(L"%s %d\n", msg, (int)elapsedtime);
    }
};
