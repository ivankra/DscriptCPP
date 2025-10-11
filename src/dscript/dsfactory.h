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

#ifndef _DSFACTORY_H_
#define _DSFACTORY_H_

#include "OLECommon.h"

class DSFactory : public NoGCBase, public IClassFactory
{
public:
#if INVARIANT
#   define DSFACTORY_SIGNATURE	0xFEAE3514
    unsigned signature;

    void invariant()
    {
	assert(signature == DSFACTORY_SIGNATURE);
    }
#else
    void invariant() { }
#endif

    //
    // IUnknown
    //
    virtual
    HRESULT
    __stdcall
    QueryInterface(REFIID riid,
                   void **ppvObject);

    virtual
    ULONG
    __stdcall
    AddRef();

    virtual
    ULONG
    __stdcall
    Release();

    //
    // IClassFactory interface implementation
    //
    virtual
    HRESULT
    __stdcall
    CreateInstance(IUnknown *pUnknownOuter,
                   const IID& iid,
                   void ** ppv);

    virtual
    HRESULT
    __stdcall
    LockServer(BOOL bLock);

    //
    // Constructors/destructor
    //
    DSFactory();

    ~DSFactory();

private:
    long m_cRef;
};

#endif //_DSFACTORY_H_
