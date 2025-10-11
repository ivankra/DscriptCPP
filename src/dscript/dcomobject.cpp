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


/*
 * COM objects are represented to dscript as Dcomobject's.
 */

#include <windows.h>

#if defined __DMC__
#include "dmc_rpcndr.h"
#endif

#include <objbase.h>
#include <activscp.h>
#include <dispex.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>

#include <math.h>
#undef exception

#include "port.h"
#include "root.h"
#include "dscript.h"
#include "value.h"
#include "date.h"
#include "dchar.h"
#include "dobject.h"
#include "mem.h"
#include "dcomobject.h"
#include "dcomlambda.h"
#include "text.h"
#include "dsobjdispatch.h"
#include "program.h"
#include "BaseDispatch.h"

#define LOG 0       // 0: disable logging, 1: enable it

#ifdef _MSC_VER
#pragma warning(disable: 4127)      // Caused by if (LOG)
#endif // _MSC_VER

void DoFinalize(void* pObj, void* pClientData);

/* For reference (wtypes.h):
	VT_EMPTY        = 0,
	VT_NULL		= 1,
	VT_I2		= 2,
	VT_I4		= 3,
	VT_R4		= 4,
	VT_R8		= 5,
	VT_CY		= 6,
	VT_DATE		= 7,
	VT_BSTR		= 8,
	VT_DISPATCH     = 9,
	VT_ERROR        = 10,
	VT_BOOL		= 11,
	VT_VARIANT      = 12,
	VT_UNKNOWN      = 13,
	VT_DECIMAL      = 14,
	VT_I1		= 16,
	VT_UI1		= 17,
	VT_UI2		= 18,
	VT_UI4		= 19,
	VT_I8		= 20,
	VT_UI8		= 21,
	VT_INT		= 22,
	VT_UINT		= 23,
	VT_VOID		= 24,
	VT_HRESULT      = 25,
	VT_PTR		= 26,
	VT_SAFEARRAY    = 27,
	VT_CARRAY       = 28,
	VT_USERDEFINED  = 29,
	VT_LPSTR        = 30,
	VT_LPWSTR       = 31,
	VT_RECORD       = 36,
	VT_FILETIME     = 64,
	VT_BLOB		= 65,
	VT_STREAM       = 66,
	VT_STORAGE      = 67,
	VT_STREAMED_OBJECT      = 68,
	VT_STORED_OBJECT        = 69,
	VT_BLOB_OBJECT  = 70,
	VT_CF		= 71,
	VT_CLSID        = 72,
	VT_BSTR_BLOB    = 0xfff,
	VT_VECTOR       = 0x1000,
	VT_ARRAY        = 0x2000,
	VT_BYREF        = 0x4000,
	VT_RESERVED     = 0x8000,
	VT_ILLEGAL      = 0xffff,
	VT_ILLEGALMASKED        = 0xfff,
	VT_TYPEMASK     = 0xfff
 */

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Dcomobject()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
Dcomobject::Dcomobject(d_string name, IDispatch *pDisp, Dcomobject *pParent)
    : Dobject(Dobject::getPrototype())
{
#if LOG
    WPRINTF(L"new Dcomobject('%ls', %p, %p)\n", d_string_ptr(name), pDisp, pParent);
#endif
    isLambda = 0;
    this->objectname = name;

    m_pDisp = pDisp;
    m_pDispEx = NULL;

    if (NULL != m_pDisp)
    {
        HRESULT hResult = E_UNEXPECTED;

        m_pDisp->AddRef();

        // Prefer the IDispatchEx interface
        //
        hResult = m_pDisp->QueryInterface(IID_IDispatchEx, (void **)&m_pDispEx);
        if (SUCCEEDED(hResult))
        {
#if LOG
            WPRINTF(L"m_pDispEx = %p\n", m_pDispEx);
#endif
        }
    }

    m_pParent = pParent;
} // Dcomobject


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// ~Dcomobject()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
Dcomobject::~Dcomobject()
{
#if LOG
    WPRINTF(L"~Dcomobject(%p)\n", this);
#endif
    finalizer();
//    objectname = d_string_ctor(L"");
    m_pParent = NULL;
}

#if INVARIANT

void Dcomobject::invariant()
{
}

#endif

void * Dcomobject::operator new(size_t m_size)
{
    void *p = Dobject::operator new(m_size);

    if (p)
        mem.setFinalizer(p, DoFinalize, NULL);
    return p;
}

void Dcomobject::finalizer()
{
    //WPRINTF(L"Dcomobject::finalizer(m_pDispEx = %x, m_pDisp = %x)\n", m_pDispEx, m_pDisp);
    if (m_pDispEx) {
	m_pDispEx->Release();
        m_pDispEx= NULL;
    }
    if (m_pDisp) {
	m_pDisp->Release();
        m_pDisp = NULL;
    }
}

/*************************************************
 * This gets called by the garbage collector when
 * a Dcomobject or Dcomlambda object gets free'd.
 * Be careful to NOT make ANY calls into the GC
 * from this function or any functions it calls.
 * We use it to get the Release()'s called on any COM objects
 * referenced by the Dcomobject or Dcomlambda object.
 */

void DoFinalize(void* pObj, void* pClientData)
{
    (void) pClientData;

    Dobject *o = (Dobject *)pObj;
#if DYNAMIC_CAST
    Dcomobject *dco = dynamic_cast<Dcomobject *>(o);
    if (dco)
	dco->finalizer();
    Dcomlambda *dlo = dynamic_cast<Dcomlambda *>(o);
    if (dlo)
	dlo->finalizer();
    Denumerator *den = dynamic_cast<Denumerator *>(o);
    if (den)
	den->finalizer();
    Dvariant *dv = dynamic_cast<Dvariant *>(o);
    if (dv)
	dv->finalizer();
#else
    // dynamic_cast doesn't work for GCC
    Dcomobject *dco = (Dcomobject *)o;
    switch (dco->isLambda)
    {
	case 1:
	    Dcomlambda *dlo = (Dcomlambda *)o;
	    if (dlo)
		dlo->finalizer();
	    break;
	case 0:
	    if (dco)
		dco->finalizer();
	    break;
	case 2:
	    Denumerator *den = (Denumerator *)(o);
	    if (den)
		den->finalizer();
	    break;
	case 3:
	    Dvariant *dv = (Dvariant *)(o);
	    if (dv)
		dv->finalizer();
	    break;
    }
#endif
}


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _GetDispID()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT Dcomobject::_GetDispID(LCID lcid, dchar* pwszName, DWORD grfdex, DISPID* pID)
{
    HRESULT hr = E_UNEXPECTED;

#if LOG
    WPRINTF(L"_GetDispID('%s')\n", pwszName ? pwszName : L"(null)");
#endif
    if (!pwszName)
    {	*pID = DISPID_VALUE;
	hr = S_OK;
    }
    else if (NULL != m_pDispEx)
    {
        BSTR bszName = SysAllocString(pwszName);
        if (NULL == bszName)
            return E_OUTOFMEMORY;

        hr = m_pDispEx->GetDispID(bszName, grfdex, pID);
        SysFreeString(bszName);
    }
    else
    {
        assert(NULL != m_pDisp);
        hr = m_pDisp->GetIDsOfNames(IID_NULL,
                                    &pwszName,
                                    1,
                                    lcid,
                                    pID);
    }

    return hr;
} // _GetDispID


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _GetNameAndInvoke()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT Dcomobject::_GetNameAndInvoke(LCID lcid, dchar* pwszName, DWORD grfdex,
                                      WORD wInvokeFlags, DISPPARAMS* pDP,
                                      VARIANT* pvarResult, EXCEPINFO* pEI)
{
    assert(0 != wInvokeFlags && NULL != pDP);

    HRESULT hr = E_UNEXPECTED;
    DISPID  id = DISPID_UNKNOWN;
    UINT    uArgError;      // Scratch variable -- just to get a result

    hr = _GetDispID(lcid, pwszName, grfdex, &id);

    if (FAILED(hr))
    {
#if LOG
        WPRINTF(L"'%ls'._GetNameAndInvoke : GetDispID('%ls') failed with x%x\n", d_string_ptr(objectname), pwszName, hr);
#endif
        return hr;
    }

    if (NULL != m_pDispEx)
    {
        hr = m_pDispEx->InvokeEx(id,
                                 lcid,
                                 wInvokeFlags,
                                 pDP,
                                 pvarResult,
                                 pEI,
                                 NULL);         // IServiceProvider* pspCaller
    }
    else
    {
        hr = m_pDisp->Invoke(id,
                             IID_NULL,
                             lcid,
                             wInvokeFlags,
                             pDP,               // DISPPARAMS
                             pvarResult,        // Return result variant
                             pEI,               // EXCEPINFO*
                             &uArgError);       // puArgError
    }

    return hr;
} // _GetNameAndInvoke


