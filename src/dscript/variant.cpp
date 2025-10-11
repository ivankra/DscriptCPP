/* Digital Mars DMDScript source code.
 * Copyright (c) 2000-2002 by Chromium Communications
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

#include <objbase.h>

#include "port.h"
#include "root.h"
#include "dscript.h"
#include "value.h"
#include "date.h"
#include "dobject.h"
#include "text.h"
#include "dcomobject.h"
#include "program.h"

#ifdef linux
#include "varFormats.h"
#endif

#define LOG 0

/************************************************
 * Parse string and convert to a dscript date.
 */

d_number parseDateString(CallContext *cc, d_string s)
{
    OLECHAR *str;
    double date;
    d_number t;
    HRESULT hResult;

    str = d_string_ptr(s);

    // Various test dates to try
    //    str = L"April 12, 1998";
    //    str = L"Tue Apr 02 02:04:57 GMT-0800 1996";
    //    str = L"Apr 02 1996";

    //WPRINTF(L"parseDateString('%s')\n", str);
    //WPRINTF(L"cc = %x, lcid = %x, %x\n", cc, cc->prog->lcid, LOCALE_USER_DEFAULT);

    hResult = VarDateFromStr(str, cc->prog->lcid, 0, &date);
    if (FAILED(hResult))
    {
	//WPRINTF(L"VarDateFromStr() failed with hResult = x%x\n", hResult);
	if (hResult == DISP_E_TYPEMISMATCH)
	{   // Happens if VarDateFromStr() doesn't recognize the string,
	    // which happens with strings like:
	    //    "Tue Apr 02 02:04:57 GMT-0800 1996"
	    // Strangely, jscript successfully parses that string,
	    // so jscript may not be using VarDateFromStr().

	    // Our date parser can handle it, so try it next.
	    t = Date::parse(str);
	}
	else
	    t = Port::nan;
    }
    else
	t = Date::VarDateToTime(date);
    return t;
}


/***********************************************
 * Convert dscript date to string.
 * Returns NULL on failure.
 */

d_string dateToString(CallContext *cc, d_number t, enum TIMEFORMAT tf)
{
    double date;
    BSTR bstr;
    HRESULT hResult;
    d_string s;
    unsigned long dwFlags;
    dchar *p;
    LCID lcid;

    lcid = cc->prog->lcid;
#if LOG
    WPRINTF(L"dateToString: cc = %x, lcid = %x, %x\n", cc, cc->prog->lcid, LOCALE_USER_DEFAULT);
#endif
    date = Date::toVarDate(t);
#ifdef linux
    // Use Chcom supplied function
    VARIANT var;
    int iNamedFormat;

    dwFlags = 0;
    switch (tf)
    {
	case TFString:
	    lcid = LOCALE_USER_DEFAULT;
	    iNamedFormat = 0;
	    break;
	case TFDateString:
	    lcid = LOCALE_USER_DEFAULT;
	    iNamedFormat = 1;
	    break;
	case TFTimeString:
	    lcid = LOCALE_USER_DEFAULT;
	    iNamedFormat = 3;
	    break;
	case TFLocaleString:
	    iNamedFormat = 0;
	    break;
	case TFLocaleDateString:
	    iNamedFormat = 1;
	    break;
	case TFLocaleTimeString:
	    iNamedFormat = 3;
	    break;

	default:
	    assert(0);
	    iNamedFormat = 0;
	    break;
    }
    VariantInit(&var);
    V_VT(&var) = VT_DATE;
    V_DATE(&var) = date;

    hResult = _VAR_FormatDateTime(lcid, &var, iNamedFormat, dwFlags, &bstr);
    VariantClear(&var);
#else
    switch (tf)
    {
	case TFString:
	    lcid = LOCALE_USER_DEFAULT;
	    dwFlags = 0;
	    break;
	case TFDateString:
	    lcid = LOCALE_USER_DEFAULT;
	    dwFlags = VAR_DATEVALUEONLY;
	    break;
	case TFTimeString:
	    lcid = LOCALE_USER_DEFAULT;
	    dwFlags = VAR_TIMEVALUEONLY;
	    break;
	case TFLocaleString:
	    dwFlags = 0;
	    break;
	case TFLocaleDateString:
	    dwFlags = VAR_DATEVALUEONLY;
	    break;
	case TFLocaleTimeString:
	    dwFlags = VAR_TIMEVALUEONLY;
	    break;

	default:
	    dwFlags = 0;		// suppress warnings
	    assert(0);
    }
#if LOG
    WPRINTF(L"+VarBstrFromDate(date = %g, lcid = %d)\n", date, lcid);
#endif
    hResult = VarBstrFromDate(date, lcid, dwFlags, &bstr);
#if LOG
    WPRINTF(L"-VarBstrFromDate(), hResult = x%x\n", hResult);
#endif
#endif
    p = bstr;
    if (FAILED(hResult))
    {
#if LOG
	WPRINTF(L"VarBstrFromDate() failed with hResult = x%x\n", hResult);
#endif
	p = L"nan";
    }
    else if (!p)
    {
	p = L"";
    }
#if LOG
    WPRINTF(L"VarBstrFromDate(), returns '%s'\n", p);
#endif


    // Convert BSTR to d_string
    Mem mem;
    GC_LOG();
    s = Dstring::dup(&mem, p);
    return s;
}


