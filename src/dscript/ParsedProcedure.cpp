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

#include <windows.h>
#include <assert.h>

#include <objbase.h>
#include <dispex.h>

#include "dscript.h"
#include "root.h"
#include "expression.h"
#include "statement.h"
#include "identifier.h"
#include "symbol.h"
#include "scope.h"
#include "ir.h"
#include "value.h"
#include "property.h"
#include "dobject.h"
#include "program.h"
#include "text.h"
#include "mem.h"

#if defined __DMC__
#include "dmc_rpcndr.h"
#endif

#include "ParsedProcedure.h"
#include "OLEScript.h"

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// CParsedProcedure()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
CParsedProcedure::CParsedProcedure()
  : m_pEngine(NULL),
    m_pfdCode(NULL),
    m_pnid(NULL),
    m_dwFlags(0),
    m_dwSourceContext(0)
{
    //WPRINTF(L"CParsedProcedure()\n");
#if INVARIANT
    signature = CPARSEDPROCEDURE_SIGNATURE;
#endif
} // CParsedProcedure


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// ~CParsedProcedure()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
CParsedProcedure::~CParsedProcedure()
{
    invariant();
    m_pEngine = NULL;
    m_pfdCode = NULL;
    m_pnid = NULL;
    m_dwSourceContext = 0;
#if INVARIANT
    signature = 0;
#endif
} // ~CParsedProcedure


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Initialize()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT CParsedProcedure::Initialize(class COleScript* pEngine, FunctionDefinition* pfd,
                          struct SNamedItemData* pnid, DWORD dwFlags, DWORD dwSourceContext)
{
    invariant();
    assert(NULL != pEngine && NULL != pfd);

    m_pEngine = pEngine;
    m_pfdCode = pfd;
    m_pnid = pnid;
    m_dwFlags = dwFlags;
    m_dwSourceContext = dwSourceContext;

    return S_OK;
} // Initialize


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _DoGetDispID()
//
// We don't support mapping names to DISPID's on parsed procedures because
// there is no name -- just an anonymous function.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT CParsedProcedure::_DoGetDispID(BSTR bstrName /*prgszNames*/,
                                       DWORD /*grfdex*/, DISPID* /*pID*/)
{
    invariant();
    //
    // Whatever it is, it's unknown to us...we don't bother with this method.
    //
    return DISP_E_UNKNOWNNAME;
} // _DoGetIDsOfNames


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _DoInvoke()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT CParsedProcedure::_DoInvoke(DISPID dispidMember, REFIID riid, LCID lcid,
                                    WORD wFlags, DISPPARAMS* pDispParams,
                                    VARIANT* pvarResult, EXCEPINFO* pExcepInfo,
                                    UINT* puArgErr)
{
    HRESULT hr = E_UNEXPECTED;
    //FuncLog funclog(L"CParsedProcedure::_DoInvoke()");

    invariant();
    //
    // We ONLY support method invocations on a parsed procedure
    //
    if ((wFlags & DISPATCH_METHOD) != DISPATCH_METHOD)
        return E_INVALIDARG;

    if (DISPID_VALUE != dispidMember)
        return DISP_E_MEMBERNOTFOUND;

    // This code is kind of a hybrid of CNamedItemDisp::_DoInvoke()
    // and ExecuteParsedScriptBlock().
    // Combining the two might be an improvement.

    Dobject*    poThis = NULL;

    //
    // Since this is a code fragment, there can only be zero or one parameters. If there's
    // none, we execute at global scope in the given named item. Otherwise if there's one,
    // we execute in the "this" context of the DISPID_THIS named parameter (if there's a
    // param, it MUST be a DISPID_THIS).
    //
    if (pDispParams->cArgs != pDispParams->cNamedArgs || pDispParams->cArgs > 1)
        return DISP_E_BADPARAMCOUNT;

    if (pDispParams->cNamedArgs == 1 && pDispParams->rgdispidNamedArgs[0] != DISPID_THIS)
    {
        if (puArgErr)
            *puArgErr = 0;              // arg 0 contains the error
        return DISP_E_PARAMNOTFOUND;
    }

    // Get the 'othis' pointer from the named arguments
    switch (pDispParams->cNamedArgs)
    {
        case 0:
            poThis = NULL;
            break;

        case 1:
        {   // Can happen when SCRIPTPROC_IMPLICIT_THIS

            Value   ovalue;
            Value*  pvalTemp = NULL;

            pvalTemp = &ovalue;                 // so the toObject() call is polymorphic
            hr = VariantToValue(pvalTemp, &pDispParams->rgvarg[0], d_string_ctor(L""), NULL);
            if (FAILED(hr))
                return hr;

            poThis = pvalTemp->toObject();      // make sure toObject() is a polymorphic call
            break;
        }
        default:
            //
            // ASSERT: Previous param checking should make it impossible to hit
            //         this case...
            //
            assert(false);
            return DISP_E_BADPARAMCOUNT;
    }

    ErrInfo errinfo;

    int dResult = m_pEngine->ExecuteParsedScript(m_pfdCode, poThis, m_pnid,
                                                 m_dwFlags, pvarResult, &errinfo);

    if (0 != dResult)
    {
        DPF(L"COM: ERROR CParsedProcedure::_DoInvoke() : parse error : \"%s\"\n", errinfo.message);
        return m_pEngine->HandleScriptError(&errinfo, pExcepInfo, m_dwSourceContext);
    }

    return S_OK;
}
