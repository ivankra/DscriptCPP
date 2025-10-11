/* Digital Mars DMDScript source code.
 * Copyright (c) 2000-2001 by Chromium Communications
 * Copyright (c) 2004-2007 by Digital Mars
 * All Rights Reserved.
 * written by Paul R. Nash
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
 * COM objects are represented to dscript as Dcomobject's.
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

#include <math.h>
#undef exception

#include "port.h"
#include "root.h"
#include "dscript.h"
#include "value.h"
#include "date.h"
#include "dchar.h"
#include "dobject.h"
#include "mem.h"
#include "dcomobject.h"
#include "dcomlambda.h"
#include "text.h"
#include "program.h"
#include "BaseDispatch.h"

#undef LOG
#define LOG 0       // 0: disable logging, 1: enable it

void DoFinalize(void* pObj, void* pClientData);

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Dcomlambda()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
Dcomlambda::Dcomlambda(DISPID id, IDispatch* pDisp,
                       IDispatchEx* pDispEx, Dcomobject* pParent)
    : Dobject(Dobject::getPrototype())
{
    //WPRINTF(L"Dcomlambda(%x)\n", this);
    isLambda = 1;
    m_idMember = id;
    m_pDisp = pDisp;
    m_pDispEx = pDispEx;
    m_pParent = pParent;

    if (NULL != m_pDisp)
        m_pDisp->AddRef();
    if (NULL != m_pDispEx)
        m_pDispEx->AddRef();
} // Dcomlambda


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// ~Dcomlambda()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
Dcomlambda::~Dcomlambda()
{
#if LOG
    WPRINTF(L"~Dcomlambda(%p)\n", this);
#endif
    m_idMember = DISPID_UNKNOWN;
    finalizer();
    m_pParent = NULL;
}


void * Dcomlambda::operator new(size_t m_size)
{
    void *p = Dobject::operator new(m_size);

    if (p)
        mem.setFinalizer(p, DoFinalize, NULL);
    return p;
}

void Dcomlambda::finalizer()
{
    if (m_pDispEx) {
	m_pDispEx->Release();
        m_pDispEx= NULL;
    }
    if (m_pDisp) {
	m_pDisp->Release();
        m_pDisp = NULL;
    }
}


/*************************************************
 */

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Call()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void *Dcomlambda::Call(CALL_ARGS)
{
    HRESULT     hResult;
    DISPPARAMS  dp;
    VARIANTARG* pvarg = 0;
    VARIANT     vresult;
    EXCEPINFO   ei;
    unsigned    argerr = 0;
    unsigned    counter;
    int         dothis;
    DISPID      idThis;

    Value::copy(ret, &vundefined);

    assert(NULL != m_pParent);

    // Support for 'this' parameter is only in IDispatchEx
    dothis = (othis && m_pDispEx != NULL) ? 1 : 0;

    // Allocate for the variants
    if (0 != (dothis + argc))
    {	int ac = dothis + argc;

        if (ac >= 32 || (pvarg = (VARIANTARG *)alloca(sizeof(VARIANTARG) * ac)) == NULL)
        {
	    // Not enough stack space - use mem
	    pvarg = (VARIANTARG *)mem.malloc(sizeof(VARIANTARG) * ac);
        }
    }

    // Set up the dispparams
    // When DISPATCH_METHOD is set in wFlags, there may be a "named
    // parameter" for the "this" value. The dispID will be DISPID_THIS and
    // it must be the first named parameter.
    // For how the DISPPARAMS is set up, see:
    //	http://msdn.microsoft.com/library/books/inole/S1227.htm

    dp.rgdispidNamedArgs = NULL;
    dp.cNamedArgs = dothis;
    dp.cArgs = dp.cNamedArgs + argc;
    dp.rgvarg = pvarg;

    if (dothis)
    {
        idThis = DISPID_THIS;
        dp.rgdispidNamedArgs = &idThis;
        ValueToVariant(&pvarg[0], &othis->value);   // first named parameter
    }

    // clear out excepinfo
    memset(&ei, 0, sizeof(ei));

    // Loop through the argument list and build up the variant array
    // Note that arguments are stored right to left in pvarg[]
    // following the named parameters.
    for (counter = 0; counter < argc; counter++)
    {
        // Mutate dscript types to variant types
        ValueToVariant(&(pvarg[dothis + argc - counter - 1]),
                       &(arglist[counter]));
    }

    // Do the call
    VariantInit(&vresult);

#if LOG
    WPRINTF(DTEXT("Dcomlambda::Call(): Invoke()\n"));
#endif
    if (NULL != m_pDispEx)
    {
        hResult = m_pDispEx->InvokeEx(m_idMember,
                                      LCID_ENGLISH_US,
                                      DISPATCH_METHOD | DISPATCH_PROPERTYGET,
                                      &dp,
                                      &vresult,
                                      &ei,
                                      NULL);

        // It may not be necessary to pass
        // DISPID_THIS to the thing we're calling. If so, all this
        // code gets simpler.
        //
        // Some things don't like the DISPID_THIS, so if we get a bad param
        // count error, just try again without a this pointer...
        //
        if (DISP_E_BADPARAMCOUNT == hResult)
        {
            VariantClear(&vresult);
            SysFreeString(ei.bstrSource);
            SysFreeString(ei.bstrDescription);
            SysFreeString(ei.bstrHelpFile);

            //
            // Reset the DISPPARAMS and don't pass a DISPID_THIS
            //
            dp.rgdispidNamedArgs = NULL;
            dp.cNamedArgs = 0;
            dp.cArgs = argc;
            dp.rgvarg = dothis ? &pvarg[1] : &pvarg[0];

            hResult = m_pDispEx->InvokeEx(m_idMember,
                                          LCID_ENGLISH_US,
                                          DISPATCH_METHOD | DISPATCH_PROPERTYGET,
                                          &dp,
                                          &vresult,
                                          &ei,
                                          NULL);
        }
    }
    else
    {
        // The reason to set DISPATCH_PROPERTYGET is because of a behavior
        // exhibited by some applications like Excel:
        //
        //   http://support.microsoft.com/support/kb/articles/Q165/2/73.ASP
        //
        // For more info:
        //   http://msdn.microsoft.com/library/default.asp?URL=/library/psdk/automat/chap5_8gfn.htm
        //
        // Basically, there can be either methods or propgets that return an object. Which
        // one gets used is entirely up to the implementor of the Automation object in question.
        //
        // Thanks to Paul for figuring this out.

        assert(NULL != m_pDisp);
        hResult = m_pDisp->Invoke(m_idMember,
                                  IID_NULL,
                                  LCID_ENGLISH_US,
                                  DISPATCH_METHOD | DISPATCH_PROPERTYGET,
                                  &dp,
                                  &vresult,
                                  &ei,
                                  &argerr);
    }
#if LOG
    WPRINTF(DTEXT("Dcomlambda::Call(): Invoke() returned %x\n"), hResult);
#endif

    if (NULL != pvarg)
    {
        for (counter = 0; counter < dp.cArgs; counter++)
        {
            VariantClear(&pvarg[counter]);
        }
    }

    if (FAILED(hResult))
    {	ErrInfo errinfo;

        return RuntimeError(&errinfo, ERR_OBJECT_NO_CALL);
    }

    // Convert return type to return value
    VariantToValue(ret, &vresult, d_string_ctor(L""), NULL);

    // Cleanup for the variant return type
    VariantClear(&vresult);

    return NULL;
} // Call

