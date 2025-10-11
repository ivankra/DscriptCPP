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
#include "BaseDispatchEx.h"

#define LOG 0

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// CBaseDispatchEx()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
CBaseDispatchEx::CBaseDispatchEx()
{
#if LOG
    WPRINTF(L"CBaseDispatchEx()\n");
#endif
} // CBaseDispatchEx


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// ~CBaseDispatchEx()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
CBaseDispatchEx::~CBaseDispatchEx()
{
#if LOG
    WPRINTF(L"~CBaseDispatchEx()\n");
#endif
} // ~CBaseDispatchEx


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// QueryInterface()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP CBaseDispatchEx::QueryInterface(REFIID riid, void** ppvObj)
{
#if LOG
    FuncLog(L"CBaseDispatchEx::QueryInterface()");
#endif
    mem.setStackBottom((void *)&ppvObj);
    if (NULL == ppvObj)
        return E_INVALIDARG;

    if (riid == IID_IDispatchEx) {
       *ppvObj = static_cast<IDispatchEx*>(this);
    }
    else if (riid == IID_IDispatch) {
       *ppvObj = static_cast<IDispatchEx*>(this);
    }
    else if (riid == IID_IUnknown) {
       *ppvObj = static_cast<IDispatchEx*>(this);
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
// Invoke()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP CBaseDispatchEx::Invoke(DISPID dispidMember, REFIID riid, LCID lcid,
                                    WORD wFlags, DISPPARAMS* pDispParams,
                                    VARIANT* pvarResult, EXCEPINFO* pExcepInfo,
                                    UINT* puArgErr)
{
#if LOG
    FuncLog(L"CBaseDispatchEx::Invoke()");
#endif
    mem.setStackBottom((void *)&puArgErr);
    UNREFERENCED_PARAMETER(riid);
    UNREFERENCED_PARAMETER(puArgErr);

    return InvokeEx(dispidMember, lcid, wFlags, pDispParams,
                    pvarResult, pExcepInfo, NULL);
} // Invoke


//
// IDispatchEx
//
STDMETHODIMP CBaseDispatchEx::GetDispID(/*[in] */ BSTR bstrName,
                                        /*[in] */ DWORD grfdex,
                                        /*[out]*/ DISPID* pID)
{
#if LOG
    FuncLog(L"CBaseDispatchEx::GetDispID()");
#endif
    mem.setStackBottom((void *)&pID);
    return _DoGetDispID(bstrName, grfdex, pID);
}


STDMETHODIMP CBaseDispatchEx::InvokeEx(/*[in] */ DISPID id,
                                       /*[in] */ LCID lcid,
                                       /*[in] */ WORD wFlags,
                                       /*[in] */ DISPPARAMS* pDispParams,
                                       /*[out]*/ VARIANT* pvarResult,                    // Can be NULL
                                       /*[out]*/ EXCEPINFO* pExcepInfo,                  // Can be NULL
                                       /*[in, unique]*/ IServiceProvider* pspCaller)     // Can be NULL
{
#if LOG
    FuncLog(L"CBaseDispatchEx::InvokeEx()");
#endif
    mem.setStackBottom((void *)&pspCaller);
    return _InvokeBase(id, IID_NULL, lcid, wFlags, pDispParams,
                       pvarResult, pExcepInfo, NULL, pspCaller);
}


STDMETHODIMP CBaseDispatchEx::DeleteMemberByName(/*[in] */ BSTR bstrName,
                                                 /*[in] */ DWORD grfdex)
{
    UNREFERENCED_PARAMETER(bstrName);
    UNREFERENCED_PARAMETER(grfdex);

#if LOG
    FuncLog(L"CBaseDispatchEx::DeleteMemberByName()");
#endif
    return E_NOTIMPL;
}


STDMETHODIMP CBaseDispatchEx::DeleteMemberByDispID(/*[in] */ DISPID id)
{
    UNREFERENCED_PARAMETER(id);

#if LOG
    FuncLog(L"CBaseDispatchEx::DeleteMemberByDispID()");
#endif
    return E_NOTIMPL;
}


STDMETHODIMP CBaseDispatchEx::GetMemberProperties(/*[in] */ DISPID id,
                                                  /*[in] */ DWORD grfdexFetch,
                                                  /*[out]*/ DWORD* pgrfdex)
{
    UNREFERENCED_PARAMETER(id);
    UNREFERENCED_PARAMETER(grfdexFetch);
    UNREFERENCED_PARAMETER(pgrfdex);

#if LOG
    FuncLog(L"CBaseDispatchEx::GetMemberProperties()");
#endif
    return E_NOTIMPL;
}


STDMETHODIMP CBaseDispatchEx::GetMemberName(/*[in] */ DISPID id,
                                            /*[out]*/ BSTR* pbstrName)
{
    UNREFERENCED_PARAMETER(id);
    UNREFERENCED_PARAMETER(pbstrName);

#if LOG
    FuncLog(L"CBaseDispatchEx::GetMemberName()");
#endif
    return E_NOTIMPL;
}


STDMETHODIMP CBaseDispatchEx::GetNextDispID(/*[in] */ DWORD grfdex,
                                            /*[in] */ DISPID id,
                                            /*[out]*/ DISPID* pid)
{
    UNREFERENCED_PARAMETER(grfdex);
    UNREFERENCED_PARAMETER(id);
    UNREFERENCED_PARAMETER(pid);

#if LOG
    FuncLog(L"CBaseDispatchEx::GetNextDispID()");
#endif
    return E_NOTIMPL;
}


STDMETHODIMP CBaseDispatchEx::GetNameSpaceParent(/*[out]*/ IUnknown** ppunk)
{
    UNREFERENCED_PARAMETER(ppunk);

#if LOG
    FuncLog(L"CBaseDispatchEx::GetNameSpaceParent()");
#endif
    return E_NOTIMPL;
}

HRESULT CBaseDispatchEx::_DoGetIDsOfNames(REFIID riid, OLECHAR** prgszNames,
                                          UINT cNames, LCID lcid, DISPID* prgIDs)
{
    UNREFERENCED_PARAMETER(riid);
    UNREFERENCED_PARAMETER(lcid);

#if LOG
    FuncLog(L"CBaseDispatchEx::_DoGetIDsOfNames()");
#endif
    UINT    idx = 0;
    BSTR    bszName = NULL;
    HRESULT hr = E_UNEXPECTED;

    for (idx = 0; idx < cNames; idx++)
    {
        bszName = SysAllocString(prgszNames[idx]);
        if (NULL == bszName)
            return E_OUTOFMEMORY;

        hr = GetDispID(bszName, fdexNameCaseInsensitive, &prgIDs[idx]);
        if (FAILED(hr))
            return hr;

        SysFreeString(bszName);
        bszName = NULL;
    }

    return S_OK;
}




