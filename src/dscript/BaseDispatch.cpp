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
#include "BaseDispatch.h"

#define LOG 0

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// CBaseDispatch()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
CBaseDispatch::CBaseDispatch()
{
#if LOG
    WPRINTF(L"CBaseDispatch()\n");
#endif
} // CBaseDispatch


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// ~CBaseDispatch()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
CBaseDispatch::~CBaseDispatch()
{
#if LOG
    WPRINTF(L"~CBaseDispatch()\n");
#endif
} // ~CBaseDispatch


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// QueryInterface()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP CBaseDispatch::QueryInterface(REFIID riid, void** ppvObj)
{
#if LOG
    FuncLog(L"CBaseDispatch::QueryInterface()");
#endif
    if (NULL == ppvObj)
        return E_INVALIDARG;

    if (riid == IID_IUnknown) {
       *ppvObj = static_cast<IDispatch*>(this);
    }
    else if (riid == IID_IDispatch) {
       *ppvObj = static_cast<IDispatch*>(this);
    }
    else {
       *ppvObj = NULL;

       return E_NOINTERFACE;
    }

    assert(NULL != *ppvObj);
    reinterpret_cast<IUnknown*>(*ppvObj)->AddRef();

    return S_OK;
} // QueryInterface


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// GetTypeInfoCount()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP CBaseDispatch::GetTypeInfoCount(UINT* pcTypeInfos)
{
#if LOG
    FuncLog(L"CBaseDispatch::GetTypeInfoCount()");
#endif
    if (NULL == pcTypeInfos)
        return E_INVALIDARG;

    *pcTypeInfos = 0;
    return S_OK;
} // GetTypeInfoCount


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// GetTypeInfo()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP CBaseDispatch::GetTypeInfo(UINT idx, LCID lcid, ITypeInfo** ppti)
{
#if LOG
    FuncLog(L"CBaseDispatch::GetTypeInfo()");
#endif
    UNREFERENCED_PARAMETER(idx);
    UNREFERENCED_PARAMETER(lcid);

    if (NULL == ppti)
        return E_INVALIDARG;

    *ppti = NULL;
    return DISP_E_BADINDEX;
} // GetTypeInfo


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// GetIDsOfNames()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP CBaseDispatch::GetIDsOfNames(REFIID riid, OLECHAR** prgszNames, UINT cNames,
                                           LCID lcid, DISPID* prgIDs)
{
#if LOG
    FuncLog(L"CBaseDispatch::GetIDsOfNames()");
#endif
    mem.setStackBottom((void *)&prgIDs);
    if (IID_NULL != riid)
        return DISP_E_UNKNOWNINTERFACE;

    if (NULL == prgszNames || NULL == prgIDs)
        return E_INVALIDARG;

    //
    // Ideally we'd only accept LCID_ENGLISH_US, but some less-well-coded clients
    // will expect us to work with an LCID of 0 (LOCALE_NEUTRAL) passed in to Invoke.
    //
    if (LCID_ENGLISH_US != lcid && LOCALE_NEUTRAL != lcid
	&& LANG_SYSTEM_DEFAULT != lcid)
        return DISP_E_UNKNOWNLCID;

    return _DoGetIDsOfNames(riid, prgszNames, cNames, lcid, prgIDs);
} // GetIDsOfNames


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Invoke()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP CBaseDispatch::Invoke(DISPID dispidMember, REFIID riid, LCID lcid,
                                    WORD wFlags, DISPPARAMS* pDispParams,
                                    VARIANT* pvarResult, EXCEPINFO* pExcepInfo,
                                    UINT* puArgErr)
{
#if LOG
    FuncLog(L"CBaseDispatch::Invoke()");
#endif
    mem.setStackBottom((void *)&puArgErr);
    return _InvokeBase(dispidMember, riid, lcid, wFlags, pDispParams,
                       pvarResult, pExcepInfo, puArgErr, NULL);
} // Invoke


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _InvokeBase()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT CBaseDispatch::_InvokeBase(DISPID dispidMember, REFIID riid, LCID lcid,
                                   WORD wFlags, DISPPARAMS* pDispParams,
                                   VARIANT* pvarResult, EXCEPINFO* pExcepInfo,
                                   UINT* puArgErr, IServiceProvider* pspCaller)
{
    UNREFERENCED_PARAMETER(pspCaller);

#if LOG
    FuncLog(L"CBaseDispatch::_InvokeBase()");
#endif
    if (IID_NULL != riid)
    {
        return DISP_E_UNKNOWNINTERFACE;
    }

    //
    // Most (all?) implementations of IDispatch do not allow a NULL pDispParams.
    //
    if (NULL == pDispParams)
    {
        return E_INVALIDARG;
    }

    //
    // Ideally we'd only accept LCID_ENGLISH_US, but some less-well-coded clients
    // will expect us to work with an LCID of 0 (LOCALE_NEUTRAL) passed in to Invoke.
    //
    if (LCID_ENGLISH_US != lcid && LOCALE_NEUTRAL != lcid && 0x800 != lcid)
    {
	//WPRINTF(L"lcid = x%x\n", lcid);
	return DISP_E_UNKNOWNLCID;
    }

    //
    // These are the documented allowable combinations of flags to
    // IDispatch/IDispatchEx. All other combinations generate an error.
    //
    switch(wFlags)
    {
    case DISPATCH_CONSTRUCT:
        //
        // Construct is only valid on objects that support IDispatchEx
        //
        if (!_IsDispEx())
	{
            return E_INVALIDARG;
	}
    case DISPATCH_PROPERTYGET:
    case DISPATCH_PROPERTYGET | DISPATCH_METHOD:
    case DISPATCH_METHOD:
    case DISPATCH_PROPERTYPUT:
    case DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF:
    case DISPATCH_PROPERTYPUTREF:
        break;
    default:
        return E_INVALIDARG;
    }

    //
    // Must init the result value VARIANT (NOT clear, because this is an out-only param)
    //
    if (NULL != pvarResult)
        VariantInit(pvarResult);

    // psp not passed to _DoInvoke() because it's not used
    return _DoInvoke(dispidMember, riid, lcid, wFlags, pDispParams,
                     pvarResult, pExcepInfo, puArgErr);
} // _InvokeBase


