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

#ifndef _NAMEDITEMDISP_H_
#define _NAMEDITEMDISP_H_

#include "BaseDispatchEx.h"

class CNamedItemDisp : public CBaseDispatchEx
{
public:
#if INVARIANT
#   define CNAMEDITEMDISP_SIGNATURE	0xCEAF3213
    unsigned signature;

    void invariant()
    {
	assert(signature == CNAMEDITEMDISP_SIGNATURE);
    }
#else
    void invariant() { }
#endif

    //
    // Constructors/destructor
    //
    CNamedItemDisp();

    HRESULT Initialize(class COleScript* pEngine, CallContext* pcc, bool fGlobal,
		DWORD dwSourceContext);

protected:
    virtual ~CNamedItemDisp();      // We delete ourselves through Release() to zero

    virtual HRESULT _DoGetDispID(BSTR bszName, DWORD grfdex, DISPID* pID);

    virtual HRESULT _DoInvoke(DISPID dispidMember, REFIID riid, LCID lcid,
                              WORD wFlags, DISPPARAMS* pDispParams,
                              VARIANT* pvarResult, EXCEPINFO* pExcepInfo,
                              UINT* puArgErr);

protected:
    //
    // Helper function performs common task of looking through
    // the scope chain for a named object
    //
    HRESULT _LocateNameInScope(d_string pszName, Dobject** ppObj, Value** ppVal);

    virtual HRESULT _OnPropertyGet(DISPID dispidMember, d_string pszName,
                                   WORD wFlags, DISPPARAMS* pDispParams,
                                   VARIANT* pvarResult, EXCEPINFO* pExcepInfo,
                                   UINT* puArgErr);

    virtual HRESULT _OnPropertyPut(DISPID dispidMember, d_string pszName,
                                   WORD wFlags, DISPPARAMS* pDispParams,
                                   VARIANT* pvarResult, EXCEPINFO* pExcepInfo,
                                   UINT* puArgErr);

    virtual HRESULT _OnMethodCall(DISPID dispidMember, d_string pszName,
                                  WORD wFlags, DISPPARAMS* pDispParams,
                                  VARIANT* pvarResult, EXCEPINFO* pExcepInfo,
                                  UINT* puArgErr);

    // Manage mapping between opaque data and DISPID's
    DISPID _AddIDName(Lstring* plstr);
    DISPID _NameToDISPID(Lstring* plstr);
    Lstring* _DISPIDToName(DISPID id);

    //
    // For equality only -- returns true or false (this is slightly faster)
    //
    virtual bool _AreNamesEqual(Lstring* plstrLeft, Lstring* plstrRight);

protected:
public:			// for debugging
    static const DISPID DISPID_BASE;

    Array       m_rgIDMap;

    class COleScript*   m_pEngine;
    CallContext*        m_pcc;
    bool                m_fGlobal;
    DWORD		m_dwSourceContext;
}; // class CNamedItemDisp


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _AreNamesEqual()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
inline bool CNamedItemDisp::_AreNamesEqual(Lstring* plstrLeft, Lstring* plstrRight)
{
    return (0 == d_string_cmp(plstrLeft, plstrRight));
} // _AreEntriesEqual



#endif // _NAMEDITEMDISP_H_