/****************************************************
 * This is the implementation of:
 *	Session.Contents("CorrectStarted") = "true";
 */

Value *Dcomlambda::put_Value(Value *ret, unsigned argc, Value *arglist)
{
    HRESULT hResult;
    DISPID dispid = DISPID_VALUE;
    DISPPARAMS dp;
    VARIANTARG *pvarg = 0;
    VARIANT vresult;
    EXCEPINFO ei;
    unsigned argerr = 0;
    unsigned counter;
    int dothis;

    if (LOG) WPRINTF(DTEXT("Dcomlambda::put_Value()\n"));

    Value::copy(ret, &vundefined);

    if (NULL == m_pDisp && NULL == m_pDispEx)
    {
        if (LOG) WPRINTF(DTEXT("Call : Unable to find something to call.\n"));
        return NULL;
    }

    dothis = 0;

    // clear out excepinfo
    memset(&ei, 0, sizeof(ei));

    // Allocate for the variants
    if (0 != (dothis + argc))
    {	int ac = dothis + argc;

        if (ac >= 32 || (pvarg = (VARIANTARG *)alloca(sizeof(VARIANTARG) * ac)) == NULL)
        {
	    // Not enough stack space - use mem
	    GC_LOG();
	    pvarg = (VARIANTARG *)mem.malloc(sizeof(VARIANTARG) * ac);
        }
	if (LOG) WPRINTF(L"pvarg = %x, dothis = %d, argc = %d\n", pvarg, dothis, argc);
    }

    DISPID dispidPut = DISPID_PROPERTYPUT;
    dp.rgdispidNamedArgs = &dispidPut;
    dp.cNamedArgs = 1;
    dp.cArgs = argc;
    dp.rgvarg = pvarg;

    // Loop through the argument list and build up the variant array
    // Note that arguments are stored right to left in pvarg[]
    // following the named parameters.
    for (counter = 0; counter < argc; counter++)
    {
        // Mutate dscript types to variant types
	ValueToVariant(&(pvarg[dothis + argc - counter - 1]),&(arglist[counter]));
	//ValueToVariant(&(pvarg[dothis + counter]),&(arglist[counter]));
    }

    // Do the call
    VariantInit(&vresult);

    WORD    wFlags = DISPATCH_PROPERTYPUT;
    if (argc && V_VT(&pvarg[dothis + 0]) == VT_DISPATCH)
        wFlags |= DISPATCH_PROPERTYPUTREF;

    if (LOG) WPRINTF(DTEXT("Dcomobject::put_Value(): Invoke()\n"));
    dispid = m_idMember;
    if (m_pDispEx)
    {
        hResult = m_pDispEx->InvokeEx(dispid,
                                    LCID_ENGLISH_US,
                                    wFlags,
                                    &dp,
                                    &vresult,
                                    &ei,
                                    NULL);
    }
    else
    {
        hResult = m_pDisp->Invoke(dispid,
                                    IID_NULL,
                                    LCID_ENGLISH_US,
                                    wFlags,
                                    &dp,
                                    &vresult,
                                    &ei,
                                    &argerr);
    }
    if (LOG) WPRINTF(DTEXT("Dcomobject::put_Value(): Invoke() returned %x\n"), hResult);

    if (DISP_E_BADVARTYPE == hResult)
    {
	if (LOG)
	{
	    WPRINTF(DTEXT("Bad Variant Type returned for Invoke\n"));
	    WPRINTF(DTEXT("dp.cArgs %d\n"), dp.cArgs);
	    for (unsigned int kk = 0; kk < dp.cArgs; kk++)
		WPRINTF(DTEXT("    dp.rgvarg[%d].vt = %d\n"), kk, dp.rgvarg[kk].vt);
	}
    }

    if (pvarg)
    {
        for (counter = 0; counter < argc + dothis; counter++)
        {
            VariantClear(&pvarg[counter]);
        }
    }

    if (FAILED(hResult))
    {
        Value* pvalErr = NULL;
	ErrInfo errinfo;

        if (DISP_E_EXCEPTION == hResult)
        {
            pvalErr = RuntimeError(&errinfo, ERR_S_S,
                                   ei.bstrSource ? ei.bstrSource : L"",
                                   ei.bstrDescription ? ei.bstrDescription : L"");
        }
        else
        {
	    pvalErr = RuntimeError(&errinfo, ERR_COM_FUNCTION_ERROR,
		    DTEXT("put_Value()"), hResult);
        }

        SysFreeString(ei.bstrSource);
        SysFreeString(ei.bstrDescription);
        SysFreeString(ei.bstrHelpFile);
        memset(&ei, 0, sizeof(ei));

        return pvalErr;
    }

    // Convert return type to return value
    VariantToValue(ret, &vresult, d_string_ctor(L""), NULL);

    // Cleanup for the variant return type
    VariantClear(&vresult);

    return NULL;
} // put_Value()



