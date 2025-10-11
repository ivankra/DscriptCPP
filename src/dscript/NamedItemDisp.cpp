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
#include <malloc.h>

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

#include "OLEScript.h"
#include "NamedItemDisp.h"
#include "dcomobject.h"

#define LOG 0

//
// The starting value for DISPID's we'll allocate is a fairly arbitrary number -- it must
// be positive and we pick it to be far from 0 so the client is likely to accidentally
// succeed if they pass in small numbers that are uninitialized variables and such.
//
const DISPID CNamedItemDisp::DISPID_BASE = 16384;


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// CNamedItemDisp()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
CNamedItemDisp::CNamedItemDisp()
  : m_pEngine(NULL),
    m_pcc(NULL),
    m_fGlobal(false),
    m_dwSourceContext(0)
{
#if LOG
    WPRINTF(L"CNamedItemDisp()\n");
#endif
#if INVARIANT
    signature = CNAMEDITEMDISP_SIGNATURE;
#endif
} // CNamedItemDisp


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// ~CNamedItemDisp()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
CNamedItemDisp::~CNamedItemDisp()
{
#if LOG
    WPRINTF(L"~CNamedItemDisp()\n");
#endif
    invariant();
    m_pcc = NULL;
    m_pEngine = NULL;
    m_fGlobal = false;
    m_dwSourceContext = 0;
#if INVARIANT
    signature = 0;
#endif
} // ~CNamedItemDisp


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Initialize()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT CNamedItemDisp::Initialize(class COleScript* pEngine, CallContext* pcc,
	bool fGlobal, DWORD dwSourceContext)
{
#if LOG
    FuncLog(L"CNamedItemDisp::Initialize()");
#endif
    invariant();
    mem.setStackBottom((void *)&pcc);
    if (NULL == pEngine || NULL == pcc)
        return E_FAIL;

    m_pEngine = pEngine;
    m_pcc = pcc;
    m_fGlobal = fGlobal;
    m_dwSourceContext = dwSourceContext;

    return S_OK;
} // Initialize


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _DoGetDispID()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT CNamedItemDisp::_DoGetDispID(BSTR bszName, DWORD grfdex, DISPID* pID)
{
#if LOG
    FuncLog(L"CNamedItemDisp::_DoGetDispID()");
#endif
    invariant();
    mem.setStackBottom((void *)&pID);
    HRESULT hResult;

    //WPRINTF(L"CNamedItemDisp::_DoGetDispID(bszName = '%s', grfdex = x%x)\n", bszName, grfdex);
    Dobject*    pObj = NULL;
    Value*      pVal = NULL;
    int         idxContext = m_pcc->scoperoot;
    bool        fDidFirstScope = false;
    d_string    pdstrName = d_string_ctor(bszName);
    unsigned    uHash = d_string_hash(pdstrName);

    CallContext*    pccSave = m_pEngine->SwitchGlobalProgramContext(m_pcc);
    Program*        pprogSave = m_pEngine->SwitchToOurGlobalProgram();

    //
    // If we're in the global module, then look up the name in all the
    // scopes on the CallContext scope stack starting at the rightmost global
    // scope and proceeding leftwards. If we're not in the global module,
    // then we should only search the rightmost scope (fDidFirstScope controls
    // this and allows exactly one iteration in that case).
    //

    hResult = DISP_E_UNKNOWNNAME;
    while ((m_fGlobal || !fDidFirstScope) && (idxContext > 0))
    {
        fDidFirstScope = true;
        pObj = (Dobject*)m_pcc->scope->data[idxContext-1];
        pVal = pObj->Dobject::Get(pdstrName, uHash);
    
        if (NULL == pVal)
        {
            *pID = DISPID_UNKNOWN;
        }
        else
        {
            //
            // Get a DISPID that maps to the Value. Either we have one already, or
            // we need to add one to the mapping table and return it.
            //
            *pID = _NameToDISPID(pdstrName);
            if (0 == *pID)
                *pID = _AddIDName(pdstrName);
	    hResult = S_OK;
	    //WPRINTF(L"found DISPID = x%x\n", *pID);
	    break;
        }

        idxContext--;
    }

    (void)m_pEngine->SetGlobalProgramContext(pccSave);
    m_pEngine->RestoreSavedGlobalProgram(pprogSave);

    return hResult;
} // _DoGetDispID


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _DoInvoke()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT CNamedItemDisp::_DoInvoke(DISPID dispidMember, REFIID riid, LCID lcid,
                                  WORD wFlags, DISPPARAMS* pDispParams,
                                  VARIANT* pvarResult, EXCEPINFO* pExcepInfo,
                                  UINT* puArgErr)
{
#if LOG
    FuncLog(L"CNamedItemDisp::_DoInvoke()");
#endif
    ENTRYLOG(L"COMNID: ENTER _DoInvoke(%i, 0x%04lx)\n", dispidMember, wFlags);
    invariant();
    HRESULT hRet = E_UNEXPECTED;

    //
    // See if we recognize the DISPID as something we handed out previously...
    //
    d_string    pdstrName = _DISPIDToName(dispidMember);

    if (NULL == pdstrName)
    {
        DPF(L"COMNID: ERROR _DoInvoke() : entry not found for %i\n", dispidMember);
        return DISP_E_MEMBERNOTFOUND;
    }

    //
    // Attempt to process as PROPERTYGET. In failure from PROPERTYGET (such as
    // if the name is not found), if the DISPATCH_METHOD flag is also set, we
    // should try to call as a method.
    //
    if (wFlags & DISPATCH_PROPERTYGET)
    {
        DPF(L"COMNID: _DoInvoke() op is PropertyGet\n");
        if (NULL == dispidMember)
	{
            return DISP_E_MEMBERNOTFOUND;
	}
        HRESULT hrTest = _OnPropertyGet(dispidMember, pdstrName, wFlags, pDispParams, pvarResult, pExcepInfo, puArgErr);

        if (FAILED(hrTest) && (wFlags & (DISPATCH_METHOD | DISPATCH_CONSTRUCT)))
        {
            hRet = _OnMethodCall(dispidMember, pdstrName, wFlags, pDispParams, pvarResult, pExcepInfo, puArgErr);
        }
        else
        {
            hRet = hrTest;
        }
    }
    else if (wFlags & DISPATCH_PROPERTYPUT)
    {
        DPF(L"COMNID: _DoInvoke() op is PropertyPut\n");
        hRet = _OnPropertyPut(dispidMember, pdstrName, wFlags, pDispParams, pvarResult, pExcepInfo, puArgErr);
    }
    else if (wFlags & (DISPATCH_METHOD | DISPATCH_CONSTRUCT))
    {
        DPF(L"COMNID: _DoInvoke() op is Method or Construct\n");
        hRet = _OnMethodCall(dispidMember, pdstrName, wFlags, pDispParams, pvarResult, pExcepInfo, puArgErr);
    }
    else
    {
        //
        // We already checked the wFlags for a valid flag, so one of the above
        // cases should have been taken and we should not have arrived here.
        //
        assert(false);
    }

    ENTRYLOG(L"COMNID: EXIT _DoInvoke() (hr = 0x%08lx)\n", hRet);
    assert(E_UNEXPECTED != hRet);
    return hRet;
} // _DoInvoke


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _LocateNameInScope()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT CNamedItemDisp::_LocateNameInScope(d_string pdstrName, Dobject** ppObj, Value** ppVal)
{
#if LOG
    FuncLog(L"CNamedItemDisp::_LocateNameInScope()");
#endif
    invariant();
    assert(NULL != ppObj);
    assert(NULL != ppVal);

    assert(NULL != m_pcc && NULL != m_pcc->scope);

    Dobject*    pObj = NULL;
    Value*      pVal = NULL;
    int         idxContext = m_pcc->scoperoot;
    bool        fFound = false;
    bool        fDidFirstScope = false;
    unsigned    uHash = d_string_hash(pdstrName);

    //
    // If we're in the global module, then look up the name in all the the
    // scopes on the CallContext scope stack starting at the rightmost global
    // scope and proceeding leftwards. If we're not in the global module,
    // then we should only search the rightmost scope (fDidFirstScope controls
    // this and allows exactly one iteration in that case).
    //
    while (!fFound && ((m_fGlobal || !fDidFirstScope) && (idxContext > 0)))
    {
        fDidFirstScope = true;
        pObj = (Dobject*)m_pcc->scope->data[idxContext-1];
        pVal = pObj->Dobject::Get(pdstrName, uHash);

        //
        // Find the rightmost object in the scope chain with the name
        //
        if (NULL != pVal)
            fFound = true;

        idxContext--;
    }

    if (!fFound)
        return DISP_E_MEMBERNOTFOUND;

    *ppObj = pObj;
    *ppVal = pVal;
    return S_OK;
} // _LocateNameInScope


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _OnPropertyGet()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT CNamedItemDisp::_OnPropertyGet(DISPID dispidMember, d_string pdstrName,
                                       WORD wFlags, DISPPARAMS* pDispParams,
                                       VARIANT* pvarResult,
                                       EXCEPINFO* pExcepInfo,
                                       UINT* puArgErr)
{
#if LOG
    FuncLog(L"CNamedItemDisp::_OnPropertyGet()");
#endif
    UNREFERENCED_PARAMETER(puArgErr);   // Only used with DISP_E_TYPEMISMATCH or DISP_E_PARAMNOTFOUND...

    UNREFERENCED_PARAMETER(dispidMember);
    UNREFERENCED_PARAMETER(pExcepInfo);
    invariant();

    //
    // In a namespace we only support parameter-less prop get's
    //
    if (pDispParams->cArgs != 0 || pDispParams->cNamedArgs != 0)
        return E_INVALIDARG;

    Dobject*    pObj = NULL;
    Value*      pVal = NULL;

    CallContext*    pccSave = m_pEngine->SwitchGlobalProgramContext(m_pcc);
    Program*        pprogSave = m_pEngine->SwitchToOurGlobalProgram();

    HRESULT hr = _LocateNameInScope(pdstrName, &pObj, &pVal);

    (void)m_pEngine->SetGlobalProgramContext(pccSave);
    m_pEngine->RestoreSavedGlobalProgram(pprogSave);

    if (SUCCEEDED(hr) && pvarResult)
    {
        ValueToVariant(pvarResult, pVal);
        return S_OK;
    }

    return hr;
} // _OnPropertyGet


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _OnPropertyPut()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT CNamedItemDisp::_OnPropertyPut(DISPID dispidMember, d_string pdstrName,
                                       WORD wFlags, DISPPARAMS* pDispParams,
                                       VARIANT* pvarResult,
                                       EXCEPINFO* pExcepInfo,
                                       UINT* puArgErr)
{
#if LOG
    FuncLog(L"CNamedItemDisp::_OnPropertyPut()");
#endif
    UNREFERENCED_PARAMETER(pvarResult); // pvarResult is ignored in this case
    UNREFERENCED_PARAMETER(puArgErr);   // Only used with DISP_E_TYPEMISMATCH or DISP_E_PARAMNOTFOUND...

    UNREFERENCED_PARAMETER(dispidMember);
    UNREFERENCED_PARAMETER(pdstrName);
    UNREFERENCED_PARAMETER(pDispParams);
    UNREFERENCED_PARAMETER(pExcepInfo);
    invariant();

    assert(NULL != m_pcc && NULL != m_pcc->scope);

    Dobject*    pObj = NULL;
    Value*      pvalRet = NULL;
    Value       value;
    HRESULT     hr;

    CallContext*    pccSave = m_pEngine->SwitchGlobalProgramContext(m_pcc);
    Program*        pprogSave = m_pEngine->SwitchToOurGlobalProgram();

    pObj = (Dobject *)m_pcc->scope->data[m_pcc->scoperoot-1];

    hr = VariantToValue(&value, &(pDispParams->rgvarg[0]), d_string_ctor(L""), NULL);
    if (SUCCEEDED(hr))
    {
        // This is a case-sensitive search per ECMA
        pvalRet = pObj->Put(pdstrName, &value, 0);

        (void)m_pEngine->SetGlobalProgramContext(pccSave);
        m_pEngine->RestoreSavedGlobalProgram(pprogSave);

        //
        // One example of a failed ret is if CanPut would have failed -- a read/only property.
        //
        if (pvalRet)
            return DISP_E_MEMBERNOTFOUND;

        return S_OK;
    }
    else
    {
        (void)m_pEngine->SetGlobalProgramContext(pccSave);
        m_pEngine->RestoreSavedGlobalProgram(pprogSave);
    }

    return hr;
} // _OnPropertyPut


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _OnMethodCall()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT CNamedItemDisp::_OnMethodCall(DISPID dispidMember, d_string pdstrName,
                                      WORD wFlags, DISPPARAMS* pDispParams,
                                      VARIANT* pvarResult,
                                      EXCEPINFO* pExcepInfo,
                                      UINT* puArgErr)
{
#if LOG
    FuncLog(L"CNamedItemDisp::_OnMethodCall()");
#endif
    invariant();
    UNREFERENCED_PARAMETER(dispidMember);
    UNREFERENCED_PARAMETER(pdstrName);
    UNREFERENCED_PARAMETER(pvarResult);
    UNREFERENCED_PARAMETER(pExcepInfo);

    Dobject*    poThis = NULL;
    Dobject*    pObj = NULL;
    Value*      pvalNamedObject = NULL;
    bool        fExited = false;
    unsigned    j = 0;
    unsigned    argc = 0;
    Value*      pvalArgs = NULL;

    CallContext*    pccSave = m_pEngine->SwitchGlobalProgramContext(m_pcc);
    Program*        pprogSave = m_pEngine->SwitchToOurGlobalProgram();

    HRESULT hr = _LocateNameInScope(pdstrName, &pObj, &pvalNamedObject);

    if (FAILED(hr))
        goto RestoreContextAndReturn;

    // Get the 'othis' pointer from the named arguments
    switch (pDispParams->cNamedArgs)
    {
        case 0:
            poThis = pObj;
            break;

        case 1:
        {
//            DPF(L"COMNID: _OnMethodCall() : one named argument");
            if (wFlags & DISPATCH_CONSTRUCT)
            {
                // Constructors don't have an othis
                hr = DISP_E_BADPARAMCOUNT;
                goto RestoreContextAndReturn;
            }

            //
            // We can accept one named argument,
            // which should be the 'othis' pointer
            //
            if (pDispParams->rgdispidNamedArgs[0] != DISPID_THIS)
            {
                if (puArgErr)
                    *puArgErr = 0;              // arg 0 contains the error
                hr = DISP_E_PARAMNOTFOUND;
                goto RestoreContextAndReturn;
            }

            Value   ovalue;
            Value*  pvalTemp = NULL;

            pvalTemp = &ovalue;                 // so the toObject() call is polymorphic
            hr = VariantToValue(pvalTemp, &pDispParams->rgvarg[0], d_string_ctor(L""), NULL);
            if (FAILED(hr))
                goto RestoreContextAndReturn;

            poThis = pvalTemp->toObject();      // make sure toObject() is a polymorphic call
            break;
        }
        default:
            DPF(L"COMNID: ERROR _OnMethodCall() : bad param count");
            return DISP_E_BADPARAMCOUNT;
    }

    //
    // Prepare argument list built from all the unnamed arguments
    //

    pvalArgs = NULL;
    argc = pDispParams->cArgs - pDispParams->cNamedArgs;
    if (0 != argc)
    {
        if (argc >= 32 || (pvalArgs = (Value *)alloca(sizeof(Value) * argc)) == NULL)
        {
	    // Not enough stack space - use mem
	    pvalArgs = (Value *)mem.malloc(sizeof(Value) * argc);
        }
    }
    for (j = 0; j < argc; j++)
    {
        //
        // Convert COM variants to DScript Values
        //
        hr = VariantToValue(&pvalArgs[j],
                            &(pDispParams->rgvarg[pDispParams->cNamedArgs + argc - j - 1]),
                            d_string_ctor(L""), NULL);
        if (FAILED(hr))
        {
            DPF(L"COMNID: _OnMethodCall() ERROR in VariantToValue! (hr = 0x%08lx)", hr);
            goto RestoreContextAndReturn;
        }
    }

#if defined _MSC_VER
    __try
#endif
    {
        Value    valueRet;
        Value*   pvalResult = NULL;

        m_pEngine->EnterScript();

	Value::copy(&valueRet, &vundefined);
        if (wFlags & DISPATCH_CONSTRUCT)
            pvalResult = (Value *)pvalNamedObject->Construct(m_pcc, &valueRet, argc, pvalArgs);
        else
            pvalResult = (Value *)pvalNamedObject->Call(m_pcc, poThis, &valueRet, argc, pvalArgs);

	//WPRINTF(L"pvalResult = %x\n", pvalResult);

        if (NULL != pvalResult)
        {
            //WPRINTF(ret->toString());

            ErrInfo errinfo;
            errinfo.message = d_string_ptr(pvalResult->toString());
            errinfo.linnum = 0;

            hr = m_pEngine->HandleScriptError(&errinfo, pExcepInfo, m_dwSourceContext);

            fExited = true;
            m_pEngine->ExitScript();
            goto RestoreContextAndReturn;
        }

        fExited = true;
        m_pEngine->ExitScript();

        //
        // Convert return type to Variant
        //
        if (NULL != pvarResult)
            ValueToVariant(pvarResult, &valueRet);
    }
#if defined _MSC_VER
    __except(EXCEPTION_CONTINUE_EXECUTION)
    {
        DPF(L"COMNID: _OnMethodCall() ERROR: caught exception from COM call...\n");
        if (!fExited)
            m_pEngine->ExitScript();

        hr = DISP_E_MEMBERNOTFOUND;
        goto RestoreContextAndReturn;
    }
#endif

    hr = S_OK;

RestoreContextAndReturn:
    (void)m_pEngine->SetGlobalProgramContext(pccSave);
    m_pEngine->RestoreSavedGlobalProgram(pprogSave);

    ENTRYLOG(L"COMNID: EXIT _OnMethodCall() (hr = 0x%08lx)\n", hr);
    return hr;
} // _OnMethodCall


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _AddIDName()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
DISPID CNamedItemDisp::_AddIDName(d_string pdstr)
{
#if LOG
    FuncLog(L"CNamedItemDisp::_AddIDName()");
#endif
    invariant();
    m_rgIDMap.push(pdstr);
    return (DISPID)(m_rgIDMap.dim - 1 + DISPID_BASE);
} // _AddIDName


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _NameToDISPID()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
DISPID CNamedItemDisp::_NameToDISPID(d_string pdstr)
{
    // PERF: This is a linear search - not very efficient.
    //       I don't think this will be a problem, however.

#if LOG
    FuncLog(L"CNamedItemDisp::_NameToDISPID()");
#endif
    invariant();
    for (unsigned i = 0; i < m_rgIDMap.dim; i++)
    {
        if (_AreNamesEqual(pdstr, (d_string)m_rgIDMap.data[i]))
            return i + DISPID_BASE;
    }
    return 0;
} // _NameToDISPID


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _DISPIDToName()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
d_string CNamedItemDisp::_DISPIDToName(DISPID id)
{
#if LOG
    FuncLog(L"CNamedItemDisp::_DISPIDToName()");
#endif
    invariant();
    assert(id >= DISPID_BASE);

    unsigned i = id - DISPID_BASE;

    if (i < 0 || i >= m_rgIDMap.dim)
        return NULL;
    return (d_string)m_rgIDMap.data[i];
}



