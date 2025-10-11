/* Digital Mars DMDScript source code.
 * Copyright (c) 2000-2001 by Chromium Communications
 * Copyright (c) 2004-2007 by Digital Mars
 * All Rights Reserved.
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

#ifndef _DSOBJDISPATCH_H_
#define _DSOBJDISPATCH_H_

// This is a COM wrapper around a Dobject, so that
// COM can access Dobjects
// Many thanks to Paul N. for figuring much of this stuff out.

#include "OLECommon.h"

struct Program;

extern const GUID IID_DSobjDispatch;

class DSobjDispatch : public NoGCBase,
                      public IDispatchEx
{
public:
    DSobjDispatch(Dobject *obj, Program *prog);
    virtual ~DSobjDispatch();

    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP GetTypeInfoCount(unsigned *pctinfo);
    STDMETHODIMP GetTypeInfo(unsigned int iTInfo,
                             LCID lcid,
                             ITypeInfo FAR* FAR* ppTInfo);

    STDMETHODIMP GetIDsOfNames(REFIID riid,
                               OLECHAR FAR* FAR* rgszNames,
                               unsigned int cNames,
                               LCID lcid,
                               DISPID FAR* rgDispId);

    STDMETHODIMP Invoke(DISPID dispIdMember,
                        REFIID riid,
                        LCID lcid,
                        WORD wFlags,
                        DISPPARAMS FAR* pDispParams,
                        VARIANT FAR* pVarResult,
                        EXCEPINFO FAR* pExcepInfo,
                        unsigned int FAR* puArgErr);

    // IDispatchEx interface

    STDMETHODIMP GetDispID(/*in*/ BSTR bstrName,
                           /*in*/ DWORD grfdex,
                           /*out*/ DISPID *pid);

    STDMETHODIMP InvokeEx(/*in*/ DISPID id,
                          /*in*/ LCID lcid,
                          /*in*/ WORD wFlags,
                          /*in*/ DISPPARAMS *pdp,
                          /*out*/ VARIANT *pvarRes,   // Can be NULL.
                          /*out*/ EXCEPINFO *pei, // Can be NULL.
                          /*in, unique*/ IServiceProvider *pspCaller); // Can be NULL.

    STDMETHODIMP DeleteMemberByName(/*in*/ BSTR bstrName, /*in*/ DWORD grfdex);

    STDMETHODIMP DeleteMemberByDispID(/*in*/ DISPID id);

    STDMETHODIMP GetMemberProperties(/*in*/ DISPID id,
                                     /*in*/ DWORD grfdexFetch,
                                     /*out*/ DWORD *pgrfdex);

    STDMETHODIMP GetMemberName(/*in*/ DISPID id,
                               /*out*/ BSTR *pbstrName);

    STDMETHODIMP GetNextDispID(/*in*/ DWORD grfdex,
                               /*in*/ DISPID id,
                               /*out*/ DISPID *pid);

    STDMETHODIMP GetNameSpaceParent(/*out*/ IUnknown **ppunk);

    Dobject *getObj() { return m_pObj; }

private:
    LONG   m_cRef;
    Dobject *m_pObj;                    // the object we are wrapping
    Program *m_prog;
    CallContext* m_pcc;

    // Array of d_string's that maps d_string's to DISPID's.
    Array nameTable;

    static const DISPID DISPID_BASE;           // starting number for DISPID's

    // Manage mapping between names and DISPID's
    DISPID addName(d_string name);
    DISPID nameToDISPID(d_string name);
    d_string DISPIDToName(DISPID id);
};

#endif