Value *Dcomobject::GetLambda(d_string PropertyName, unsigned hash)
{
    HRESULT hr = E_UNEXPECTED;
    DISPID  id;
    Value*  pvalLocal = NULL;
    dchar*  pname = d_string_ptr(PropertyName);

#if LOG
    WPRINTF(L"%x.GetLambda('%s')\n", this, pname);
#endif

    //
    // First check to see if the property exists in the local scope
    // (this generally will only be the case for Dcomobjects that are
    // wrapping Named Item namespaces, since that's the only way things
    // get Put() into the local property table...).
    //
    pvalLocal = Dobject::Get(PropertyName, hash);
    if (NULL != pvalLocal)
    {
#if LOG
	WPRINTF(L"\tit is local\n");
#endif
        return pvalLocal;
    }

    hr = _GetDispID(LCID_ENGLISH_US, pname, fdexNameCaseSensitive, &id);

    if (FAILED(hr))
    {
#if LOG
        WPRINTF(DTEXT("\tDcomobject::GetLambda() failed, hResult = x%x\n"), hr);
#endif
        return NULL;
    }

#if LOG
    WPRINTF(L"\tdispid = %x\n", id);
#endif

    GC_LOG();
    Dobject* pObj = new Dcomlambda(id, m_pDisp, m_pDispEx, this);
    GC_LOG();
    return new Vobject(pObj);
}


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Get()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
Value *Dcomobject::Get(d_string PropertyName, unsigned hash)
{
    HRESULT hResult = E_UNEXPECTED;
    dchar* pname = d_string_ptr(PropertyName);

    if (LOG) WPRINTF(DTEXT("'%ls'.Dcomobject::Get('%ls')\n"), d_string_ptr(objectname), pname);

    //
    // First check to see if the property exists in the local scope
    // (this generally will only be the case for Dcomobjects that are
    // wrapping Named Item namespaces, since that's the only way things
    // get Put() into the local property table...).
    //
    Value*  pvalLocal = Dobject::Get(PropertyName, hash);
    if (NULL != pvalLocal)
        return pvalLocal;

    DISPPARAMS  dispParams;
    EXCEPINFO   ExcepInfo;
    VARIANT     varResult;
    Value       value;

    memset(&dispParams, 0, sizeof(dispParams));
    memset(&ExcepInfo, 0, sizeof(ExcepInfo));
    VariantInit(&varResult);

    //
    // See if there's an item name for this dispatch object with PropertyName
    //
    hResult = _GetNameAndInvoke(LCID_ENGLISH_US, pname, fdexNameCaseSensitive,
                                DISPATCH_PROPERTYGET,
                                &dispParams, &varResult, NULL /*&ExcepInfo*/);

    if (FAILED(hResult))
    {
        if (LOG) WPRINTF(DTEXT("failed to [[Get]] %ls, hResult = x%x\n"), pname, hResult);
	//ErrInfo errinfo;
	//return RuntimeError(&errinfo, ERR_GET_FAILED, pname);
	return NULL;
    }
    else
    {
        VariantToValue(&value, &varResult, PropertyName, this);
    }

    Value* pVal = new(this) Value();
    Value::copy(pVal, &value);

    return pVal;
}

Value *
Dcomobject::Get(d_string PropertyName)
{
    return Get(PropertyName, d_string_hash(PropertyName));
}

