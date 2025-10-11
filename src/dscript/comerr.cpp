
/* Digital Mars DMDScript source code.
 * Copyright (c) 2002 by Chromium Communications
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


/*
 * Converts COM error message numbers to dscript error message numbers
 */

#include <windows.h>

#if defined __DMC__
#include "dmc_rpcndr.h"
#endif

#include <objbase.h>
#include <activscp.h>
#include <dispex.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>

#include "port.h"
#include "root.h"
#include "dscript.h"
#include "text.h"

struct Tmsg
{
    HRESULT hResult;	// HRESULT to look up
    int msgnum;		// corresponding message number
};

static Tmsg TransTable[] =
{
    // Add new values here and add text to textgen.c
    { E_UNEXPECTED, ERR_E_UNEXPECTED },
};


/*************************************
 * Input:
 *	hResult		the HRESULT we got
 *	msgnum		default message number if HRESULT is not found
 */

int hresultToMsgnum(HRESULT hResult, int msgnum)
{
    unsigned i;

    for (i = 0; i < sizeof(TransTable) / sizeof(TransTable[0]); i++)
    {
	if (TransTable[i].hResult == hResult)
	{   msgnum = TransTable[i].msgnum;
	    break;
	}
    }
    return msgnum;
}
