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

#ifndef _CBASEDISPATCHEX_H_
#define _CBASEDISPATCHEX_H_

#include <dispex.h>
#include "BaseDispatch.h"


class CBaseDispatchEx : public CBaseDispatch, public IDispatchEx
{
public:
    //
    // Constructors/destructor
    //
    CBaseDispatchEx();

protected:
    virtual ~CBaseDispatchEx();      // We delete ourselves through Release() to zero

    virtual bool _IsDispEx()    { return true; }

    virtual HRESULT _DoInvoke(DISPID dispidMember, REFIID riid, LCID lcid,
                              WORD wFlags, DISPPARAMS* pDispParams,
                              VARIANT* pvarResult, EXCEPINFO* pExcepInfo,
                              UINT* puArgErr) = 0;

    virtual HRESULT _DoGetDispID(BSTR bstrName, DWORD grfdex, DISPID* pID) = 0;

private:
    //
    // We override this as private because we provide the standard implementation that
    // loops through and call's the base of GetDispID() for each entry. So there's a
    // different virtual the derived class should override instead: _DoGetDispID()
    //
    virtual HRESULT _DoGetIDsOfNames(REFIID riid, OLECHAR** prgszNames,
                                     UINT cNames, LCID lcid, DISPID* prgIDs);

public:
    //
    // IUnknown
    //
    STDMETHODIMP_(ULONG) AddRef() {return CBaseDispatch::AddRef();};
    STDMETHODIMP_(ULONG) Release() {return CBaseDispatch::Release();};
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);

    //
    // IDispatch
    //
    STDMETHODIMP GetTypeInfoCount(UINT* pcTypeInfos)  { return CBaseDispatch::GetTypeInfoCount(pcTypeInfos); }
    
    STDMETHODIMP GetTypeInfo(UINT idx, LCID lcid, ITypeInfo** ppti) { return CBaseDispatch::GetTypeInfo(idx, lcid, ppti); }

    STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** prgszNames, UINT cNames,
                               LCID lcid, DISPID* prgIDs) { return CBaseDispatch::GetIDsOfNames(riid, prgszNames, cNames, lcid, prgIDs); }

    STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
                        DISPPARAMS* pDispParams, VARIANT* pvarResult,
                        EXCEPINFO* pExcepInfo, UINT* puArgErr);

    //
    // IDispatchEx
    //

    STDMETHODIMP GetDispID(/*[in] */ BSTR bstrName,
                           /*[in] */ DWORD grfdex,
                           /*[out]*/ DISPID* pID);

    STDMETHODIMP InvokeEx(/*[in] */ DISPID id,
                          /*[in] */ LCID lcid,
                          /*[in] */ WORD wFlags,
                          /*[in] */ DISPPARAMS* pDispParams,
                          /*[out]*/ VARIANT* pvarResult,                    // Can be NULL
                          /*[out]*/ EXCEPINFO* pExcepInfo,                  // Can be NULL
                          /*[in, unique]*/ IServiceProvider* pspCaller);    // Can be NULL

    STDMETHODIMP DeleteMemberByName(/*[in] */ BSTR bstrName, /*[in] */ DWORD grfdex);

    STDMETHODIMP DeleteMemberByDispID(/*[in] */ DISPID id);

    STDMETHODIMP GetMemberProperties(/*[in] */ DISPID id,
                                     /*[in] */ DWORD grfdexFetch,
                                     /*[out]*/ DWORD* pgrfdex);

    STDMETHODIMP GetMemberName(/*[in] */ DISPID id,
                               /*[out]*/ BSTR* pbstrName);

    STDMETHODIMP GetNextDispID(/*[in] */ DWORD grfdex,
                               /*[in] */ DISPID id,
                               /*[out]*/ DISPID* pid);

    STDMETHODIMP GetNameSpaceParent(/*[out]*/ IUnknown** ppunk);

}; // class CBaseDispatchEx



#endif // _CBASEDISPATCHEX_H_