Value *
Dcomobject::Get(d_uint32 index)
{
    dchar buffer[sizeof(index)*3+1];

#if LOG
    WPRINTF(L"Dcomobject::Get(%u)\n", index);
#endif
    Port::itow(index, buffer, 10);
    return Get(Dstring::dup(this, buffer));
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Put()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
Value *
Dcomobject::Put(d_string PropertyName, Value *pValue, unsigned attributes)
{
    HRESULT     hResult = E_UNEXPECTED;
    DISPPARAMS  dispParams;
    EXCEPINFO   ei;
    VARIANT     varValue;
    DISPID      idPropPut = DISPID_PROPERTYPUT;
    dchar*      pname = d_string_ptr(PropertyName);

#if LOG
        WPRINTF(DTEXT("'%ls'.Dcomobject::Put('%ls') = '%ls'\n"),
                d_string_ptr(objectname), pname, d_string_ptr(pValue->toString()));
#endif

    //
    // If we're being told to instantiate the property, then we should put it
    // in the local proptable, not on the object itself. Otherwise the other
    // case where we use the proptable isntead of the object is when the property
    // exists there (which means we've instantiated it previously).
    //
    if ((Instantiate & attributes) || Dobject::HasProperty(PropertyName))
    {
        return Dobject::Put(PropertyName, pValue, (attributes & ~Instantiate));
    }

    // clear out excepinfo
    memset(&ei, 0, sizeof(ei));

    // Convert Value to vt type
    ValueToVariant(&varValue, pValue);

    // Build the DISPPARAMS
    dispParams.rgdispidNamedArgs = &idPropPut;
    dispParams.cNamedArgs = 1;
    dispParams.cArgs = 1;
    dispParams.rgvarg = &varValue;

    WORD    wFlags = DISPATCH_PROPERTYPUT;

    if (V_VT(&varValue) == VT_DISPATCH)
        wFlags |= DISPATCH_PROPERTYPUTREF;

    hResult = _GetNameAndInvoke(LCID_ENGLISH_US,
				pname, fdexNameCaseSensitive | fdexNameEnsure,
                                wFlags, &dispParams, NULL, &ei);
    VariantClear(&varValue);

    if (FAILED(hResult))
    {
#if LOG
        WPRINTF(DTEXT("failed to [[Put]] %ls, error code x%x\n"), pname, hResult);
#endif
        Value* pvalErr = NULL;
	ErrInfo errinfo;
        if (DISP_E_EXCEPTION == hResult && ei.bstrDescription)
        {
            pvalErr = RuntimeError(&errinfo, ERR_S_S,
                                   ei.bstrSource ? ei.bstrSource : L"",
                                   ei.bstrDescription);
        }
        else
        {   int errnum = hresultToMsgnum(hResult, ERR_PUT_FAILED);
	    pvalErr = RuntimeError(&errinfo, errnum, pname);
        }
        SysFreeString(ei.bstrSource);
        SysFreeString(ei.bstrDescription);
        SysFreeString(ei.bstrHelpFile);
        memset(&ei, 0, sizeof(ei));
	return pvalErr;
    }

    Value *pVal = new(this) Value();
    Value::copy(pVal, pValue);
    return NULL;
} // Put


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
Value *
Dcomobject::Put(d_string PropertyName, Dobject *o, unsigned attributes)
{   Vobject v(o);
    return Put(PropertyName, &v, attributes);
} // Put

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
Value *
Dcomobject::Put(d_string PropertyName, d_number n, unsigned attributes)
{   Vnumber v(n);

    return Put(PropertyName, &v, attributes);
} // Put

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
Value *
Dcomobject::Put(d_string PropertyName, d_string s, unsigned attributes)
{   Vstring v(s);

    return Put(PropertyName, &v, attributes);
} // Put

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
Value *
Dcomobject::Put(d_uint32 index, Value *value, unsigned attributes)
{
#if LOG
    WPRINTF(L"Dcomobject::Put(index = %d)\n", index);
#endif
    dchar buffer[sizeof(index) * 3 + 1];

    Port::itow(index, buffer, 10);
    return Put(Dstring::dup(this, buffer), value, attributes);
} // Put


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// PutDefault()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
Value *
Dcomobject::PutDefault(Value *pValue)
{
    HRESULT     hResult = E_UNEXPECTED;
    DISPPARAMS  dispParams;
    EXCEPINFO   ei;
    EXCEPINFO   *pei = NULL; /*&ei;*/
    VARIANT     varValue;
    DISPID      idPropPut = DISPID_PROPERTYPUT;

#if LOG
    WPRINTF(DTEXT("'%ls'.Dcomobject::PutDefault() = '%ls'\n"),
	    d_string_ptr(objectname), d_string_ptr(pValue->toString()));
#endif

    // clear out excepinfo
    memset(&ei, 0, sizeof(ei));

    // Convert Value to vt type
    ValueToVariant(&varValue, pValue);

    // Build the DISPPARAMS
    dispParams.rgdispidNamedArgs = &idPropPut;
    dispParams.cNamedArgs = 1;
    dispParams.cArgs = 1;
    dispParams.rgvarg = &varValue;

    WORD    wFlags = DISPATCH_PROPERTYPUT;

    if (V_VT(&varValue) == VT_DISPATCH)
        wFlags |= DISPATCH_PROPERTYPUTREF;

    if (NULL != m_pDispEx)
    {
        hResult = m_pDispEx->InvokeEx(DISPID_VALUE,
                                 LCID_ENGLISH_US,
                                 wFlags,
                                 &dispParams,
                                 NULL,
                                 pei,
                                 NULL);         // IServiceProvider* pspCaller
    }
    else
    {
        hResult = m_pDisp->Invoke(DISPID_VALUE,
                             IID_NULL,
                             LCID_ENGLISH_US,
                             wFlags,
                             &dispParams,       // DISPPARAMS
                             NULL,              // Return result variant
                             pei,               // EXCEPINFO*
                             NULL);             // puArgError
    }

    VariantClear(&varValue);

    if (FAILED(hResult))
    {
        // if (LOG) WPRINTF(DTEXT("failed to [[PutDefault]] %ls, error code x%x\n"), pname, hResult);
        return NULL;
    }

    Value *pVal = new(this) Value();
    Value::copy(pVal, pValue);
    return pVal;
} // PutDefault


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// CanPut()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
int Dcomobject::CanPut(d_string PropertyName)
{
    HRESULT hResult;
    DISPID dispid;
    DWORD grfdex;

    if (Dobject::HasProperty(PropertyName))
    {
        return Dobject::CanPut(PropertyName);
    }

    if (m_pDispEx)
    {
        BSTR bszName = SysAllocString(d_string_ptr(PropertyName));
        if (NULL == bszName)
            return FALSE;
        hResult = m_pDispEx->GetDispID(bszName,
                                       fdexNameCaseSensitive,
                                       &dispid);
        SysFreeString(bszName);
        if (SUCCEEDED(hResult))
        {
            grfdex = 0;
            hResult = m_pDispEx->GetMemberProperties(dispid,
                                                     fdexPropCanPut,
                                                     &grfdex);

            if (SUCCEEDED(hResult) && (grfdex & fdexPropCanPut) == 0)
                return FALSE;
        }
    }
    else
    {
        // Assume it will work
    }
    if (internal_prototype)
        return internal_prototype->CanPut(PropertyName);

    return TRUE;
} // CanPut


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// HasProperty()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
int Dcomobject::HasProperty(d_string PropertyName)
{
    HRESULT hResult;
    DISPID  id;

    //
    // See if there's an item name for this dispatch object with PropertyName
    //
    hResult = _GetDispID(LCID_ENGLISH_US, d_string_ptr(PropertyName), fdexNameCaseSensitive, &id);

    if (SUCCEEDED(hResult))
        return TRUE;
    if (internal_prototype)
        return internal_prototype->HasProperty(PropertyName);

    return FALSE;
} // HasProperty


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Delete()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
int Dcomobject::Delete(d_string PropertyName)
{
    HRESULT hResult;

    if (Dobject::HasProperty(PropertyName))
    {
        return Dobject::Delete(PropertyName);
    }

    if (m_pDispEx)
    {
        BSTR    bszName = SysAllocString(d_string_ptr(PropertyName));
        if (NULL == bszName)
            return FALSE;
        hResult = m_pDispEx->DeleteMemberByName(bszName,
                                                fdexNameCaseSensitive);
        SysFreeString(bszName);

        //
        // Specifically compare to S_OK, which means successfully deleted. Anything else is failure.
        //
        if (S_OK == hResult)
            return TRUE;
    }

    // Can't delete it
    return FALSE;
} // Delete


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Delete()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
int Dcomobject::Delete(d_uint32 index)
{
    dchar buffer[sizeof(index) * 3 + 1];

    Port::itow(index, buffer, 10);
    return Delete(Dstring::dup(this, buffer));
} // Delete


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// implementsDelete()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
int Dcomobject::implementsDelete()
{
    // Delete is implemented if there's an IDispatchEx interface

    return (m_pDispEx != NULL);
} // implementsDelete


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// DefaultValue
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void *Dcomobject::DefaultValue(Value *ret, dchar *Hint)
{
    //
    // The behavior of the [[DefaultValue]]
    // method is defined by this specification for all
    // native ECMAScript objects (see section 8.6.2.6).
    // "Host objects" are left more or less undefined.
    // We simply try to coerce the return of a Call()
    // (Invoke on DISPID_VALUE) to a string or number.
    //
#if LOG
    WPRINTF(L"Dcomobject::DefaultValue(ret = %x, Hint = '%s')\n", ret, Hint ? Hint : DTEXT(""));
#endif

    if (NULL != m_pDisp)
    {
        void*           a;
        CallContext*    cc;

        Value::copy(ret, &vundefined);
        cc = Program::getProgram()->callcontext;
        a = (void*)Call(cc, this, ret, 0, NULL);
        if (a)                  // if exception was thrown, then bail
            return a;

        if (ret)
            return NULL;

        Vstring::putValue(ret, classname);
	ErrInfo errinfo;
        return RuntimeError(&errinfo, ERR_COM_NO_DEFAULT_VALUE);
    }
    else
    {
        Value::copy(ret, &vnull);
        return NULL;
    }
} // DefaultValue


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Construct()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void *Dcomobject::Construct(CONSTRUCT_ARGS)
{
    HRESULT hResult = E_UNEXPECTED;
    DISPID dispid;
    DISPPARAMS dp;
    VARIANTARG *pvarg = 0;
    VARIANT vresult;
    EXCEPINFO ei;
    EXCEPINFO *pei = NULL; // &ei;
    unsigned argerr = 0;
    unsigned counter;

    if (LOG) WPRINTF(DTEXT("'%ls'.Dcomobject::Construct()\n"), d_string_ptr(objectname));
    assert(m_pDisp);

    Value::copy(ret, &vundefined);

    dispid = DISPID_VALUE;

    // clear out excepinfo
    memset(&ei, 0, sizeof(ei));

    // Allocate for the variants
    if (0 != argc)
    {
        if (argc >= 32 || (pvarg = (VARIANTARG *)alloca(sizeof(VARIANTARG) * argc)) == NULL)
        {
	    // Not enough stack space - use mem
	    GC_LOG();
	    pvarg = (VARIANTARG *)mem.malloc(sizeof(VARIANTARG) * argc);
        }
    }

    // Set up the dispparams
    dp.rgdispidNamedArgs = 0;
    dp.cNamedArgs = 0;
    dp.cArgs = argc;
    dp.rgvarg = pvarg;

    // March through the parameter list and build up the variant array
    for (counter = 0; counter < argc; counter++)
    {
        // Mutate dscript types to variant types
        ValueToVariant(&(pvarg[argc - counter - 1]),&(arglist[counter]));
    }

    // Do the call
    VariantInit(&vresult);

    if (LOG) WPRINTF(DTEXT("Dcomobject::Construct(), Invoke()"));

    if (m_pDispEx)
    {
        hResult = m_pDispEx->InvokeEx(dispid,
                                      LCID_ENGLISH_US,
                                      DISPATCH_CONSTRUCT,
                                      &dp,
                                      &vresult,
                                      pei,
                                      NULL);
    }
    else
    {
        hResult = m_pDisp->Invoke(dispid,
                                  IID_NULL,
                                  LCID_ENGLISH_US,
                                  DISPATCH_METHOD,
                                  &dp,
                                  &vresult,
                                  pei,
                                  &argerr);
    }
    if (LOG) WPRINTF(DTEXT("Dcomobject::Construct(): Invoke() returned x%x\n"), hResult);
    if (pvarg)
    {
        for (counter = 0; counter < argc; counter++)
        {
            VariantClear(&pvarg[counter]);
        }
    }

    if (FAILED(hResult))
    {
	ErrInfo errinfo;

	int errnum = hresultToMsgnum(hResult, ERR_COM_NO_CONSTRUCT_PROPERTY);
	errinfo.code = 445;
	return RuntimeError(&errinfo, errnum, d_string_ptr(objectname));
    }

    // Convert return type to return value
    VariantToValue(ret, &vresult, d_string_ctor(L""), NULL);

    // Cleanup for the variant return type
    VariantClear(&vresult);

    return NULL;
}

/*************************************************
 */

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// dcomobject_isNull()
//
// Shortcut way to test a Dcomobject for Null.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
d_boolean dcomobject_isNull(Dcomobject* pObj)
{
    return (NULL == pObj && NULL == pObj->m_pDisp);
}  // dcomobject_isNull


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// dcomobject_areEqual()
//
// Tests two Dcomobject's for all possible types
// of equality (if the wrappers are equal, the
// contained objects definitely are, otherwise
// we have to QI for IUnknown on both objects and
// compare the result for equality by COM definition.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
d_boolean dcomobject_areEqual(Dcomobject* pLeft, Dcomobject* pRight)
{
    if (pLeft == pRight)
        return true;
    else if (NULL == pLeft || NULL == pRight)
        return false;

    IUnknown*   punkLeft = NULL;
    IUnknown*   punkRight = NULL;
    bool        fEqual = false;

    if (NULL == pLeft->m_pDisp && NULL == pRight->m_pDisp)
        return true;
    if (NULL == pLeft->m_pDisp || NULL == pRight->m_pDisp)
        return false;

    pLeft->m_pDisp->QueryInterface(IID_IUnknown, (void**)&punkLeft);
    pRight->m_pDisp->QueryInterface(IID_IUnknown, (void**)&punkRight);

    fEqual = (punkLeft == punkRight);
    punkLeft->Release();
    punkRight->Release();

    return fEqual;
} // dcomobject_areEqual


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Call()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void *Dcomobject::Call(CALL_ARGS)
{
    //WPRINTF(L"Dcomobject::Call()\n");

    if (m_pDisp)
    {
	return Call2(NULL, cc,othis,ret,argc,arglist);
    }
    else
    {
	assert(m_pParent);
	return m_pParent->Call2(objectname, cc,othis,ret,argc,arglist);
    }
}

void *dcomobject_call(d_string s, CALL_ARGS)
{
    return ((Dcomobject *)othis)->Call2(s, cc,othis,ret,argc,arglist);
}

void *Dcomobject::Call2(d_string s, CALL_ARGS)
{
    HRESULT hResult;
    DISPID dispid = DISPID_UNKNOWN;
    DISPPARAMS dp;
    VARIANTARG *pvarg = 0;
    VARIANT vresult;
    EXCEPINFO ei;
    unsigned argerr = 0;
    unsigned counter;
    int dothis;
    DISPID thisdispid;
    dchar *pobjname = s ? d_string_ptr(s) : NULL;

    if (LOG && s) WPRINTF(DTEXT("'%ls'.dcomobject_call()\n"), pobjname);

    Value::copy(ret, &vundefined);

    if (NULL == m_pDisp && NULL == m_pDispEx)
    {
        if (LOG) WPRINTF(DTEXT("Call : Unable to find something to call.\n"));
        return NULL;
    }

    hResult = _GetDispID(cc->prog->lcid, pobjname, fdexNameCaseSensitive, &dispid);
    if (FAILED(hResult))
    {
	if (LOG) WPRINTF(L"GetDispID('%ls') failed with x%x\n", pobjname, hResult);
	ErrInfo errinfo;
	int errnum = hresultToMsgnum(hResult, ERR_NO_PROPERTY);
	return RuntimeError(&errinfo, errnum, pobjname);
    }

    // Support for 'othis' parameter is only in IDispatchEx
    dothis = (othis && m_pDispEx != NULL) ? 1 : 0;

    // clear out excepinfo
    memset(&ei, 0, sizeof(ei));

    // Allocate for the variants
    if (0 != (dothis + argc))
    {	int ac = dothis + argc;

        if (ac >= 32 || (pvarg = (VARIANTARG *)alloca(sizeof(VARIANTARG) * ac)) == NULL)
        {
	    // Not enough stack space - use mem
	    GC_LOG();
	    pvarg = (VARIANTARG *)mem.malloc(sizeof(VARIANTARG) * ac);
        }
	if (LOG) WPRINTF(L"pvarg = %x, dothis = %d, argc = %d\n", pvarg, dothis, argc);
    }

    // Set up the dispparams
    // When DISPATCH_METHOD is set in wFlags, there may be a "named
    // parameter" for the "this" value. The dispID will be DISPID_THIS and
    // it must be the first named parameter.
    // For how the DISPPARAMS is set up, see:
    //	http://msdn.microsoft.com/library/books/inole/S1227.htm

    dp.rgdispidNamedArgs = NULL;
    dp.cNamedArgs = dothis;
    dp.cArgs = dp.cNamedArgs + argc;
    dp.rgvarg = pvarg;

    if (dothis)
    {
        thisdispid = DISPID_THIS;
        dp.rgdispidNamedArgs = &thisdispid;
        ValueToVariant(&pvarg[0], &othis->value);   // first named parameter
    }

    // Loop through the argument list and build up the variant array
    // Note that arguments are stored right to left in pvarg[]
    // following the named parameters.
    for (counter = 0; counter < argc; counter++)
    {
        // Mutate dscript types to variant types
        ValueToVariant(&(pvarg[dothis + argc - counter - 1]),&(arglist[counter]));
    }

    // Do the call
    VariantInit(&vresult);

    if (LOG) WPRINTF(DTEXT("Dcomobject::Call(): Invoke()\n"));
    if (m_pDispEx)
    {
        hResult = m_pDispEx->InvokeEx(dispid,
                                    cc->prog->lcid,
                                    DISPATCH_METHOD | DISPATCH_PROPERTYGET,
                                    &dp,
                                    &vresult,
                                    &ei,
                                    NULL);

        //
        // Some things don't like the DISPID_THIS, so if we get a bad param
        // count error, just try again without a this pointer...
        //
        if (DISP_E_BADPARAMCOUNT == hResult)
        {
            VariantClear(&vresult);
            SysFreeString(ei.bstrSource);
            SysFreeString(ei.bstrDescription);
            SysFreeString(ei.bstrHelpFile);
            memset(&ei, 0, sizeof(ei));

            //
            // Reset the DISPPARAMS and don't pass a DISPID_THIS
            //
            dp.rgdispidNamedArgs = NULL;
            dp.cNamedArgs = 0;
            dp.cArgs = argc;
            dp.rgvarg = dothis ? &pvarg[1] : &pvarg[0];

            hResult = m_pDispEx->InvokeEx(dispid,
                                        cc->prog->lcid,
                                        DISPATCH_METHOD | DISPATCH_PROPERTYGET,
                                        &dp,
                                        &vresult,
                                        &ei,
                                        NULL);
        }
    }
    else
    {
        // The reason to set DISPATCH_PROPERTYGET is because of a behavior
        // exhibited by some applications like Excel:
        //
        //   http://support.microsoft.com/support/kb/articles/Q165/2/73.ASP
        // For more info:
        //   http://msdn.microsoft.com/library/default.asp?URL=/library/psdk/automat/chap5_8gfn.htm
        //
        // Basically, there can be either methods or propgets that return an object. Which
        // one gets used is entirely up to the implementor of the Automation object in question.
        //
        // Thanks to Paul for figuring this out.

        hResult = m_pDisp->Invoke(dispid,
                                    IID_NULL,
                                    cc->prog->lcid,
                                    DISPATCH_METHOD | DISPATCH_PROPERTYGET,
                                    &dp,
                                    &vresult,
                                    &ei,
                                    &argerr);
    }
    if (LOG) WPRINTF(DTEXT("Dcomobject::Call(): Invoke() returned %x\n"), hResult);

    if (DISP_E_BADVARTYPE == hResult)
    {
	if (LOG)
	{
	    WPRINTF(DTEXT("Bad Variant Type returned for Invoke on %s\n"), pobjname);
	    WPRINTF(DTEXT("dp.cArgs %d\n"), dp.cArgs);
	    for (unsigned int kk = 0; kk < dp.cArgs; kk++)
		WPRINTF(DTEXT("    dp.rgvarg[%d].vt = %d\n"), kk, dp.rgvarg[kk].vt);
	}
    }

    if (pvarg)
    {
        for (counter = 0; counter < argc + dothis; counter++)
        {
            VariantClear(&pvarg[counter]);
        }
    }

    if (FAILED(hResult))
    {
        Value* pvalErr = NULL;
	ErrInfo errinfo;

        if (DISP_E_EXCEPTION == hResult && ei.bstrDescription)
        {
            pvalErr = RuntimeError(&errinfo, ERR_S_S,
                                   ei.bstrSource ? ei.bstrSource : L"",
                                   ei.bstrDescription);
        }
	else
	{   int errnum;

	    switch (hResult)
	    {
		case DISP_E_BADPARAMCOUNT:
		    errnum = ERR_DISP_E_BADPARAMCOUNT;
		    break;
		case DISP_E_TYPEMISMATCH:
		    errnum = ERR_DISP_E_TYPEMISMATCH;
		    break;
		default:
		    errnum = hresultToMsgnum(hResult, ERR_COM_FUNCTION_ERROR);
		    break;
	    }
	    pvalErr = RuntimeError(&errinfo,
		    errnum, pobjname ? pobjname : L"", hResult);
	}

        SysFreeString(ei.bstrSource);
        SysFreeString(ei.bstrDescription);
        SysFreeString(ei.bstrHelpFile);
        memset(&ei, 0, sizeof(ei));

	//PRINTF("Returning pvalErr = %p, p->o = %p\n", pvalErr, pvalErr->object);
	//WPRINTF(L"Call/Construct failed! (\"%s\").\n", d_string_ptr(pvalErr->toString()));
        return pvalErr;
    }

    // Convert return type to return value
    VariantToValue(ret, &vresult, d_string_ctor(L""), NULL);

    // Cleanup for the variant return type
    VariantClear(&vresult);

    return NULL;
} // Call

/****************************************************
 * This is the implementation of:
 *	Session("property") = object;
 */

Value *Dcomobject::put_Value(Value *ret, unsigned argc, Value *arglist)
{
    HRESULT hResult;
    DISPID dispid = DISPID_VALUE;
    DISPPARAMS dp;
    VARIANTARG *pvarg = 0;
    VARIANT vresult;
    EXCEPINFO ei;
    unsigned argerr = 0;
    unsigned counter;
    int dothis;

    if (LOG) WPRINTF(DTEXT("Dcomobject::put_Value()\n"));

    Value::copy(ret, &vundefined);

    if (NULL == m_pDisp && NULL == m_pDispEx)
    {
        if (LOG) WPRINTF(DTEXT("Call : Unable to find something to call.\n"));
        return NULL;
    }

    dothis = 0;

    // clear out excepinfo
    memset(&ei, 0, sizeof(ei));

    // Allocate for the variants
    if (0 != (dothis + argc))
    {	int ac = dothis + argc;

        if (ac >= 32 || (pvarg = (VARIANTARG *)alloca(sizeof(VARIANTARG) * ac)) == NULL)
        {
	    // Not enough stack space - use mem
	    GC_LOG();
	    pvarg = (VARIANTARG *)mem.malloc(sizeof(VARIANTARG) * ac);
        }
	if (LOG) WPRINTF(L"pvarg = %x, dothis = %d, argc = %d\n", pvarg, dothis, argc);
    }

    DISPID dispidPut = DISPID_PROPERTYPUT;
    dp.rgdispidNamedArgs = &dispidPut;
    dp.cNamedArgs = 1;
    dp.cArgs = argc;
    dp.rgvarg = pvarg;

    // Loop through the argument list and build up the variant array
    // Note that arguments are stored right to left in pvarg[]
    // following the named parameters.
    for (counter = 0; counter < argc; counter++)
    {
        // Mutate dscript types to variant types
	ValueToVariant(&(pvarg[dothis + argc - counter - 1]),&(arglist[counter]));
	//ValueToVariant(&(pvarg[dothis + counter]),&(arglist[counter]));
    }

    // Do the call
    VariantInit(&vresult);

    WORD    wFlags = DISPATCH_PROPERTYPUT;
    if (argc && V_VT(&pvarg[dothis + 0]) == VT_DISPATCH)
        wFlags |= DISPATCH_PROPERTYPUTREF;

    if (LOG) WPRINTF(DTEXT("Dcomobject::put_Value(): Invoke()\n"));
    if (m_pDispEx)
    {
        hResult = m_pDispEx->InvokeEx(dispid,
                                    LCID_ENGLISH_US,
                                    wFlags,
                                    &dp,
                                    &vresult,
                                    &ei,
                                    NULL);
    }
    else
    {
        hResult = m_pDisp->Invoke(dispid,
                                    IID_NULL,
                                    LCID_ENGLISH_US,
                                    wFlags,
                                    &dp,
                                    &vresult,
                                    &ei,
                                    &argerr);
    }
    if (LOG) WPRINTF(DTEXT("Dcomobject::put_Value(): Invoke() returned %x\n"), hResult);

    if (DISP_E_BADVARTYPE == hResult)
    {
	if (LOG)
	{
	    WPRINTF(DTEXT("Bad Variant Type returned for Invoke\n"));
	    WPRINTF(DTEXT("dp.cArgs %d\n"), dp.cArgs);
	    for (unsigned int kk = 0; kk < dp.cArgs; kk++)
		WPRINTF(DTEXT("    dp.rgvarg[%d].vt = %d\n"), kk, dp.rgvarg[kk].vt);
	}
    }

    if (pvarg)
    {
        for (counter = 0; counter < argc + dothis; counter++)
        {
            VariantClear(&pvarg[counter]);
        }
    }

    if (FAILED(hResult))
    {
        Value* pvalErr = NULL;
	ErrInfo errinfo;

        if (DISP_E_EXCEPTION == hResult)
        {
#if LOG
	    WPRINTF(L"DISP_E_EXCEPTION %s\n", ei.bstrDescription);
#endif
	    errinfo.code = ei.scode;
            pvalErr = RuntimeError(&errinfo, ERR_S_S,
                                   ei.bstrSource ? ei.bstrSource : L"",
                                   ei.bstrDescription ? ei.bstrDescription : L"");
        }
        else
        {
	    int errnum = hresultToMsgnum(hResult, ERR_COM_FUNCTION_ERROR);
	    pvalErr = RuntimeError(&errinfo, errnum,
		    DTEXT("put_Value()"), hResult);
        }

        SysFreeString(ei.bstrSource);
        SysFreeString(ei.bstrDescription);
        SysFreeString(ei.bstrHelpFile);
        memset(&ei, 0, sizeof(ei));

        return pvalErr;
    }

    // Convert return type to return value
    VariantToValue(ret, &vresult, d_string_ctor(L""), NULL);

    // Cleanup for the variant return type
    VariantClear(&vresult);

    return NULL;
} // put_Value()


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// isDcomobject()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
int Dcomobject::isDcomobject()
{
    return TRUE;
} // isDcomobject


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// toComObject()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
IDispatch* Dcomobject::toComObject()
{
    IDispatch*  pdisp = NULL;

    if (NULL != m_pDisp)
    {
        pdisp = m_pDisp;
        pdisp->AddRef();
    }
    else if (NULL != m_pDispEx)
    {
        HRESULT hr = m_pDispEx->QueryInterface(IID_IDispatch, (void**)&pdisp);
        assert(SUCCEEDED(hr) && NULL != pdisp);
        //
        // Don't need to addref, just got a ref through QI
        //
    }
    else if (NULL != m_pParent->m_pDisp)
    {
        pdisp = m_pParent->m_pDisp;
        pdisp->AddRef();
    }
    else if (NULL != m_pParent->m_pDispEx)
    {
        HRESULT hr = m_pParent->m_pDispEx->QueryInterface(IID_IDispatch, (void**)&pdisp);
        assert(SUCCEEDED(hr) && NULL != pdisp);
        //
        // Don't need to addref, just got a ref through QI
        //
    }

    //
    // Returns either NULL or the object we got
    //
    return pdisp;
} // toDcomobject


/* ====================== Dglobal_GetObject ================ */

BUILTIN_FUNCTION(Dglobal_, GetObject, 2)
{
#if !defined linux
    dchar *pathname = NULL;
    dchar *classname = NULL;

    CLSID   clsid;

    Dobject*    pObj = NULL;
    IDispatch*  pdisp = NULL;
    IUnknown*   punk = NULL;
    HRESULT     hResult = E_UNEXPECTED;

    switch (argc)
    {
    case 0:
        goto Lerror;

    default:
        classname = d_string_ptr(arglist[1].toString());
    case 1:
        pathname = d_string_ptr(arglist[0].toString());
        break;
    }

    if (*pathname == 0 && classname)
    {
        // return new instance of object

        // GetObject(, classname)
        // An automation object must call RegisterActiveObject to support GetObject(, classname).

        hResult = CLSIDFromProgID(classname, &clsid);
        if (FAILED(hResult))
        {
            if (LOG) WPRINTF(L"CLSIDFromProgID('%ls') failed with x%x", classname, hResult);
            goto Lerror;
        }
        hResult = GetActiveObject(clsid, NULL, &punk);
        if (FAILED(hResult))
        {
            if (LOG) WPRINTF(L"GetActiveObject('%ls') failed with x%x", classname, hResult);
            goto Lerror;
        }
        assert(NULL != punk);

        hResult = punk->QueryInterface(IID_IDispatch, (void **)&pdisp);
        punk->Release();
        if (FAILED(hResult))
            goto Lerror;
    }
    else if (pathname && !classname)
    {
        // If the document object is not already running, a new
        // instance of the object's server application is launched,
        // and the application is told to open the corresponding file.
        // The pathname must represent an existing file.

        // GetObject(filename,)
        // An automation object must implement IPersistFile to support
        // GetObject(filename,).

        IBindCtx*   pbc = NULL;
        IMoniker*   pmk = NULL;
        ULONG       cEaten = 0;

        hResult = CreateBindCtx(0, &pbc);
        if (FAILED(hResult))
        {
            if (LOG) WPRINTF(L"CreateBindCtx() failed with x%x", hResult);
            goto Lerror;
        }
        hResult = MkParseDisplayName(pbc, pathname, &cEaten, &pmk);
        if (FAILED(hResult))
        {
            pbc->Release();
            if (LOG) WPRINTF(L"MkParseDisplayName() failed with x%x", hResult);
            goto Lerror;
        }
        hResult = BindMoniker(pmk, 0, IID_IDispatch, (void **)&pdisp);
        pmk->Release();
        pbc->Release();
        if (FAILED(hResult))
        {
            if (LOG) WPRINTF(L"BindMoniker() failed with x%x", hResult);
            goto Lerror;
        }
    }
    else
    {
        // A new instance of the application is always launched,
        // even if the document is already open in a running
        // instance of the application

        // GetObject(filename, classname)
        // An automation object must implement IPersistFile to support
        // GetObject(filename, classname). 

        IPersistFile *pPF = NULL;

        hResult = CLSIDFromProgID(classname, &clsid);
        if (FAILED(hResult))
        {   if (LOG) WPRINTF(L"CLSIDFromProgID('%ls') failed with x%x", classname, hResult);
            goto Lerror;
        }
        hResult = CoCreateInstance(clsid, NULL, CLSCTX_SERVER,
                                   IID_IUnknown, (void **)&punk);
        if (FAILED(hResult))
        {   if (LOG) WPRINTF(L"CoCreateInstance() failed with x%x", hResult);
            goto Lerror;
        }
        hResult = punk->QueryInterface(IID_IPersistFile, (void **)&pPF);
        punk->Release();
        punk = NULL;
        if (FAILED(hResult))
            goto Lerror;

        hResult = pPF->Load(pathname, 0);
        if (FAILED(hResult))
        {   pPF->Release();
            goto Lerror;
        }

        hResult = pPF->QueryInterface(IID_IDispatch, (void **)&pdisp);
        pPF->Release();
        if (FAILED(hResult))
            goto Lerror;
    }

    assert(NULL != pdisp);
    GC_LOG();
#if _MSC_VER
    // work around bug with new(this)
    pObj = new Dcomobject(d_string_ctor(L""), pdisp, NULL);
#elif __DMC__
    // work around bug with new(this)
    pObj = new Dcomobject(d_string_ctor(L""), pdisp, NULL);
#else
    pObj = new(this) Dcomobject(d_string_ctor(L""), pdisp, NULL);
#endif
    //WPRINTF(L"2Dcomobject = %x, pDisp = %x\n", pObj, pdisp);
    pdisp->Release();

    Vobject::putValue(ret, pObj);
    return NULL;

Lerror:
#endif /* !defined linux */
    Value::copy(ret, &vundefined);
    return NULL;
}


/* ===================== Dactivexobject_constructor ==================== */

struct Dactivexobject_constructor : Dfunction
{
    Dactivexobject_constructor();

    void *Construct(CONSTRUCT_ARGS);
};


Dactivexobject_constructor::Dactivexobject_constructor()
    : Dfunction(1, Dfunction::getPrototype())
{
    name = "ActiveXObject";
}

void *Dactivexobject_constructor::Construct(CONSTRUCT_ARGS)
{
    // This is equivalent to the VBScript function CreateObject(classname)
    d_string s;
    dchar *p;
    Dobject* o = NULL;

    CLSID       clsid;
    IDispatch*  pdisp = NULL;
    HRESULT     hResult = E_UNEXPECTED;

    if (LOG) WPRINTF(L"Dactivexobject_constructor\n");
    switch (argc)
    {
    case 0:
	if (LOG) WPRINTF(L"no arguments\n");
        goto Lerror;

    default:
        s = arglist[0].toString();
        p = d_string_ptr(s);
        break;

    // Ignore second argument
    }

    if (LOG) WPRINTF(L"p = '%ls'\n", p);
    hResult = CLSIDFromProgID(p, &clsid);

    if (FAILED(hResult))
    {
        if (LOG) WPRINTF(L"CLSIDFromProgID('%ls') failed with x%x", p, hResult);
        goto Lerror;
    }

#if 0
WPRINTF(L"CLSID = { %08lx, %04x, %04x, { %02x %02x %02x %02x %02x %02x %02x %02x }}",
    clsid.Data1, clsid.Data2, clsid.Data3,
    clsid.Data4[0],clsid.Data4[1],clsid.Data4[2],clsid.Data4[3],
    clsid.Data4[4],clsid.Data4[5],clsid.Data4[6],clsid.Data4[7]);
#endif

    //
    // NOTE: Not using CLSCTX_INPROC_HANDLER because it seems to screw us up in
    //       some cases. For instance, Excel objects have a LocalServer32 and an
    //       InprocHandler32 registered, but the handler doesn't work right.
    //       MS JScript doesn't seem to use INPROC_HANDLER either.
    //
    hResult = CoCreateInstance(clsid, NULL,
                               CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER,
                               IID_IDispatch, (void **)&pdisp);
    if (FAILED(hResult))
    {
        if (LOG) WPRINTF(L"CoCreateInstance() failed with x%x", hResult);
        goto Lerror;
    }

    GC_LOG();
    o = new Dcomobject(d_string_ctor(L""), pdisp, NULL);
    //WPRINTF(L"3Dcomobject = %x, pDisp = %x\n", o, pdisp);
    pdisp->Release();
    Vobject::putValue(ret, o);
    return NULL;

Lerror:
    ErrInfo errinfo;
    return RuntimeError(&errinfo, ERR_ACTIVEX);
}


/* ============================================================== */

// Shell to avoid pulling in all the windows header files in other modules

void Dcomobject_init(ThreadContext *tc)
{
}

void Dcomobject_addCOM(Dobject *o)
{
    // o is really the global object.
    // Add automation support objects

    o->Put(TEXT_ActiveXObject, new(o) Dactivexobject_constructor(), DontEnum);
    o->Put(TEXT_GetObject, new(o) Dglobal_GetObject(), DontEnum);
}


/* =======================Type mutation helpers==================== */

void ValueToVariant(VARIANTARG *variant, Value *value)
{
    dchar *tx;

#if LOG
    WPRINTF(L"ValueToVariant(%x, %x)\n", variant, value);
    WPRINTF(L"\t%s\n", value->getType());
#endif

    // Initialize
    VariantInit(variant);

    // Get value type
    tx = value->getType();

    if (tx == TypeObject) {
        Dobject*    pObj = NULL;
        IDispatch*  pdisp = NULL;

        pObj = value->toObject();
#if LOG
	WPRINTF(L"\tIt's a Dobject %x\n", pObj);
#endif
        if (NULL != pObj)
        {
	    if (pObj->isDvariant() || pObj->isDvbarray())
	    {
#if LOG
		WPRINTF(L"\tIt's a Dvariant\n");
#endif
		Dvariant *dv = (Dvariant *)pObj;
		VariantCopy(variant, &dv->variant);
		return;
	    }
            pdisp = pObj->toComObject();

            if (NULL != pdisp)
            {
#if LOG
		WPRINTF(L"\tIt's an IDispatch %x\n", pdisp);
#endif
                V_VT(variant) = VT_DISPATCH;
                V_DISPATCH(variant) = pdisp;
                return;
            }
        }

        V_VT(variant) = VT_NULL;
    }
    else if (tx == TypeUndefined) {
        // VariantInit is sufficient for this (VT_EMPTY == TypeUndefined)
        assert(VT_EMPTY == V_VT(variant));
    }
    else if (tx == TypeNull) {
        V_VT(variant) = VT_NULL;
    }
    else if (tx == TypeNumber) {
	double r8;
	long i4;

	r8 = value->toNumber();
	i4 = (long)r8;
	if (i4 == r8)			// if exact conversion
        {   // Some Chcom objects expect an integer type, and will incorrectly give
	    // a DISP_E_BADVARTYPE error on real types. This is the WORKAROUND.
	    V_VT(variant) = VT_I4;
	    V_I4(variant) = i4;
	}
	else
        {   V_VT(variant) = VT_R8;
	    V_R8(variant) = r8;
	}
    }
    else if (tx == TypeBoolean) {
        V_VT(variant) = VT_BOOL;
        V_BOOL(variant) = (unsigned short) (value->toBoolean() ? 0xFFFF : 0);   // 0xFFFF == VARIANT_TRUE, 0 == VARIANT_FALSE
    }
    else if (tx == TypeString) {
	d_string s = value->toString();
        V_VT(variant) = VT_BSTR;
        V_BSTR(variant) = SysAllocStringLen(d_string_ptr(s), d_string_len(s));
#if 0
	WPRINTF(L"'%s'\n", d_string_ptr(s));
	WPRINTF(L"'");
	int i;
	for (i = 0; i < d_string_len(s); i++)
	{
	    WPRINTF(L"%c", d_string_ptr(s)[i]);
	}
	WPRINTF(L"'\nstring done\n");
#endif
    }
    else if (tx == TypeDate) {
	V_VT(variant) = VT_DATE;
	V_DATE(variant) = value->number;
    }
    else {
        // unexpected type to mutate
	if (LOG) WPRINTF(L"ValueToVariant()");
#       if defined _MSC_VER
	_asm int 3;
#       endif
    }
}

HRESULT VariantToValue(Value *ret, VARIANTARG *variant, d_string PropertyName, Dcomobject *pParent)
{   d_string s;
    HRESULT hResult;

    //WPRINTF(L"VariantToValue(vt = %d)\n", V_VT(variant));
    hResult = S_OK;
    switch(V_VT(variant))
    {
    case VT_NULL:
        Value::copy(ret, &vnull);
        break;

    case VT_EMPTY:
        if (LOG) WPRINTF(L"VT_EMPTY\n");
        Value::copy(ret, &vundefined);
        break;

    case VT_BOOL:
        Vboolean::putValue(ret, V_BOOL(variant) != 0);
        break;

    case VT_I2:
        Vnumber::putValue(ret, V_I2(variant));
        break;

    case VT_I4:
        Vnumber::putValue(ret, V_I4(variant));
        break;

    case VT_UI4:
#if defined __DMC__     // oleauto.h needs updating
        Vnumber::putValue(ret, (unsigned)V_I4(variant));
#else
        Vnumber::putValue(ret, V_UI4(variant));
#endif
        break;

    case VT_R4:
        Vnumber::putValue(ret, V_R4(variant));
        break;

    case VT_R8:
        Vnumber::putValue(ret, V_R8(variant));
        break;

    case VT_DATE:
	Vdate::putValue(ret, V_DATE(variant));
	break;

    case VT_BSTR:
        {
            dchar *p;

            p = V_BSTR(variant);
            // Experiments show this can be NULL
            // (happens with www.msn.com)
            if (p == NULL)
                p = L"";

	    Mem mem;
            s = Dstring::dup(&mem, p);
            if (LOG) WPRINTF(DTEXT("VT_BSTR = '%ls'\n"), p);
            Vstring::putValue(ret, s);
            break;
        }

    case VT_DISPATCH:				// 9
        {
            IDispatch*      pdisp = NULL;
            Dobject*        pObj = NULL;
            DSobjDispatch*  pds = NULL;

            pdisp = V_DISPATCH(variant);
            if (LOG) WPRINTF(DTEXT("VT_DISPATCH = %x\n"), pdisp);

            if (NULL == pdisp)
            {
                Value::copy(ret, &vnull);
                break;
            }

            // See if pdisp is our COM wrapper by doing a QI on a
            // special secret IID that our wrappers support, but
            // which nobody else will. (HACK'd QueryInterface :-( )
            //
            // This prevents us from recursively wrapping a Dobject
            // in a DSobjdispatch in a Dcomobject in a DSobjdispatch...forever
            //
            hResult = pdisp->QueryInterface(IID_DSobjDispatch, (void **)&pds);

            if (FAILED(hResult) || NULL == pds)
            {
                // pdisp is not a Dobject, so create a wrapper for it
		GC_LOG();
                pObj = new Dcomobject(PropertyName, pdisp, pParent);
		//WPRINTF(L"4Dcomobject = %x, pDisp = %x\n", pObj, pdisp);
                if (NULL == pObj)
                    hResult = E_OUTOFMEMORY;
                else
                    hResult = S_OK;
            }
            else
            {
                pObj = pds->getObj();
                pds->Release(); // Release the extra ref we got in the QI()
            }

            pdisp = NULL;   // Do NOT Release() -- we don't own the reference

            Vobject::putValue(ret, pObj);
            break;
        }

    default:
	if (LOG) WPRINTF(L"VariantToValue(%d, x%x)\n", V_VT(variant), V_VT(variant));
	{   Dvariant *dv = new Dvariant(NULL, variant);
	    Vobject::putValue(ret, dv);
	    break;
	}
    case VT_VARIANT | VT_ARRAY:
	{   Dvbarray *dv = new Dvbarray(variant);
	    Vobject::putValue(ret, dv);
	    break;
	}
    }
    return hResult;
}


