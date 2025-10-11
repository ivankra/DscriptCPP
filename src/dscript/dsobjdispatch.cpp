/* Digital Mars DMDScript source code.
 * Copyright (c) 2000-2002 by Chromium Communications
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

#if defined linux
#  if !defined INITGUID
#    define INITGUID
#  endif
#endif

#include <windows.h>
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>

#include "dscript.h"
#include "root.h"
#include "expression.h"
#include "statement.h"
#include "identifier.h"
#include "symbol.h"
#include "scope.h"
#include "ir.h"
#include "value.h"
#include "property.h"
#include "dobject.h"
#include "program.h"
#include "text.h"
#include "mem.h"
#include "function.h"
#include "iterator.h"

#if defined __DMC__
#include "dmc_rpcndr.h"
#endif

#include <objbase.h>
#include <activscp.h>
#include <objsafe.h>
#include <dispex.h>

#include "dcomobject.h"
#include "dsobjdispatch.h"
#include "OLEScript.h"

#define LOG 0


// {A86CE249-C2B1-43a9-8379-FC730FF80C05}
const GUID IID_DSobjDispatch = 
{ 0xa86ce249, 0xc2b1, 0x43a9, { 0x83, 0x79, 0xfc, 0x73, 0xf, 0xf8, 0xc, 0x5 } };

const DISPID DSobjDispatch::DISPID_BASE = 4096;    // starting number for DISPID's


DSobjDispatch::DSobjDispatch(Dobject *obj, Program *prog)
        : m_cRef(1)
{
#if LOG
    FuncLog(L"DSobjDispatch()");
#endif
    m_pObj = obj;
#if !defined linux
    m_pObj->com = this;
#endif
    m_prog = prog;

    //
    // We need to copy the CallContext and scope chain at the time of our creation,
    // because we may be called back later when a completely different scope chain
    // is in effect.
    // We'll temporarily restore our own for the language engine to use.
    //

    m_pcc = COleScript::_CopyCallContext(m_prog->callcontext);
    assert(m_pcc);
}

DSobjDispatch::~DSobjDispatch()
{
#if LOG
    FuncLog(L"~DSobjDispatch()");
#endif
    // NULL'ing out the pointers helps the garbage collector

#if !defined linux
    m_pObj->com = NULL;
#endif
    m_pObj = NULL;

    nameTable.zero();

    m_prog = NULL;
    memset(m_pcc, 0, sizeof(CallContext));
    m_pcc = NULL;
}


STDMETHODIMP_(ULONG) DSobjDispatch::AddRef()
{
#if LOG
    FuncLog(L"DSobjDispatch::AddRef()");
#endif
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) DSobjDispatch::Release()
{
#if LOG
    FuncLog(L"DSobjDispatch::Release()");
#endif
    if (InterlockedDecrement(&m_cRef) == 0) {
       delete this;
  
       return 0;
    }

    return m_cRef;
}


STDMETHODIMP DSobjDispatch::QueryInterface(REFIID riid, void **ppv)
{
#if LOG
    FuncLog(L"DSobjDispatch::QueryInterface()");
#endif
    //WPRINTF(L"DSobjDispatch::QueryInterface(this = %x, ppv = %x)\n", this, ppv);
    if (NULL == ppv)
        return E_INVALIDARG;

    if (riid == IID_IUnknown ||
        riid == IID_IDispatch ||
        riid == IID_IDispatchEx)
    {
	//WPRINTF(L"IDispatchEx = %x\n", this);
        *ppv = static_cast<IDispatchEx*>(this);
	reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    }
    //
    // Need to do this cast separate in case
    // we ever multiply inherit to implement a second
    // interface -- that way we're sure we get the right VTable.
    //
    else if(riid == IID_DSobjDispatch)
    {
	//WPRINTF(L"DSobjDispatch = %x\n", this);
        *ppv = static_cast<DSobjDispatch*>(this);
	AddRef();
    }
    else
    {
        *ppv = NULL;

	//WPRINTF(L"E_NOINTERFACE\n");
        return E_NOINTERFACE;
    }


    return S_OK;
}

STDMETHODIMP DSobjDispatch::GetTypeInfoCount(unsigned *pctinfo)
{
#if LOG
    FuncLog(L"DSobjDispatch::GetTypeInfoCount()");
#endif
    if (NULL == pctinfo)
        return E_INVALIDARG;

    *pctinfo = 0;
    return S_OK;
}

STDMETHODIMP DSobjDispatch::GetTypeInfo(unsigned int iTInfo,
                                        LCID lcid,
                                        ITypeInfo** ppTInfo)
{
#if LOG
    FuncLog(L"DSobjDispatch::GetTypeInfo()");
#endif
    (void) iTInfo;
    (void) lcid;

    *ppTInfo = NULL;
    return DISP_E_BADINDEX;
} // GetTypeInfo

STDMETHODIMP DSobjDispatch::GetIDsOfNames(REFIID riid,
                                          OLECHAR** rgszNames,
                                          unsigned int cNames,
                                          LCID lcid,
                                          DISPID* rgDispId)
{   unsigned nCounter;
    HRESULT hResult = S_OK;

#if LOG
    FuncLog(L"DSobjDispatch::GetIDsOfNames()");
#endif
    mem.setStackBottom((void *)&rgDispId);

    for (nCounter = 0 ; nCounter < cNames; nCounter++)
    {
        DISPID dispid;
        OLECHAR *name;
        HRESULT h;

        name = rgszNames[nCounter];

#if LOG
        DPF(L"DSobjdispatch::GetIDsOfNames() : evaluating name \"%s\"\n", name);
#endif
        //
        // Get the names using our IDispatchEx functionality (but do it
        // case-insensitive because GetIDsOfNames() is on IDispatch).
        //
        h = GetDispID(name, fdexNameCaseInsensitive, &dispid);
        if (h != S_OK)
        {   dispid = DISPID_UNKNOWN;
            hResult = DISP_E_UNKNOWNNAME;
        }
        rgDispId[nCounter] = dispid;
    }

    //ENTRYLOG(L"DSobjdispatch::GetIDsOfNames() EXIT (hr = 0x%08lx)\n", hResult);
    return hResult;
} // GetIDsOfNames


STDMETHODIMP DSobjDispatch::Invoke(DISPID dispIdMember,
                                   REFIID riid,
                                   LCID lcid,
                                   WORD wFlags,
                                   DISPPARAMS* pDispParams,
                                   VARIANT* pVarResult,
                                   EXCEPINFO* pExcepInfo,
                                   unsigned int* puArgErr)
{
    unsigned argc;
    Value *arglist;
    unsigned counter;
    Dobject *othis;
    HRESULT hResult;

#if LOG
    FuncLog(L"DSobjDispatch::Invoke()");
#endif
    //WPRINTF(L"DSobjdispatch::Invoke(%i, 0x%04lx) ENTER\n", dispIdMember, wFlags);
    mem.setStackBottom((void *)&puArgErr);

    if (riid != IID_NULL)
        return DISP_E_UNKNOWNINTERFACE;

    if (wFlags & DISPATCH_PROPERTYPUTREF)
    {
        // makes no sense for dscript
	return E_UNEXPECTED;
    }
    if (wFlags & DISPATCH_PROPERTYPUT)
    {
        d_string name;
        Value value;
        Value *v;

        name = DISPIDToName(dispIdMember);
        if (!name)
        {
            //WPRINTF(L"DSobjdispatch::Invoke() ERROR : name not found in mapping for PropPut.\n");
            return DISP_E_MEMBERNOTFOUND;
        }

        // If there is a DISPID_THIS on a put, this check below is invalid
        argc = pDispParams->cArgs;
        if (argc != 1)
        {
            //WPRINTF(L"DSobjdispatch::Invoke() ERROR : too many named parameters for PropPut.\n");
            return DISP_E_BADPARAMCOUNT;
        }

        hResult = VariantToValue(&value, &(pDispParams->rgvarg[0]), d_string_ctor(L""), NULL);
        if (FAILED(hResult))
            return hResult;

        // This does a case-sensitive search, because that's how
	// ECMAScript is defined.
        // (IDispatch is case insensitive, though IDispatchEx can be sensitive).
        v = m_pObj->Put(name, &value, 0);
        if (v)
        {
            //WPRINTF(L"DSobjdispatch::Invoke() ERROR : PropPut call failed.\n");
            return DISP_E_MEMBERNOTFOUND;
        }

        return S_OK;
    }
    // Sometimes both METHOD and PROPERTYGET are set, which means do the METHOD.
    // Note: a PROPERTYGET with DISPID_VALUE is really an autonomous method dispatch.
    else if ((wFlags & (DISPATCH_PROPERTYGET | DISPATCH_METHOD)) == DISPATCH_PROPERTYGET)
    {
        if (dispIdMember == DISPID_VALUE)
        {
#if 0	// Wrong - this will cause infinite recursion
            if (pVarResult)
	    {
		VariantInit(pVarResult);
                V_VT(pVarResult) = VT_DISPATCH;
                V_DISPATCH(pVarResult) = this;
	    }
#else
            Value   valRet;
	    Value::copy(&valRet, &vundefined);
            m_pObj->DefaultValue(&valRet, TypeString);

            if (pVarResult)
                ValueToVariant(pVarResult, &valRet);
#endif

            return S_OK;
        }
        else
        {
            d_string name;
            Value *v;

            name = DISPIDToName(dispIdMember);
            if (!name)
            {
                //WPRINTF(L"DSobjdispatch::Invoke() ERROR : name not found in mapping for PropGet/Method.\n");
                return DISP_E_MEMBERNOTFOUND;
            }

            // This does a case sensitive lookup, per ECMAScript specs.
            v = m_pObj->Get(name);
            if (!v)
            {
                //WPRINTF(L"DSobjdispatch::Invoke() ERROR : Get() failed.\n");
                return DISP_E_MEMBERNOTFOUND;
            }

            if (pVarResult)
                ValueToVariant(pVarResult, v);

            return S_OK;
        }
    }
    else if (!(wFlags & (DISPATCH_PROPERTYGET | DISPATCH_METHOD | DISPATCH_CONSTRUCT)))
    {
        //WPRINTF(L"DSobjDispatch::Invoke(wflags = x%x)\n", wFlags);
        return DISP_E_MEMBERNOTFOUND;
    }

    // Get the 'othis' pointer from the named arguments
    switch (pDispParams->cNamedArgs)
    {
        case 0:
            othis = m_pObj;
            break;

        case 1:
        {
            if (wFlags & DISPATCH_CONSTRUCT)
                // Constructors don't have an othis
                return DISP_E_BADPARAMCOUNT;

            // We can accept one named argument,
            // which should be the 'othis' pointer
            if (pDispParams->rgdispidNamedArgs[0] != DISPID_THIS)
            {
                if (puArgErr)
                    *puArgErr = 0;              // arg 0 contains the error
                return DISP_E_PARAMNOTFOUND;
            }

            Value ovalue;
            Value *v;

            v = &ovalue;                        // so the toObject() call is polymorphic
            hResult = VariantToValue(v, &pDispParams->rgvarg[0], d_string_ctor(L""), NULL);
            if (hResult != S_OK)
                return hResult;
            othis = v->toObject();              // make sure toObject() is a polymorphic call
            break;
        }
        default:
            return DISP_E_BADPARAMCOUNT;
    }

    //
    // Prepare argument list built from all the unnamed arguments
    //

    arglist = NULL;
    argc = pDispParams->cArgs - pDispParams->cNamedArgs;
    if (0 != argc)
    {
        if (argc >= 32 || (arglist = (Value *)alloca(sizeof(Value) * argc)) == NULL)
        {
	    // Not enough stack space - use mem
	    arglist = (Value *)mem.malloc(sizeof(Value) * argc);
        }
    }
    for (counter = 0; counter < argc; counter++)
    {
        // Convert COM variants to DScript Values
        hResult = VariantToValue(&arglist[counter],
                &(pDispParams->rgvarg[pDispParams->cNamedArgs + argc - counter - 1]), d_string_ctor(L""), NULL);
        if (hResult != S_OK)
            return hResult;
    }

    //WPRINTF(L"DSobjdispatch::Invoke() : doing Call/Construct...\n");
    hResult = S_OK;
    __try
    {
        Value value;
        Value *ret;

        CallContext *ccSave = m_prog->callcontext;
	m_prog->callcontext = m_pcc;

        Program *program_save = Program::getProgram();
        Program::setProgram(m_prog);

	Value::copy(&value, &vundefined);
        if (wFlags & DISPATCH_CONSTRUCT)
            ret = (Value *)m_pObj->Construct(m_pcc, &value, argc, arglist);
        else
            ret = (Value *)m_pObj->Call(m_pcc, othis, &value, argc, arglist);

        if (ret)
        {
#if LOG
	    WPRINTF(L"DSobjdispatch::Invoke() ERROR : ret = %p, p->o = %p\n", ret, ret->object);
            WPRINTF(L"DSobjdispatch::Invoke() ERROR : Call/Construct failed! (\"%s\").\n", d_string_ptr(ret->toString()));
#endif
	    if (!m_pObj->DefaultValue(&value, TypeString))
	    {   
		if (pVarResult)
		    ValueToVariant(pVarResult, &value);
		hResult = S_OK;
	    }
	    else
		hResult = DISP_E_MEMBERNOTFOUND;
        }
        else
        {
            //
            // Convert return type to Variant
            //
            if (pVarResult)
                ValueToVariant(pVarResult, &value);
        }
	Program::setProgram(program_save);
	m_prog->callcontext = ccSave;
    }
    __except(EXCEPTION_CONTINUE_EXECUTION)
    {
#if LOG
	WPRINTF(L"DSobjdispatch::Invoke() EXCEPTION\n");
#endif
	// or do DISP_E_EXCEPTION
        hResult = DISP_E_MEMBERNOTFOUND;
    }

    return hResult;
} // Invoke


/* ------------------------- IDispatchEx ----------------------------- */

