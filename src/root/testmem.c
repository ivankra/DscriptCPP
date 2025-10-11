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

#include "mem.h"
#include "root.h"
#include "gc.h"

int main()
{
    void *p;

    mem.init();
    p = new Object();
    p = mem.malloc(100);
    p = mem.malloc(75);
    mem.fullcollect();
    wprintf(L"Success - wchar\n");
    printf("Success - char\n");
    return EXIT_SUCCESS;
}
