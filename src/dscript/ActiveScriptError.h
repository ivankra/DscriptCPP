/* Digital Mars DMDScript source code.
 * Copyright (c) 2000-2002 by Chromium Communications
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

#ifndef _ACTIVESCRIPTERROR_H_
#define _ACTIVESCRIPTERROR_H_

#include <activscp.h>

#include "dscript.h"
#include "BaseComObject.h"

class CActiveScriptError : public CBaseComObject, public IActiveScriptError
{
public:
#if INVARIANT
#   define CACTIVESCRIPTERROR_SIGNATURE	0xFC3EE154
    unsigned signature;

    void invariant()
    {
	assert(signature == CACTIVESCRIPTERROR_SIGNATURE);
    }
#else
    void invariant() { }
#endif

    //
    // Constructors/destructor
    //
    CActiveScriptError();

    HRESULT Initialize(EXCEPINFO* pExcepInfo, const WCHAR *pcwszSource,
	ULONG uLine, LONG nChar, DWORD dwSourceContextCookie);

protected:
    virtual ~CActiveScriptError();      // We delete ourselves through Release() to zero

    HRESULT _CopyExcepInfo(EXCEPINFO* peiDest, EXCEPINFO* peiSrc);
    void _ClearExcepInfo(EXCEPINFO* pEI);

public:
    //
    // IUnknown
    //
    STDMETHODIMP_(ULONG) AddRef() {return CBaseComObject::AddRef();};
    STDMETHODIMP_(ULONG) Release() {return CBaseComObject::Release();};
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);

    //
    // IActiveScriptError
    //
    STDMETHODIMP GetExceptionInfo(/*[out]*/ EXCEPINFO* pEI);
    
    STDMETHODIMP GetSourcePosition(/*[out]*/ DWORD* pdwSourceContext,
                                   /*[out]*/ ULONG* puLine,
                                   /*[out]*/ LONG* plChar);
    
    STDMETHODIMP GetSourceLineText(/*[out]*/ BSTR* pbszSourceLine);

protected:
    EXCEPINFO   m_ExcepInfo;
    BSTR        m_bszSource;
    int         m_nLine;
    int         m_nChar;
    DWORD	m_dwSourceContextCookie;
}; // class CActiveScriptError



#endif // _ACTIVESCRIPTERROR_H_



