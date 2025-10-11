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

#include <windows.h>
#include <assert.h>
#include <malloc.h>

#include <objbase.h>
#include <dispex.h>

#if defined linux
#include <string.h>
#endif

#include "dscript.h"
#include "printf.h"
#include "ActiveScriptError.h"
#include "OLECommon.h"

#define LOG	0

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// CActiveScriptError()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
CActiveScriptError::CActiveScriptError()
: m_bszSource(NULL),
  m_nLine(0),
  m_nChar(-1),
  m_dwSourceContextCookie(0)
{
#if LOG
    FuncLog f(L"CActiveScriptError()");
#endif
    memset(&m_ExcepInfo, 0, sizeof(m_ExcepInfo));
#if INVARIANT
    signature = CACTIVESCRIPTERROR_SIGNATURE;
#endif
} // CActiveScriptError


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// ~CActiveScriptError()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
CActiveScriptError::~CActiveScriptError()
{
#if LOG
    FuncLog f(L"~CActiveScriptError()");
#endif
    invariant();
    _ClearExcepInfo(&m_ExcepInfo);
    if (NULL != m_bszSource)
    {
	SysFreeString(m_bszSource);
	m_bszSource = NULL;
    }
#if INVARIANT
    signature = 0;
#endif
} // ~CActiveScriptError


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Initialize()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT CActiveScriptError::Initialize(EXCEPINFO* pExcepInfo,
                                       const WCHAR *pcwszSource,
                                       ULONG nLine, LONG nChar,
					DWORD dwSourceContextCookie)
{
#if LOG
    FuncLog f(L"CActiveScriptError::Initialize()");
#endif
    invariant();
    _ClearExcepInfo(&m_ExcepInfo);
    HRESULT hr = _CopyExcepInfo(&m_ExcepInfo, pExcepInfo);

    if (FAILED(hr))
        return hr;

    if (NULL != pcwszSource)
    {
        m_bszSource = SysAllocString(pcwszSource);
        if (NULL == m_bszSource)
        {
            _ClearExcepInfo(&m_ExcepInfo);
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        m_bszSource = NULL;
    }

    m_nLine = nLine;
    //WPRINTF(L"m_nLine = %d\n", m_nLine);
    m_nChar = nChar;
    m_dwSourceContextCookie = dwSourceContextCookie;

    return S_OK;
} // Initialize


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _CopyExcepInfo()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT CActiveScriptError::_CopyExcepInfo(EXCEPINFO* peiDest, EXCEPINFO* peiSrc)
{
#if LOG
    FuncLog f(L"CActiveScriptError::_CopyExcepInfo()");
#endif
    invariant();
    assert(NULL != peiDest);
    assert(NULL != peiSrc);

    memcpy(peiDest, peiSrc, sizeof(EXCEPINFO));
    peiDest->bstrSource = peiDest->bstrDescription = peiDest->bstrHelpFile = NULL;

    if (NULL != peiSrc->bstrSource)
    {
        peiDest->bstrSource = SysAllocString(peiSrc->bstrSource);
        if (NULL == peiDest->bstrSource)
            goto NoMem;
    }

    if (NULL != peiSrc->bstrDescription)
    {
        peiDest->bstrDescription = SysAllocString(peiSrc->bstrDescription);
        if (NULL == peiDest->bstrDescription)
            goto NoMem;
    }

    if (NULL != peiSrc->bstrHelpFile)
    {
        peiDest->bstrHelpFile = SysAllocString(peiSrc->bstrHelpFile);
        if (NULL == peiDest->bstrHelpFile)
            goto NoMem;
    }

    return S_OK;

NoMem:
    _ClearExcepInfo(peiDest);
    return E_OUTOFMEMORY;
} // _CopyExcepInfo


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _ClearExcepInfo()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void CActiveScriptError::_ClearExcepInfo(EXCEPINFO* pEI)
{
#if LOG
    FuncLog f(L"CActiveScriptError::_ClearExcepInfo()");
#endif
    invariant();
    assert(NULL != pEI);

    if (NULL != pEI->bstrSource)
        SysFreeString(pEI->bstrSource);

    if (NULL != pEI->bstrDescription)
        SysFreeString(pEI->bstrDescription);

    if (NULL != pEI->bstrHelpFile)
        SysFreeString(pEI->bstrHelpFile);

    memset(pEI, 0, sizeof(EXCEPINFO));
} // _ClearExcepInfo


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// QueryInterface()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP CActiveScriptError::QueryInterface(REFIID riid, void** ppvObj)
{
#if LOG
    FuncLog f(L"CActiveScriptError::QueryInterface()");
#endif
    invariant();
    if (NULL == ppvObj)
        return E_INVALIDARG;

    if (riid == IID_IActiveScriptError) {
       *ppvObj = static_cast<IActiveScriptError*>(this);
    }
    else if (riid == IID_IUnknown) {
       *ppvObj = static_cast<IActiveScriptError*>(this);
    }
    else
    {
#if LOG
	WPRINTF(L"\tE_NOINTERFACE\n");
#endif
	*ppvObj = NULL;
	return E_NOINTERFACE;
    }

    assert(NULL != *ppvObj);
    reinterpret_cast<IUnknown*>(*ppvObj)->AddRef();

    return S_OK;
} // QueryInterface


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// GetExceptionInfo()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP CActiveScriptError::GetExceptionInfo(/*[out]*/ EXCEPINFO* pEI)
{
#if LOG
    FuncLog f(L"CActiveScriptError::GetExceptionInfo()");
#endif
    invariant();
    if (NULL == pEI)
        return E_INVALIDARG;

    //
    // NOTE: Don't call _ClearExcepInfo() on pEI -- we have to assume there's
    //       nothing important in pEI, because it's just an [out] param, not
    //       an [in] param. If there's anything leaked in there, it's not our fault.
    //
    return _CopyExcepInfo(pEI, &m_ExcepInfo);
} // GetExceptionInfo


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// GetSourcePosition()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP CActiveScriptError::GetSourcePosition(/*[out]*/ DWORD* pdwSourceContext,
                                                   /*[out]*/ ULONG* puLine, /*[out]*/ LONG* plChar)
{
#if LOG
    FuncLog f(L"CActiveScriptError::GetSourcePosition()");
#endif
    invariant();
    if (NULL != pdwSourceContext)
    {
#if LOG
	WPRINTF(L"\tdwSourceContext = %x\n", m_dwSourceContextCookie);
#endif
        *pdwSourceContext = m_dwSourceContextCookie;
    }

    if (NULL != puLine)
    {
#if LOG
	WPRINTF(L"\tuLine = %d\n", m_nLine);
#endif
        *puLine = m_nLine;
    }
    if (NULL != plChar)
    {
#if LOG
	WPRINTF(L"\tlChar = %d\n", m_nChar);
#endif
	*plChar = m_nChar;
    }

    return S_OK;
} // GetSourcePosition


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// GetSourceLineText()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP CActiveScriptError::GetSourceLineText(/*[out]*/ BSTR* pbszSourceLine)
{
#if LOG
    FuncLog f(L"CActiveScriptError::GetSourceLineText()");
#endif
    invariant();
    if (NULL == pbszSourceLine)
        return E_INVALIDARG;

    //
    // NOTE: Do NOT call SysFreeString() on pbszSourceLine -- it's an out-only param.
    //
    if (NULL == m_bszSource)
        *pbszSourceLine = SysAllocString(L" ");
    else
    {   *pbszSourceLine = SysAllocString(m_bszSource);
#if LOG
	WPRINTF(L"SourceLine = '%s'\n", *pbszSourceLine);
#endif
    }
    if (NULL == *pbszSourceLine)
        return E_OUTOFMEMORY;

    return S_OK;
} // GetSourceLineText


