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


#ifndef _CBASEDISPATCH_H_
#define _CBASEDISPATCH_H_

#include "BaseComObject.h"

#define LCID_ENGLISH_US MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT)

class CBaseDispatch : public CBaseComObject, public IDispatch
{
public:
    //
    // Constructors/destructor
    //
    CBaseDispatch();

protected:
    virtual ~CBaseDispatch();      // We delete ourselves through Release() to zero

    virtual bool _IsDispEx()    { return false; }

    //
    // Does the basic work of validating params and flags for whatever Invoke()
    // entry point is the main one for this class (Invoke for Dispatch, and
    // InvokeEx for DispatchEx).
    //
    HRESULT _InvokeBase(DISPID dispidMember, REFIID riid, LCID lcid,
                        WORD wFlags, DISPPARAMS* pDispParams,
                        VARIANT* pvarResult, EXCEPINFO* pExcepInfo,
                        UINT* puArgErr, IServiceProvider* psp);

    virtual HRESULT _DoGetIDsOfNames(REFIID riid, OLECHAR** prgszNames,
                                     UINT cNames, LCID lcid, DISPID* prgIDs) = 0;

    virtual HRESULT _DoInvoke(DISPID dispidMember, REFIID riid, LCID lcid,
                              WORD wFlags, DISPPARAMS* pDispParams,
                              VARIANT* pvarResult, EXCEPINFO* pExcepInfo,
                              UINT* puArgErr) = 0;

public:
    //
    // IUnknown
    //
    STDMETHODIMP_(ULONG) AddRef() {return CBaseComObject::AddRef();};
    STDMETHODIMP_(ULONG) Release() {return CBaseComObject::Release();};
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);

    //
    // IDispatch
    //
    STDMETHODIMP GetTypeInfoCount(UINT* pcTypeInfos);
    
    STDMETHODIMP GetTypeInfo(UINT idx, LCID lcid, ITypeInfo** ppti);

    STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** prgszNames, UINT cNames,
                               LCID lcid, DISPID* prgIDs);

    STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
                        DISPPARAMS* pDispParams, VARIANT* pvarResult,
                        EXCEPINFO* pExcepInfo, UINT* puArgErr);
}; // class CBaseDispatch



#endif // _CBASEDISPATCH_H_



