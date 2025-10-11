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

#include "dscript.h"
#include "BaseComObject.h"

//
// Global data
//
extern LONG g_cComponents;


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// CBaseComObject()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
CBaseComObject::CBaseComObject()
  : m_cRef(1)
{
    InterlockedIncrement(&g_cComponents);
} // CBaseComObject


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// ~CBaseComObject()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
CBaseComObject::~CBaseComObject()
{
    InterlockedDecrement(&g_cComponents);
} // ~CBaseComObject


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// AddRef()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP_(ULONG) CBaseComObject::AddRef()
{
    return InterlockedIncrement(&m_cRef);
} // AddRef


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Release()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP_(ULONG) CBaseComObject::Release()
{
    LONG lRef = InterlockedDecrement(&m_cRef);
    if (lRef == 0)
    {
        delete this;
        return 0;
    }

    return (ULONG)lRef;
} // Release