/********************************
 */

int localeCompare(CallContext *cc, d_string s1, d_string s2)
{
    int result = 0;

#if LOG
    WPRINTF(L"+localeCompare(cc=%x, lcid=%d, s1='%s', s2 = '%s')\n", cc, cc->prog->lcid, d_string_ptr(s1), d_string_ptr(s2));
#endif

#if 1
    result = CompareString(cc->prog->lcid, 0, d_string_ptr(s1), d_string_len(s1),
	d_string_ptr(s2), d_string_len(s2));
    switch (result)
    {
	case CSTR_LESS_THAN:
		result = -1;
		break;
	case CSTR_EQUAL:
		result = 0;
		break;
	case CSTR_GREATER_THAN:
		result = 1;
		break;
	default:
#if LOG
		WPRINTF(L"unknown CSTR %d\n", result);
#endif
		break;
    }
#else
    // VarBstrCmp() not implemented by Chcom
    BSTR b1;
    BSTR b2;
    HRESULT hResult;

    b1 = SysAllocStringLen(d_string_ptr(s1), d_string_len(s1));
    b2 = SysAllocStringLen(d_string_ptr(s2), d_string_len(s2));
    if (b1 && b2)
    {
#if LOG
	WPRINTF(L"+VarBstrCmp()\n");
#endif
#ifdef linux
	hResult = VarBstrCmp(b1, b2, cc->prog->lcid);
#else
	hResult = VarBstrCmp(b1, b2, cc->prog->lcid, 0);
#endif
#if LOG
	WPRINTF(L"-VarBstrCmp(), hResult = %x\n", hResult);
#endif

	switch (hResult)
	{
	    case VARCMP_LT:	result = -1;	break;
	    case VARCMP_GT:	result = 1;	break;

	    // result is 0 for all these
	    //case VARCMP_EQ:
	    //case VARCMP_NULL:
	    //default:
	}
    }

    SysFreeString(b1);
    SysFreeString(b2);
#endif

#if LOG
    WPRINTF(L"-localeCompare() = %d\n", result);
#endif

    return result;
}

/* ======================= Dvariant ======================= */

Dvariant::Dvariant(Dobject *prototype, VARIANT *v)
    : Dobject(prototype ? prototype : Dobject::getPrototype())
{
    //WPRINTF(L"Dvariant::Dvariant(VARIANT %p)\n", v);
    classname = TEXT_unknown;
    isLambda = 3;
    VariantInit(&variant);

    if (v)
    {
	HRESULT hResult;

	hResult = VariantCopy(&variant, v);
	if (FAILED(hResult))
	{
	    //WPRINTF(L"VariantCopy failed, hResult = %x\n", hResult);
	}
    }
}

d_string Dvariant::getTypeof()
{
    return TEXT_unknown;
}

void * Dvariant::operator new(size_t m_size)
{
    void *p = Dobject::operator new(m_size);

    if (p)
        mem.setFinalizer(p, DoFinalize, NULL);
    return p;
}

void Dvariant::finalizer()
{
    //WPRINTF(L"Dvariant::finalizer(mEnum = %x)\n", mEnum);
    VariantClear(&variant);
}