// The documentation for IDispatchEx, such as it is, is from Microsoft's
// dispex.idl file.
// We can only guess what much of this functionality is supposed to do.

/*********************************************
 * grfdex can contain any subset of the bits
 *      fdexNameCaseSensitive
 *      fdexNameEnsure          // add name if it is not there already
 *      fdexNameImplicit
 *      fdexNameCaseInsensitive
 *      fdexNameInternal
 *      fdexNameNoDynamicProperties
 */

STDMETHODIMP DSobjDispatch::GetDispID(
                /*in*/ BSTR bstrName,
                /*in*/ DWORD grfdex,
                /*out*/ DISPID *pid)
{
    DISPID  dispid;
    Value*  pVal = NULL;
    d_string name;

#if LOG
    FuncLog(L"DSobjDispatch::GetDispID()");
#endif
    //WPRINTF(L"DSobjdispatch::GetDispID(\"%s\", 0x%08lx) ENTER\n", bstrName, grfdex);
    mem.setStackBottom((void *)&pid);

    if (NULL == pid)
        return E_INVALIDARG;

    name = d_string_ctor(bstrName);
    pVal = m_pObj->Get(name);		// always case sensitive per ECMA
    if (NULL == pVal)
    {
        if (grfdex & fdexNameEnsure)
        {
            if (m_pObj->Put(name, &vundefined, 0))
            {
                DPF(L"DSobjdispatch::GetDispID() : ERROR: Put() failed while attempting to create fdexNameEnsure slot\n");
                return DISP_E_MEMBERNOTFOUND;
            }
        }
        else
        {
            DPF(L"DSobjdispatch::GetDispID() : ERROR: Property does not exist\n");
            return DISP_E_MEMBERNOTFOUND;
        }
    }

    dispid = nameToDISPID(name);
    if (dispid == 0)
    {
        //
        // DISPID was not previously mapped, so add it. Note this has
        // nothing to do with fdexNameEnsure -- it's just about whether or
        // not we've ever assigned a dynamic DISPID for this string name.
        // If we failed to find the prop and fdexNameEnsure was not set,
        // we would have already exited.
        //
        dispid = addName(name);
    }

    *pid = dispid;

    return S_OK;
}

