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

#ifndef _PARSEDPROCEDURE_H_
#define _PARSEDPROCEDURE_H_

#include "BaseDispatchEx.h"

class CParsedProcedure : public CBaseDispatchEx
{
public:
#if INVARIANT
#   define CPARSEDPROCEDURE_SIGNATURE	0xFAEE3154
    unsigned signature;

    void invariant()
    {
	assert(signature == CPARSEDPROCEDURE_SIGNATURE);
    }
#else
    void invariant() { }
#endif

    //
    // Constructors/destructor
    //
    CParsedProcedure();

    HRESULT Initialize(class COleScript* pEngine, FunctionDefinition* pfd,
           struct SNamedItemData* pnid, DWORD dwFlags, DWORD dwSourceContext);

protected:
    virtual ~CParsedProcedure();      // We delete ourselves through Release() to zero

    virtual HRESULT _DoGetDispID(BSTR bstrName, DWORD grfdex, DISPID* pID);

    virtual HRESULT _DoInvoke(DISPID dispidMember, REFIID riid, LCID lcid,
                              WORD wFlags, DISPPARAMS* pDispParams,
                              VARIANT* pvarResult, EXCEPINFO* pExcepInfo,
                              UINT* puArgErr);

protected:
    class COleScript*       m_pEngine;
    FunctionDefinition*     m_pfdCode;
    struct SNamedItemData*  m_pnid;
    DWORD                   m_dwFlags;
    DWORD		    m_dwSourceContext;

}; // class CParsedProcedure


#endif // _PARSEDPROCEDURE_H_



