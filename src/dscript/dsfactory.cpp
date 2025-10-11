/* Digital Mars DMDScript source code.
 * Copyright (c) 2000-2001 by Chromium Communications
 * Copyright (c) 2004-2007 by Digital Mars
 * All Rights Reserved.
 * written by Walter Bright and Paul R. Nash
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

//
// DScript intepreter includes
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "dscript.h"
#include "root.h"
#include "dchar.h"
#include "dobject.h"
#include "program.h"
#include "identifier.h"
#include "lexer.h"
#include "mem.h"

#if defined __DMC__
#include "dmc_rpcndr.h"
#endif

#include <objbase.h>
#include <activscp.h>
#include <objsafe.h>
#include "OLEScript.h"
#include "dsfactory.h"

//
// Global data
//
extern LONG g_cComponents;
extern LONG g_cServerLocks;

DSFactory::DSFactory()
{
    m_cRef = 1;

#if INVARIANT
    signature = DSFACTORY_SIGNATURE;
#endif
    InterlockedIncrement(&g_cComponents);
}

DSFactory::~DSFactory()
{
    invariant();
    InterlockedDecrement(&g_cComponents);
#if INVARIANT
    signature = 0;
#endif
}

HRESULT
__stdcall
DSFactory::QueryInterface(REFIID riid,
                          void **ppvObject)
{
    invariant();
    if ((riid == IID_IUnknown) || (riid == IID_IClassFactory)) {
        *ppvObject = static_cast<IClassFactory*>(this);
    }
    else {
        *ppvObject = 0;

        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();

    return S_OK;
}

ULONG
__stdcall
DSFactory::AddRef()
{
    invariant();
    return InterlockedIncrement(&m_cRef);
}

ULONG
__stdcall
DSFactory::Release()
{
    invariant();
    if (InterlockedDecrement(&m_cRef) == 0) {
        delete this;

        return 0;
    }

    return m_cRef;
}

HRESULT
__stdcall
DSFactory::CreateInstance(IUnknown *pUnknownOuter,
                          const IID& iid,
                          void ** ppv)
{
    HRESULT hResult = E_FAIL;

    invariant();
    mem.setStackBottom((void *)&ppv);
    if (NULL != pUnknownOuter)
        return CLASS_E_NOAGGREGATION;
    if (NULL == ppv)
        return E_INVALIDARG;

    //WPRINTF(L"DSFactory::CreateInstance()\n");
    COleScript* pScriptObj = new COleScript;
    if (NULL == pScriptObj)
    {
        return E_OUTOFMEMORY;
    }

    hResult = pScriptObj->QueryInterface(iid, ppv);

    //
    // Release the IUnknown pointer
    //
    pScriptObj->Release();

    return hResult;
}

HRESULT
__stdcall
DSFactory::LockServer(BOOL bLock)
{
    invariant();
    if (bLock) {
        InterlockedIncrement(&g_cServerLocks);
    }
    else {
        InterlockedDecrement(&g_cServerLocks);
    }

    return S_OK;
}