/*********************************************
 * pvarRes, pei and pspCaller may be NULL.
 * When DISPATCH_METHOD is set in wFlags, there may be a "named
 *     parameter" for the "this" value. The dispID will be DISPID_THIS and
 *     it must be the first named parameter.
 * There is a new value for wFlags: DISPATCH_CONSTRUCT. This indicates
 *     that the item is being used as a constructor.
 * The legal values for wFlags are:
 *     DISPATCH_PROPERTYGET
 *     DISPATCH_METHOD
 *     DISPATCH_PROPERTYGET | DISPATCH_METHOD
 *     DISPATCH_PROPERTYPUT
 *     DISPATCH_PROPERTYPUTREF
 *     DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF
 *     DISPATCH_CONSTRUCT
 * IDispatchEx::Invoke should support the same values that
 *     IDispatchEx::InvokeEx supports (eg, DISPID_THIS, DISPATCH_CONSTRUCT).
 */

STDMETHODIMP DSobjDispatch::InvokeEx(
                /*in*/ DISPID id,
                /*in*/ LCID lcid,
                /*in*/ WORD wFlags,
                /*in*/ DISPPARAMS *pdp,
                /*out*/ VARIANT *pvarRes,       // Can be NULL.
                /*out*/ EXCEPINFO *pei, // Can be NULL.
                /*in, unique*/ IServiceProvider *pspCaller) // Can be NULL.
{
    // We don't know what pspCaller is for

#if LOG
    FuncLog(L"DSobjDispatch::InvokeEx()");
#endif
    return Invoke(id, IID_NULL, lcid, wFlags, pdp, pvarRes, pei, NULL);
}

