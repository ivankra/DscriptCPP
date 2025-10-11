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

#ifndef _CBASECOMOBJECT_H_
#define _CBASECOMOBJECT_H_

#include "OLECommon.h"

class CBaseComObject : public NoGCBase, public IUnknown
{
public:
    //
    // Constructors/destructor
    //
    CBaseComObject();

protected:
    virtual ~CBaseComObject();      // We delete ourselves through Release() to zero

public:
    //
    // IUnknown
    //
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) = 0;

protected:
    LONG        m_cRef;

};

#endif // _CBASECOMOBJECT_H_

