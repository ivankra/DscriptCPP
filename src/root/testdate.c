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
#include <assert.h>
#include <time.h>

#include "root.h"
#include "date.h"

int main()
{
    Date::init();

    d_time t;

    t = ::time(NULL) * 1000.0;
    printf("t = %g\n", t);
#if defined UNICODE
    wprintf(L"t = %d (%g), '%ls'\n", (long)(t/1000), t, Date::ToString(t));
#else
    printf("t = %d (%g), '%s'\n", (long)(t/1000), t, Date::ToString(t));
#endif

    t = Date::time();
    printf("t = %g\n", t);
#if defined UNICODE
    wprintf(L"t = %d (%g), '%ls'\n", (long)(t/1000), t, Date::ToString(t));
#else
    printf("t = %d (%g), '%s'\n", (long)(t/1000), t, Date::ToString(t));
#endif

    time_t ct;
    ct = ::time(NULL);
    printf("ctime() = '%s'\n", ctime(&ct));

    return 0;
}