/**************************************
 * (*) grfdex may contain fdexNameCaseSensitive or fdexNameCaseInsensitive
 * (*) If the member doesn't exist, return S_OK.
 * (*) If the member exists but can't be deleted, return S_FALSE.
 * (*) If the member is deleted, the DISPID still needs to be valid for
 *     GetNextDispID and if a member of the same name is recreated, the
 *     dispID should be the same.
 */

STDMETHODIMP DSobjDispatch::DeleteMemberByName(/*[in ]*/ BSTR bstrName,
                                               /*[in ]*/ DWORD grfdex)
{
    // Always case sensitive per ECMA

    d_string name;

    mem.setStackBottom((void *)&grfdex);
    name = d_string_ctor(bstrName);
    return m_pObj->Delete(name) ? S_OK : S_FALSE;
}


/***************************************
 * (*) If the member doesn't exist, return S_OK.
 * (*) If the member exists but can't be deleted, return S_FALSE.
 * (*) If the member is deleted, the DISPID still needs to be valid for
 *     GetNextDispID and if a member of the same name is recreated, the
 *     dispID should be the same.
 */

STDMETHODIMP DSobjDispatch::DeleteMemberByDispID(/*[in ]*/ DISPID id)
{
    d_string name;

    mem.setStackBottom((void *)&id);
    name = DISPIDToName(id);
    if (name)
        return m_pObj->Delete(name) ? S_OK : S_FALSE;
    return DISP_E_MEMBERNOTFOUND;
}

/************************************************
 * Return values (each is a separate bit):
 *      fdexPropCanGet
 *      fdexPropCannotGet
 *      fdexPropCanPut
 *      fdexPropCannotPut
 *      fdexPropCanPutRef
 *      fdexPropCannotPutRef
 *      fdexPropNoSideEffects           (*)
 *      fdexPropDynamicType             (*)
 *      fdexPropCanCall
 *      fdexPropCannotCall
 *      fdexPropCanConstruct
 *      fdexPropCannotConstruct
 *      fdexPropCanSourceEvents         (*)
 *      fdexPropCannotSourceEvents      (*)
 *
 *      (*) don't know what to do with these
 */

STDMETHODIMP DSobjDispatch::GetMemberProperties(
                /*[in ]*/ DISPID id,
                /*[in ]*/ DWORD grfdexFetch,
                /*[out]*/ DWORD *pgrfdex)
{
    d_string name;
    Property *p;
    DWORD result;

    mem.setStackBottom((void *)&pgrfdex);
    if (NULL == pgrfdex)
        return E_INVALIDARG;

    *pgrfdex = 0;

    name = DISPIDToName(id);
    if (name)
    {
        p = m_pObj->proptable.getProperty(name);
        if (p)
        {
            result = fdexPropCanGet |
                     fdexPropCanPut |
                     fdexPropCanCall |
                     fdexPropCanConstruct |
                     fdexPropCannotPutRef;

            if (p->attributes & ReadOnly)
                result ^= fdexPropCanPut | fdexPropCannotPut;

            Value *v = &p->value;
            if (v->isPrimitive())
            {
                result ^= fdexPropCanCall | fdexPropCannotCall |
                          fdexPropCanConstruct | fdexPropCannotConstruct;
            }
            else
            {
                // Assume the Dobject can call & construct
            }

            *pgrfdex = result & grfdexFetch;
            return S_OK;
        }
    }
    return DISP_E_MEMBERNOTFOUND;
}


STDMETHODIMP DSobjDispatch::GetMemberName(
                /*[in ]*/ DISPID id,
                /*[out]*/ BSTR *pbstrName)
{
    d_string name;

    mem.setStackBottom((void *)&pbstrName);
    if (NULL == pbstrName)
        return E_INVALIDARG;

    *pbstrName = NULL;

    name = DISPIDToName(id);
    if (name && m_pObj->Get(name))
    {
        // This is an [out] parameter, so create copy
        *pbstrName = SysAllocString(d_string_ptr(name));
        if (NULL == *pbstrName)
            return E_OUTOFMEMORY;

        return S_OK;
    }
    return DISP_E_MEMBERNOTFOUND;
}

/******************************
 * This function apparently is here
 * to support the javascript for-in statement.
 */

STDMETHODIMP DSobjDispatch::GetNextDispID(
                /*[in ]*/ DWORD grfdex,
                /*[in ]*/ DISPID idCur,
                /*[out]*/ DISPID *pidNext)
{   d_string name;
    Property *p;

    mem.setStackBottom((void *)&pidNext);
    if (NULL == pidNext)
        return E_INVALIDARG;

    DISPID  idNext = idCur;

    if (idNext == DISPID_STARTENUM)
    {
        // Initialize for enumeration by ensuring all the properties
        // have DISPID's
        Iterator iter(m_pObj);

        while (!iter.done())
        {
            p = iter.p;
            name = p->key.toString();
            if (!nameToDISPID(name))
                addName(name);
            iter.next();
        }
        idNext = DISPID_BASE - 1;
    }

    for (;;)
    {
        idNext++;                               // to next DISPID
        name = DISPIDToName(idNext);
        if (!name)
            return DISP_E_MEMBERNOTFOUND;

        p = m_pObj->proptable.getProperty(name);   // convert name to Property
        if (p && !(p->attributes & DontEnum))
            break;
    }

    *pidNext = idNext;
    return S_OK;
}


STDMETHODIMP DSobjDispatch::GetNameSpaceParent(/*[out]*/ IUnknown **ppunk)
{
    //
    // There's no documentation or examples for what this method
    // was intended to do.
    //
    (void)ppunk;

    return E_NOTIMPL;
}


/***********************************
 * Assign a DISPID to name.
 * Returns:
 *      DISPID that was assigned
 */

DISPID DSobjDispatch::addName(d_string name)
{
    mem.setStackBottom((void *)&name);
    nameTable.push(name);
    return nameTable.dim - 1 + DISPID_BASE;
}

/**********************************
 * Find DISPID associated with name.
 * Return 0 if not found.
 */

DISPID DSobjDispatch::nameToDISPID(d_string name)
{
    mem.setStackBottom((void *)&name);

    // PERF: This is a linear search - not very efficient.
    //       I don't think this will be a problem, however.

    for (unsigned i = 0; i < nameTable.dim; i++)
    {
        if (d_string_cmp(name, (d_string)nameTable.data[i]) == 0)
            return i + DISPID_BASE;
    }
    return 0;
}

/*****************************************
 * Convert DISPID to name.
 * Return NULL if not found.
 */

d_string DSobjDispatch::DISPIDToName(DISPID id)
{
    unsigned i;

    i = id - DISPID_BASE;
    if (i < 0 || i >= nameTable.dim)
        return NULL;
    return (d_string) nameTable.data[i];
}

/********************************************
 */

IDispatch* Dobject::toComObject()
{
#if defined linux
    IDispatch*  pdisp = NULL;
#else
    IDispatch*  pdisp = com;
#endif

    if (NULL == pdisp)
    {
        Program *p = Program::getProgram();

        assert(NULL != p);
        pdisp = new DSobjDispatch(this, p); // wrap object in a COM IDispatch interface
        if (NULL == pdisp)
        {
            return NULL;
        }
    }
    else
    {
        pdisp->AddRef();
    }

    return pdisp;
}



