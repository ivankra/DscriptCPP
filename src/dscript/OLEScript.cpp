/* Digital Mars DMDScript source code.
 * Copyright (c) 2000-2001 by Chromium Communications
 * Copyright (c) 2004-2007 by Digital Mars
 * All Rights Reserved.
 * written by Paul R. Nash, Walter Bright, and Pat Nelson
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
#include <locale.h>
#include <stdio.h>

#include <objbase.h>
#include <dispex.h>

#if 0 && !defined linux
#include <hostinfo.h>
#endif

#include "perftimer.h"

#include "dscript.h"
#include "root.h"
#include "stringtable.h"
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

#include "OLEScript.h"
#include "ActiveScriptError.h"
#include "ParsedProcedure.h"

/* Script states are (from activscp.h):
	SCRIPTSTATE_UNINITIALIZED	= 0
	SCRIPTSTATE_STARTED		= 1
	SCRIPTSTATE_CONNECTED		= 2
	SCRIPTSTATE_DISCONNECTED	= 3
	SCRIPTSTATE_CLOSED		= 4
	SCRIPTSTATE_INITIALIZED		= 5
 */

/* Script items are (from activscp.h):
	SCRIPTITEM_ISVISIBLE		= 0x2
	SCRIPTITEM_ISSOURCE		= 0x4
	SCRIPTITEM_GLOBALMEMBERS	= 0x8
	SCRIPTITEM_ISPERSISTENT		= 0x40
	SCRIPTITEM_CODEONLY		= 0x200
	SCRIPTITEM_NOCODE		= 0x400

	from olescrpt.h:
	SCRIPTITEM_MULTIINSTANCE	= 0x100
 */

#define LOG 0

void clearStack()
{
    int foo[2000];

    memset(foo, 0, sizeof(foo));
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// *** Constructor / Destructor ***
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// COleScript()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
COleScript::COleScript()
  : m_pScriptSite(NULL),
    m_pNamedItems(NULL),
    m_pParsedScripts(NULL),
    m_pGlobalModuleDisp(NULL),
    m_stsState(SCRIPTTHREADSTATE_NOTINSCRIPT),
    m_ssState(SCRIPTSTATE_UNINITIALIZED),
#ifndef DISABLE_OBJECT_SAFETY
    m_dwCurrentSafetyOptions(0),
#endif // !DISABLE_OBJECT_SAFETY
    m_dwRuntimeUniqueNumber(100),             // Start at an arbitrary non-zero value
    m_uNextModuleNumber(GLOBAL_MODULE+1),
    m_lcid(LOCALE_USER_DEFAULT),
    m_fPersistLoaded(false),
    m_fIsPseudoDisconnected(false)
{
    ENTRYLOG(L"COM: ENTER COleScript() this = %x\n", this);
#if INVARIANT
    signature = COLESCRIPT_SIGNATURE;
#endif
    GC_LOG();
    m_pProgram = new Program(NULL, NULL);
    m_pProgram->lcid = m_lcid;
    //WPRINTF(L"\tm_pProgram = x%x\n", m_pProgram);
    m_pProgram->initContext();
    m_pccGlobal = _CopyCallContext(m_pProgram->callcontext);
    invariant();
    ENTRYLOG(L"COM: EXIT COleScript()\n");
} // COleScript


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// ~COleScript()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
COleScript::~COleScript()
{
    ENTRYLOG(L"COM: ENTER ~COleScript() this = %x\n", this);
    invariant();
    assert(m_ssState == SCRIPTSTATE_CLOSED || m_ssState == SCRIPTSTATE_UNINITIALIZED);
    Close();

    if (NULL != m_pccGlobal)
    {
        if (NULL != m_pccGlobal->scope)
	    m_pccGlobal->scope->zero();

	//delete m_pccGlobal;
        memset(m_pccGlobal, 0, sizeof(CallContext));
        m_pccGlobal = NULL;
    }

    if (NULL != m_pProgram)
    {
	//delete m_pProgram;
        memset(m_pProgram, 0, sizeof(Program));
        m_pProgram = NULL;
    }

    Program::setProgram(NULL);

#if INVARIANT
    signature = 0;
#endif

    // NULL out pointers so we can GC
    m_pScriptSite = NULL;
    m_pNamedItems = NULL;
    m_pParsedScripts = NULL;

    // Free the string table
    ThreadContext *tc = ThreadContext::getThreadContext();
    if (tc->stringtable)
    {	tc->stringtable->~StringTable();
	tc->stringtable = NULL;
    }

    dobject_term(tc);

    // NOTE: We know at this point that we have NO valid roots
    // on the stack. Every root should be in ThreadContext. Since
    // Chcom uses large amounts of stack that is never initialized,
    // scanning the stack results in the pinning of large amounts of
    // memory due to leftover stackframe garbage.

    // Don't do it here, in preference to doing it in Reset()
    mem.fullcollectNoStack();


    logflag = 0;			// turn off logging
    ENTRYLOG(L"COM: EXIT ~COleScript()\n");
} // ~COleScript


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// *** IUnknown Methods ***
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// QueryInterface()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP COleScript::QueryInterface(REFIID riid, void** ppvObj)
{
    invariant();
    if (NULL == ppvObj)
        return E_INVALIDARG;

    if (riid == IID_IUnknown || riid == IID_IActiveScript) {
        *ppvObj = static_cast<IActiveScript*>(this);
//        DPF(L"COM: QI for IUnknown/IActiveScript\n");
    }
    else if (riid == IID_IActiveScriptParse) {
        *ppvObj = static_cast<IActiveScriptParse*>(this);
//        DPF(L"COM: QI for IActiveScriptParse\n");
    }
    else if (riid == IID_IActiveScriptParseProcedure) {
        *ppvObj = static_cast<IActiveScriptParseProcedure*>(this);
//        DPF(L"COM: QI for IActiveScriptParseProcedure\n");
    }
#ifndef DISABLE_OBJECT_SAFETY
    else if (riid == IID_IObjectSafety) {
        *ppvObj = static_cast<IObjectSafety*>(this);
//        DPF(L"COM: QI for IObjectSafety\n");
    }
#endif // !DISABLE_OBJECT_SAFETY
    else if (riid == IID_IHostInfoUpdate) {
        *ppvObj = static_cast<IHostInfoUpdate*>(this);
//        DPF(L"COM: QI for IHostInfoUpdate (NYI)\n");
    }
    else {
        DPF(L"COM: QI for unsupported interface {%08lx-%04lx-%04lx-%04lx-%02lx%02lx%02lx%02lx%02lx%02lx}\n",
            riid.Data1, riid.Data2, riid.Data3, *((unsigned short*)(riid.Data4)),
            riid.Data4[2], riid.Data4[3], riid.Data4[4],
            riid.Data4[5], riid.Data4[6], riid.Data4[7] );
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    /* Other interfaces queried for by Explorer but we don't need to implement:
	{4954e0d0-fbc7-11d1-1084-006008c3fbfc}	IActiveScriptProperty
	{71ee5b20-fb04-11d1-a8b3-00a0c911e8b2}	IActiveScriptParseProcedure2
	{51973c10-cb0c-11d0-c9b5-00a0244a0e7a}	IActiveScriptDebug
     */

    assert(NULL != *ppvObj);
    reinterpret_cast<IUnknown*>(*ppvObj)->AddRef();

    return S_OK;
} // QueryInterface

#ifndef DISABLE_OBJECT_SAFETY

const DWORD OLESCRIPT_INTERFACE_SAFETY_OPTIONS_SUPPORTED    = (INTERFACESAFE_FOR_UNTRUSTED_CALLER |
                                                               INTERFACESAFE_FOR_UNTRUSTED_DATA |
                                                               INTERFACE_USES_DISPEX |
                                                               INTERFACE_USES_SECURITY_MANAGER);

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// *** IObjectSafety Methods ***
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// GetInterfaceSafetyOptions()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP COleScript::GetInterfaceSafetyOptions(/*[in] */ REFIID riid,
                                                   /*[out]*/ DWORD* pdwSupportedOptions,
                                                   /*[out]*/ DWORD* pdwEnabledOptions)
{
    ENTRYLOG(L"COM: ENTER GetInterfaceSafetyOptions()\n");
    invariant();

    //
    // We only support object-wide security settings, so we can ignore riid
    //
    UNREFERENCED_PARAMETER(riid);

    if (NULL == pdwSupportedOptions || NULL == pdwEnabledOptions)
    {
        DPF(L"COM: ERROR GetInterfaceSafetyOptions()\n");
        return E_INVALIDARG;
    }

    *pdwSupportedOptions = OLESCRIPT_INTERFACE_SAFETY_OPTIONS_SUPPORTED;
    *pdwEnabledOptions = m_dwCurrentSafetyOptions;

    return S_OK;
} // GetInterfaceSafetyOptions


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// SetInterfaceSafetyOptions()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP COleScript::SetInterfaceSafetyOptions(/*[in] */ REFIID riid,
                                                   /*[in] */ DWORD  dwOptionsSetMask,
                                                   /*[in] */ DWORD  dwEnabledOptions)
{
    ENTRYLOG(L"COM: ENTER SetInterfaceSafetyOptions(riid, 0x%08lx, 0x%08lx)\n",
        dwOptionsSetMask, dwEnabledOptions);
    invariant();
    //
    // We only support object-wide security settings, so we can ignore riid
    //
    UNREFERENCED_PARAMETER(riid);

    //
    // Early-out if the call is a no-op
    //
    if ((dwOptionsSetMask & dwEnabledOptions) == 0)
        return S_OK;

    //
    // If caller is changing flags we don't understand, then fail the call
    //
    if (dwOptionsSetMask & ~(OLESCRIPT_INTERFACE_SAFETY_OPTIONS_SUPPORTED))
    {
        DPF(L"COM: ERROR SetInterfaceSafetyOptions() unsupported flags\n");
        return E_FAIL;
    }

    //
    // New options are (all bits that aren't in mask left alone) + (enabled bits that are masked on)
    //
    m_dwCurrentSafetyOptions = (m_dwCurrentSafetyOptions & ~dwOptionsSetMask) | (dwEnabledOptions & dwOptionsSetMask);

    return S_OK;
} // SetInterfaceSafetyOptions

#endif // !DISABLE_OBJECT_SAFETY

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// *** IActiveScript Methods ***
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// SetScriptSite()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP COleScript::SetScriptSite(/*[in] */ IActiveScriptSite* passSite)
{
    LCID    lcid = LOCALE_USER_DEFAULT;

    ENTRYLOG(L"COM: ENTER SetScriptSite(passSite = x%x)\n", passSite);

    invariant();
    mem.setStackBottom((void *)&passSite);
    if (NULL == passSite)
        return E_INVALIDARG;

    if (NULL != m_pScriptSite)
        return E_UNEXPECTED;

    m_pScriptSite = passSite;
    m_pScriptSite->AddRef();

    if (SUCCEEDED(m_pScriptSite->GetLCID(&lcid)))
        m_lcid = lcid;
    else
        m_lcid = GetUserDefaultLCID();
    m_pProgram->lcid = m_lcid;

    _SetLocale();

    //
    // If we are a clone or were reset through a state change,
    // now is the time to re-register our persistent named items
    //
    if (NULL != m_pNamedItems)
    {
        _RegisterPersistentNamedItems();
    }

    if (m_fPersistLoaded)
    {
        _ChangeScriptState(SCRIPTSTATE_INITIALIZED);
    }

    ENTRYLOG(L"COM: EXIT SetScriptSite()\n");
    return S_OK;
} // SetScriptSite


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// GetScriptSite()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP COleScript::GetScriptSite(/*[in] */ REFIID riid,
                                       /*[out]*/ void** ppvSite)
{
    ENTRYLOG(L"COM: ENTER GetScriptSite()\n");
    invariant();

    if (NULL == ppvSite)
        return E_INVALIDARG;

    *ppvSite = NULL;

    if (NULL == m_pScriptSite)
        return S_FALSE;

    return m_pScriptSite->QueryInterface(riid, ppvSite);
} // GetScriptSite


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// SetScriptState()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP COleScript::SetScriptState(/*[in] */ SCRIPTSTATE ssNew)
{
    ENTRYLOG(L"COM: ENTER SetScriptState(ssNew = %d) this = %x\n", ssNew, this);
    //FuncLog funclog(L"SetScriptState()");
    invariant();
    mem.setStackBottom((void *)&ssNew);

    //
    // Nop's just bail immediately
    //
    if (m_ssState == ssNew)
    {
        DPF(L"COM: SetScriptState() : no-op change\n");
        return S_OK;
    }

    //
    // Caller is being bad -- we're closed, so this method fails now.
    //
    if (SCRIPTSTATE_CLOSED == m_ssState)
    {
        return E_UNEXPECTED;
    }
    if (SCRIPTSTATE_UNINITIALIZED == m_ssState)
    {
        //
        // Invalid to go from UNINITIALIZED to STARTED.
        // The only way out of UNINITIALIZED is through SetScriptState().
        //
        return E_UNEXPECTED;
    }

    HRESULT hr = E_INVALIDARG;

    switch(ssNew)
    {
        case SCRIPTSTATE_INITIALIZED:
            switch(m_ssState)
            {
                case SCRIPTSTATE_CONNECTED:         // Currently connected to events
                case SCRIPTSTATE_DISCONNECTED:      // Disconnected from events
                    Stop();
                case SCRIPTSTATE_STARTED:           // Executing code, but not connected
                    Reset(true);
                    _ChangeScriptState(SCRIPTSTATE_INITIALIZED);
                    hr = S_OK;
                    break;
                default:
                    break;
            }
            break;

        case SCRIPTSTATE_STARTED:
            switch(m_ssState)
            {
                case SCRIPTSTATE_DISCONNECTED:      // Disconnected from events
                    Stop();
                    Reset(true);
                case SCRIPTSTATE_INITIALIZED:       // Any code or named items get queued
                    _ExecutePendingScripts();        // Executes any code previously queued.
                case SCRIPTSTATE_CONNECTED:         // Currently connected to events
                    _ChangeScriptState(SCRIPTSTATE_STARTED);
                    hr = S_OK;
                    break;
                default:
                    break;
            }
            break;

        case SCRIPTSTATE_CONNECTED:
            switch(m_ssState)
            {
                case SCRIPTSTATE_INITIALIZED:
                    _ExecutePendingScripts();
                    //
                    // Fall-through: "if going from INITIALIZED to CONNECTED, engine must
                    //                transition through STARTED"
                    //
                case SCRIPTSTATE_STARTED:
                    hr = Run();
                    break;
                case SCRIPTSTATE_DISCONNECTED:
                    Reconnect();
                    hr = S_OK;
                    break;
                default:
                    break;
            }
            break;

        case SCRIPTSTATE_DISCONNECTED:
            hr = PseudoDisconnect();
            break;

        case SCRIPTSTATE_CLOSED:
            hr = Close();
            break;

	case SCRIPTSTATE_UNINITIALIZED:
            switch(m_ssState)
            {
                case SCRIPTSTATE_CONNECTED:         // Currently connected to events
                case SCRIPTSTATE_DISCONNECTED:      // Disconnected from events
                    Stop();
                case SCRIPTSTATE_STARTED:           // Executing code, but not connected
                case SCRIPTSTATE_INITIALIZED:       // In initialized state
                    //
                    // Reset, but don't re-register named items just yet
                    //
                    Reset(false);
                    if (NULL != m_pScriptSite)
                    {
                        m_pScriptSite->Release();
                        m_pScriptSite = NULL;
                    }

                    _ChangeScriptState(SCRIPTSTATE_UNINITIALIZED);
                    hr = S_OK;
                    break;
                default:
                    break;
            }
            break;

        default:
            //
            // Fall-through causes default E_UNEXPECTED to be returned.
            //
            break;
    }

    ENTRYLOG(L"COM: EXIT SetScriptState() : hr=0x%08lx\n", hr);
    return hr;
} // SetScriptState


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// GetScriptState()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP COleScript::GetScriptState(/*[out]*/ SCRIPTSTATE* pssState)
{
    ENTRYLOG(L"COM: ENTER GetScriptState()\n");
    invariant();

    if (NULL == pssState)
        return E_INVALIDARG;

    *pssState = m_ssState;

    return S_OK;
} // GetScriptState


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Close()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP COleScript::Close(void)
{
    ENTRYLOG(L"COM: ENTER Close() this = x%x\n", this);
    invariant();

    SNamedItemData* pnid = NULL;
    SParsedScriptData*  ppsd = NULL;

    mem.setStackBottom((void *)&pnid);

    switch(m_ssState)
    {
        case SCRIPTSTATE_CONNECTED:
        case SCRIPTSTATE_DISCONNECTED:
            Stop();
            _TeardownEventHandlers();
        case SCRIPTSTATE_INITIALIZED:
        case SCRIPTSTATE_STARTED:
	    if (NULL != m_pccGlobal)
	    {
		if (NULL != m_pccGlobal->scope)
		    m_pccGlobal->scope->zero();
		//delete m_pccGlobal;
		memset(m_pccGlobal, 0, sizeof(CallContext));
		m_pccGlobal = NULL;
	    }
	    m_pProgram = NULL;
            if (NULL != m_pGlobalModuleDisp)
            {
                m_pGlobalModuleDisp->Release();
                m_pGlobalModuleDisp = NULL;
            }
            // Cause all the back-pointers to any
            // COM objects to be released
        case SCRIPTSTATE_UNINITIALIZED:
            _ChangeScriptState(SCRIPTSTATE_CLOSED);
            //
            // Free up named item list
            //
            while (NULL != m_pNamedItems)
            {
                pnid = m_pNamedItems;
                m_pNamedItems = pnid->pNext;

                pnid->pNext = NULL;
                delete pnid;
                pnid = NULL;    // Dtor will cleanup any allocated resources
            }
            //
            // Free up parsed list
            //
            while (NULL != m_pParsedScripts)
            {
                ppsd = m_pParsedScripts;
                m_pParsedScripts = ppsd->pNext;

                ppsd->pNext = NULL;
                delete ppsd;
                ppsd = NULL;        // Dtor will cleanup any allocated resources
            }

            //
            // Let go of the site interface
            //
            if (NULL != m_pScriptSite)
            {
                m_pScriptSite->Release();
                m_pScriptSite = NULL;
            }

        case SCRIPTSTATE_CLOSED:
            break;

        default:
            //
            // Should've handled all possible values of m_ssState in above cases
            //
            assert(false);
            return E_UNEXPECTED;
            break;
    }
    ENTRYLOG(L"COM: EXIT Close()\n");

    return S_OK;
} // Close


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// AddNamedItem()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP COleScript::AddNamedItem(/*[in] */ LPCOLESTR pcwszName,
                                      /*[in] */ DWORD dwFlags)
{
    ENTRYLOG(L"COM: ENTER AddNamedItem(\"%s\", 0x%08lx)\n", pcwszName, dwFlags);
    invariant();
    mem.setStackBottom((void *)&dwFlags);

    if (NULL == pcwszName)
        return E_POINTER;       // E_POINTER, E_INVALIDARG, whatever -- doesn't really matter

    //
    // NOTE: We don't care what state we're in now. We can do our work here
    //       regardless of the current SCRIPTSTATE.
    //

    //
    // First, check the flags to make sure we don't have conflicts.  We don't
    // support SCRIPTITEM_ALL_FLAGS
    //

    //
    // CODEONLY conflicts with everything but ISPERSISTENT
    //
    if ((dwFlags & SCRIPTITEM_CODEONLY) && (dwFlags & (SCRIPTITEM_GLOBALMEMBERS |
                                                       SCRIPTITEM_ISSOURCE |
                                                       SCRIPTITEM_ISVISIBLE |
                                                       SCRIPTITEM_NOCODE)))
    {
        DPF(L"COM: ERROR AddNamedItem() : flag incompatibility with SCRIPTITEM_CODEONLY\n");
        return E_INVALIDARG;
    }

    //
    // ISSOURCE conflicts with NOCODE
    //
    if ((dwFlags & SCRIPTITEM_ISSOURCE) && (dwFlags & SCRIPTITEM_NOCODE))
    {
        DPF(L"COM: ERROR AddNamedItem() : ISSOURCE and NOCODE given\n");
        return E_INVALIDARG;
    }

    SNamedItemData* pnid = NULL;

    //
    // If the caller tries to add a NamedItem that has already been added,
    // return with an error. Note that we don't need to use pnid right now.
    // We'll find it again later when we actually execute this code.
    //
    pnid = _FindNamedItem(pcwszName);
    if (NULL != pnid)
    {
        //
        // No-op if we are re-adding an existing named item with same flags, else error
        //
        if (dwFlags != pnid->dwAddFlags)
        {
            DPF(L"COM: ERROR AddNamedItem() : different flags given for existing item\n");
            return E_INVALIDARG;
        }
        else
        {
            ENTRYLOG(L"COM: EXIT AddNamedItem() : existing item early out\n");
            return S_OK;
        }
    }

    HRESULT hr = E_UNEXPECTED;

    //
    // Create a new named item structure
    //
    GC_LOG();
    pnid = new SNamedItemData;
    if (NULL == pnid)
        return E_OUTOFMEMORY;

    pnid->bszItemName = SysAllocString(pcwszName);
    if (NULL == pnid->bszItemName)
    {
        pnid->pNext = NULL;
        delete pnid;
        pnid = NULL;
        return E_OUTOFMEMORY;
    }

    pnid->dwAddFlags = dwFlags;

    if (dwFlags & SCRIPTITEM_GLOBALMEMBERS)
    {
        pnid->uModuleNumber = GLOBAL_MODULE;
    } else
    {
        pnid->uModuleNumber = m_uNextModuleNumber++;
    }

    if ((dwFlags & SCRIPTITEM_CODEONLY) || !(dwFlags & SCRIPTITEM_NOCODE))
    {
        pnid->fHasCode = true;
    }

    //
    // Register the item
    //
    hr = _RegisterNamedItem(pnid);

    if (FAILED(hr))
    {
        pnid->pNext = NULL;
        delete pnid;
        pnid = NULL;            // Dtor cleans up allocations
        return hr;
    }

    pnid->pNext = m_pNamedItems;
    m_pNamedItems = pnid;

    ENTRYLOG(L"COM: EXIT AddNamedItem()\n");
    return S_OK;
} // AddNamedItem


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// AddTypelib()
// See:
//	http://www.devx.com/premier/mgznarch/vcdj/2000/08aug00/aa0008/aa0008.asp
// for some info on it.
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP COleScript::AddTypeLib(/*[in] */ REFGUID rguidTypeLib,
                                    /*[in] */ DWORD dwMajor,
                                    /*[in] */ DWORD dwMinor,
                                    /*[in] */ DWORD dwFlags)
{
    //
    // DPF so we can see if this is called (ENTRYLOG is disabled right now)
    //
    DPF(L"COM: ENTER COleScript::AddTypeLib({%08lx-%04lx-%04lx...}, dwMajor=%d, dwMinor=%d, dwFlags=0x%08lx)\n",
        rguidTypeLib.Data1, rguidTypeLib.Data2, rguidTypeLib.Data3,
        dwMajor, dwMinor, dwFlags);
    invariant();
    mem.setStackBottom((void *)&dwFlags);

    UNREFERENCED_PARAMETER(rguidTypeLib);
    UNREFERENCED_PARAMETER(dwMajor);
    UNREFERENCED_PARAMETER(dwMinor);

    if (SCRIPTTYPELIB_ISCONTROL & dwFlags)
    {
        DPF(L"COM: ERROR AddTypeLib() : can't handle SCRIPTTYPELIB_ISCONTROL\n");
        return E_NOTIMPL;
    }
    else if (0 != dwFlags)
    {
        DPF(L"COM: ERROR AddTypeLib() : unknown dwFlags.\n");
        return E_INVALIDARG;
    }

    //
    // Attempt to find out the path location of the identified
    // TypeLib so we can load it and then scan it for information.
    //
    BSTR        bszPath = NULL;
    HRESULT     hr = E_UNEXPECTED;
    ITypeLib*   ptl = NULL;
    UINT        cTypeInfos = 0;
    UINT        i;

    hr = QueryPathOfRegTypeLib(rguidTypeLib, (USHORT)dwMajor, (USHORT)dwMinor,
                                       LCID_ENGLISH_US, &bszPath);
    if (FAILED(hr))
        goto Ret;
    if (NULL == bszPath)
    {
        hr = E_FAIL;
        goto Ret;
    }

    DPF(L"COM: AddTypeLib() : Path is \"%s\"\n", bszPath);
    hr = LoadTypeLib(bszPath, &ptl);
    if (FAILED(hr))
        goto Ret;

    //
    // Walk through the TLB and
    // look for any constants or enums that we can find.
    //

    assert(ptl);
    cTypeInfos = ptl->GetTypeInfoCount();

    for (i = 0; i < cTypeInfos; i++)
    {
	LPTYPEINFO pITypeInfo;
	//BSTR pssTypeInfoName;
	TYPEATTR *pTypeAttr;

	hr = ptl->GetTypeInfo(i, &pITypeInfo);
        //COMD1("\tpITypeInfo %x\n",pITypeInfo);
	if (S_OK != hr)
	    goto Ret;

#if 0	//not used
	//hr = pITypeInfo->GetDocumentation(MEMBERID_NIL, &pssTypeInfoName, 0, 0, 0);
	//if (S_OK != hr)
	    //goto fail1;
#endif

	hr = pITypeInfo->GetTypeAttr(&pTypeAttr);
        //COMD1("\tpTypeAttr %x\n",pTypeAttr);
	if (S_OK != hr)
	{
	    //SysFreeString(pssTypeInfoName);
	    pITypeInfo->Release();
	    goto Ret;
	}

        //COMD1("\tpTypeAttr->cVars %x\n",pTypeAttr->cVars);
	if (pTypeAttr->cVars)
	{   // only interested in consts
	    int j;
	    for (j = 0; j < pTypeAttr->cVars; j++)
	    {
		VARDESC *pVarDesc;
		BSTR     pssVarName;
		Value	 v;

		pITypeInfo->GetVarDesc(j, &pVarDesc);
		hr = pITypeInfo->GetDocumentation(pVarDesc->memid, &pssVarName, 0, 0, 0);
		switch (pVarDesc->varkind)
		{
		    case VAR_PERINSTANCE:	// field or member of type
			//COMD("\t\tVAR_PERINSTANCE\n");
			break;

		    case VAR_STATIC:		// variable
			//COMD("\t\tVAR_STATIC\n");
#if 0			// does script engine read these?
			ELEMDESC elemdesc = pVarDesc->elemdescVar;
			TYPEDESC tdesc = elemdesc->tdesc;
			VARTYPE vt = tdesc->vt;
			switch (vt)
			{
			}
#endif
			break;

		    case VAR_CONST:		// constant
		    {	// Because typelib is handled after parsing consts
			// must be treated as var symbols
			//WPRINTF(L"\t\tVAR_CONST = '%s'\n", pssVarName);
			VariantToValue(&v, pVarDesc->lpvarValue, NULL, NULL);
			Mem mem;
			d_string s = Dstring::dup(&mem, pssVarName);
			m_pccGlobal->global->Put(s, &v, ReadOnly);
			break;
		    }
		    case VAR_DISPATCH:		// object
			// does script engine read these?
			//COMD("\t\tVAR_DISPATCH\n");
			//dispid = pVarDesc->memid;
			break;

		    default:
			break;
		}
		SysFreeString(pssVarName);
		pITypeInfo->ReleaseVarDesc(pVarDesc);
	    }
	}

	//SysFreeString(pssTypeInfoName);
	pITypeInfo->ReleaseTypeAttr(pTypeAttr);
//fail1:
	pITypeInfo->Release();
    }

    hr = S_OK;

Ret:

    if (NULL != ptl)
    {
        ptl->Release();
    }

#if 0
    // According to MSDN, QueryPathOfRegTypeLib() strangely expects
    // the caller to allocate the BSTR, which can't be done because
    // there's no data for the string. Letting Q...() allocate the BSTR
    // results in a crash here when we free it.
    // So, don't free it.
    if (NULL != bszPath)
    {
        SysFreeString(bszPath);
    }
#endif


    DPF(L"COM: EXIT COleScript::AddTypeLib(), hr = x%x\n", hr);
    return hr;
} // AddTypeLib


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// GetScriptDispatch()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP COleScript::GetScriptDispatch(/*[in] */ LPCOLESTR pcwszItemName,
                                           /*[out]*/ IDispatch** ppDisp)
{
    UNREFERENCED_PARAMETER(pcwszItemName);

    ENTRYLOG(L"COM: ENTER GetScriptDispatch(this = x%x, '%s')\n", this, pcwszItemName);
    invariant();
    mem.setStackBottom((void *)&ppDisp);
    if (NULL == ppDisp)
        return E_POINTER;

    *ppDisp = NULL;     // Clear the out-param like a good COM citizen

    SNamedItemData* pnid = NULL;

    HRESULT         hr = E_UNEXPECTED;

    //
    // If a namespace name was provided, try to find our data structure
    // describing that namespace.
    //
    if (NULL != pcwszItemName && wcslen(pcwszItemName) > 0)
    {
        pnid = _FindNamedItem(pcwszItemName);

        //
        // Couldn't find it -- the namespace has not been added!
        //
        if (NULL == pnid)
        {
            DPF(L"COM: ERROR GetScriptDispatch() : named item not found\n");
            return E_INVALIDARG;
        }
    }

    //
    // If the NamedItem was added with no code (SCRIPTITEM_NOCODE), then
    // there was no namespace created and we can't return a dispatch
    // for said namespace.
    //
    if (NULL != pnid && !pnid->fHasCode)
        return E_INVALIDARG;

    hr = _BuildScriptDispatchWrapper(pnid, ppDisp, 0);

    ENTRYLOG(L"COM: EXIT GetScriptDispatch() returning 0x%08lx\n", hr);
    return hr;
} // GetScriptDispatch


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// GetCurrentScriptThreadID()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP COleScript::GetCurrentScriptThreadID(/*[out]*/ SCRIPTTHREADID* pstidThread)
{
    UNREFERENCED_PARAMETER(pstidThread);

    DPF(L"COM: WARNING! GetCurrentScriptThreadID() called (NYI)\n");
    invariant();
    return S_OK;
} // GetCurrentScriptThreadID


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// GetScriptThreadID()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP COleScript::GetScriptThreadID(/*[in] */ DWORD dwWin32ThreadId,
                                           /*[out]*/ SCRIPTTHREADID* pstidThread)
{
    UNREFERENCED_PARAMETER(dwWin32ThreadId);
    UNREFERENCED_PARAMETER(pstidThread);

    DPF(L"COM: WARNING! GetScriptThreadID(0x%08lx) called (NYI)\n", dwWin32ThreadId);
    invariant();
    return S_OK;
} // GetScriptThreadID


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// GetScriptThreadState()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP COleScript::GetScriptThreadState(/*[in] */ SCRIPTTHREADID stidThread,
                                              /*[out]*/ SCRIPTTHREADSTATE* pstsState)
{
    DPF(L"COM: ENTER GetScriptThreadState(0x%08lx) called (Not fully implemented)\n", stidThread);
    invariant();

    if (NULL == pstsState)
        return E_POINTER;

    if (SCRIPTSTATE_UNINITIALIZED == m_ssState ||
        SCRIPTSTATE_CLOSED == m_ssState)
    {
        return E_UNEXPECTED;
    }

    if (SCRIPTTHREADID_BASE == stidThread)
        *pstsState = m_stsState;

    return S_OK;
} // GetScriptThreadState


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// InterruptScriptThread()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP COleScript::InterruptScriptThread(/*[in] */ SCRIPTTHREADID stidThread,
                                               /*[in] */ const EXCEPINFO* pexcepinfo,
                                               /*[in] */ DWORD dwFlags)
{
    UNREFERENCED_PARAMETER(pexcepinfo);
    UNREFERENCED_PARAMETER(dwFlags);

    //WPRINTF(L"InterruptScriptThread()\n");
    ENTRYLOG(L"ENTER: InterruptScriptThread(%d,x%x, dwFlags=x%x) m_pccGlobal %x\n",stidThread,pexcepinfo,dwFlags,m_pccGlobal);
    CallContext *cc;
    Program *p;

    invariant();
    if (m_pScriptSite == NULL || m_pccGlobal == NULL)
    {
	return E_UNEXPECTED;
    }
    if (dwFlags)
	return E_INVALIDARG;
    switch (stidThread)
    {
	case SCRIPTTHREADID_ALL:
#if LOG
	    WPRINTF(L"\tSCRIPTTHREADID_ALL\n");
#endif
	    return E_UNEXPECTED;

	case SCRIPTTHREADID_BASE:
#if LOG
	    WPRINTF(L"\tSCRIPTTHREADID_BASE, this = %x\n", this);
	    //WPRINTF(L"error '%s', number %d\n", pei->bstrDescription, pei->scode);
#endif
	    p = Program::getProgram();
	    if (!p)
		p = m_pProgram;
	    if (p)
	    {
		cc = p->callcontext;
		//m_pccGlobal->Interrupt = 1;
		cc->Interrupt = 1;
#if LOG
		WPRINTF(L"\tthis = %x, program = %x, cc = %x\n", this, p, cc);
#endif
	    }
    	    break;

	case SCRIPTTHREADID_CURRENT:
#if LOG
	    WPRINTF(L"\tSCRIPTTHREADID_CURRENT\n");
#endif
	    return E_UNEXPECTED;

	default:
#if LOG
	    WPRINTF(L"\tdefault\n");
#endif
	    return E_INVALIDARG;
    }
    invariant();
    ENTRYLOG(L"EXIT: InterruptScriptThread(%d,x%x, dwFlags=x%x) m_pccGlobal %x\n",stidThread,pexcepinfo,dwFlags,m_pccGlobal);
    return S_OK;
} // InterruptScriptThread


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Clone()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP COleScript::Clone(/*[out]*/ IActiveScript** ppNew)
{
    if (NULL == ppNew)
        return E_POINTER;

    DPF(L"COM: WARNING! Clone() called (NYI)\n");
    invariant();
    return E_NOTIMPL;
} // Clone



//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// *** IActiveScriptParse Methods ***
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// InitNew()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP COleScript::InitNew()
{
    int dummy;

    ENTRYLOG(L"COM: ENTER InitNew()\n");
    invariant();
    mem.setStackBottom((void *)&dummy);
    //
    // Bail if we're already loaded...
    //
    if (m_fPersistLoaded)
        return E_UNEXPECTED;

    m_fPersistLoaded = true;

    if (NULL != m_pScriptSite)
    {
        _ChangeScriptState(SCRIPTSTATE_INITIALIZED);
    }

    ENTRYLOG(L"COM: EXIT InitNew()\n");
    return S_OK;
} // InitNew


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// AddScriptlet()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP COleScript::AddScriptlet(/*[in] */ LPCOLESTR   pcwszDefaultName,
                                      /*[in] */ LPCOLESTR   pcwszCode,
                                      /*[in] */ LPCOLESTR   pcwszItemName,
                                      /*[in] */ LPCOLESTR   pcwszSubItemName,
                                      /*[in] */ LPCOLESTR   pcwszEventName,
                                      /*[in] */ LPCOLESTR   pcwszDelimiter,
                                      /*[in] */ DWORD       dwSourceContext,
                                      /*[in] */ ULONG       ulStartingLineNumber,
                                      /*[in] */ DWORD       dwFlags,
                                      /*[out]*/ BSTR*       pbszName,
                                      /*[out]*/ EXCEPINFO*  pExcepInfo)
{
    mem.setStackBottom((void *)&pExcepInfo);
    invariant();
    //WPRINTF(L"ulStartingLineNumber = %u\n", ulStartingLineNumber);
#ifdef DISABLE_EVENT_HANDLING
    DPF(L"COM: ERROR AddScriptlet(\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", %l, %l, 0x%08lx). Event handling support disabled!\n",
                                  pcwszDefaultName, pcwszCode, pcwszItemName,
                                  pcwszSubItemName, pcwszEventName, pcwszDelimiter);

    UNREFERENCED_PARAMETER(pcwszDefaultName);
    UNREFERENCED_PARAMETER(pcwszCode);
    UNREFERENCED_PARAMETER(pcwszItemName);
    UNREFERENCED_PARAMETER(pcwszSubItemName);
    UNREFERENCED_PARAMETER(pcwszEventName);
    UNREFERENCED_PARAMETER(pcwszDelimiter);
    UNREFERENCED_PARAMETER(dwSourceContext);
    UNREFERENCED_PARAMETER(ulStartingLineNumber);
    UNREFERENCED_PARAMETER(pbszName);
    UNREFERENCED_PARAMETER(pExcepInfo);
    
    return E_NOTIMPL;
#else

    ENTRYLOG(L"COM: ENTER AddScriptlet(\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", %l, %l, 0x%08lx)\n",
                                  pcwszDefaultName, pcwszCode, pcwszItemName,
                                  pcwszSubItemName, pcwszEventName, pcwszDelimiter);

    UNREFERENCED_PARAMETER(ulStartingLineNumber);

    //
    // Not presently doing anything useful with these...
    //
    UNREFERENCED_PARAMETER(pcwszDelimiter);

    UNREFERENCED_PARAMETER(pcwszDefaultName);
    UNREFERENCED_PARAMETER(pbszName);
    UNREFERENCED_PARAMETER(pExcepInfo);

    //
    // These are the only documented flags, so bail if any others are passed in
    //
    if (dwFlags & ~(SCRIPTTEXT_ISVISIBLE | SCRIPTTEXT_ISPERSISTENT | SCRIPTTEXT_HOSTMANAGESSOURCE))
    {
        DPF(L"COM: ERROR AddScriptlet() : received unexpected flags.\n");
        return E_INVALIDARG;
    }

    //
    // Clear the out-parameters if they're asked for
    //
    if (NULL != pbszName)
        *pbszName = NULL;

    if (NULL != pExcepInfo)
        ZeroMemory(pExcepInfo, sizeof(EXCEPINFO));

    SNamedItemData* pnid = NULL;

    //
    // If we don't get a named item, we wouldn't be able to check for
    // SCRIPTITEM_ISSOURCE, or otherwise know what object it is that should
    // be firing the magical event the caller was requesting to add code for.
    //
    pnid = _FindNamedItem(pcwszItemName);

    //
    // If we can't find the named item referenced, or the named item was
    // not added as an event source, then we can't continue.
    //
    if (NULL == pnid || !(pnid->dwAddFlags & SCRIPTITEM_ISSOURCE))
    {
        DPF(L"COM: ERROR AddScriptlet() : nameditem not found or not added as ISSOURCE.\n");
        return E_INVALIDARG;
    }

    WCHAR*  pwszHandlerName = NULL;
    HRESULT hr = E_UNEXPECTED;

    hr = _BuildHandlerName(pcwszItemName, pcwszSubItemName, &pwszHandlerName);
    if (FAILED(hr))
    {
        DPF(L"COM: ERROR AddScriptlet() : BuildHandlerName failure (0x%08lx)\n", hr);
        return hr;
    }

    WCHAR*  pwszFuncName = NULL;
    WCHAR*  pwszFunc = NULL;

    hr = _BuildFunctionNameAndBody(pwszHandlerName, pcwszEventName, pcwszCode,
                                   pcwszDelimiter, &pwszFuncName, &pwszFunc);
    if (FAILED(hr))
    {
        DPF(L"COM: ERROR AddScriptlet() : BuildFunctionNameAndBody failure (0x%08lx)\n", hr);
        return hr;
    }

    hr = _AddScriptTextToNamedItem(pnid, pwszFunc, dwFlags, pExcepInfo, dwSourceContext);
    pwszFunc = NULL;

    if (FAILED(hr))
    {
        DPF(L"COM: ERROR AddScriptlet() : _AddScriptTextToNamedItem failure (0x%08lx)\n", hr);
        return hr;
    }

    IDispatch*  pdispSink = NULL;

    hr = _BuildScriptDispatchWrapper(pnid, &pdispSink, dwSourceContext);
    if (FAILED(hr))
    {
        DPF(L"COM: ERROR AddScriptlet() : _BuildScriptDispatchWrapper failure (0x%08lx)\n", hr);
        return hr;
    }

    CDispatchEventSink* pSink = NULL;

    hr = _GetNamedItemEventSink(pnid, pcwszSubItemName, pwszHandlerName, &pSink);
    if (FAILED(hr))
    {
        DPF(L"COM: ERROR AddScriptlet() : _GetNamedItemEventSink failure (0x%08lx)\n", hr);
        return hr;
    }

    //
    // Above call should have found or created a sink if it succeeded.
    //
    assert(NULL != pSink);

    hr = pSink->AttachEvent(pcwszEventName, pwszFuncName);
    if (FAILED(hr))
    {
        DPF(L"COM: ERROR AddScriptlet() : AttachEvent failure (0x%08lx)\n", hr);
        return hr;
    }

    if (NULL != pbszName)
    {
        *pbszName = SysAllocString(pwszFuncName);
        if (NULL == *pbszName)
            return E_OUTOFMEMORY;
    }

    ENTRYLOG(L"COM: EXIT AddScriptlet()\n");
    return S_OK;

#endif // DISABLE_EVENT_HANDLING
} // AddScriptlet


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// ParseScriptText()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP COleScript::ParseScriptText(/*[in] */ LPCOLESTR   pcwszCode,
                                         /*[in] */ LPCOLESTR   pcwszItemName,
                                         /*[in] */ IUnknown*   punkContext,
                                         /*[in] */ LPCOLESTR   pcwszDelimiter,
                                         /*[in] */ DWORD       dwSourceContext,
                                         /*[in] */ ULONG       ulStartingLineNumber,
                                         /*[in] */ DWORD       dwFlags,
                                         /*[out]*/ VARIANT*    pvarResult,
                                         /*[out]*/ EXCEPINFO*  pexcepinfo)
{
    invariant();
    mem.setStackBottom((void *)&pexcepinfo);
    ENTRYLOG(L"COM: ENTER ParseScriptText(this = x%x)\n", this);
    //WPRINTF(L"Code = '%s'\n", pcwszCode);
#if 0
	WPRINTF(L"\tdwSourceContext = %x\n", dwSourceContext);
	WPRINTF(L"\tulStartingLineNumber = %d\n", ulStartingLineNumber);
	WPRINTF(L"\tpunkContext = %x\n", punkContext);
#endif
    // ENTRYLOG(L"COM: ENTER ParseScriptText(\"%s\", \"%s\", punkContext, \"%s\", dwSourceContext, ulStartingLineNumber, 0x%08lx)\n",
    //     pcwszCode, pcwszItemName, pcwszDelimiter, dwFlags);
    // punkContext, dwSourceContext and ulStartingLineNumber
    // are all debugger concepts, so we ignore them.
    //
    UNREFERENCED_PARAMETER(punkContext);
    UNREFERENCED_PARAMETER(ulStartingLineNumber);
    UNREFERENCED_PARAMETER(pcwszDelimiter);

    if (NULL == pcwszCode)
        return E_INVALIDARG;

    if (SCRIPTSTATE_UNINITIALIZED == m_ssState ||
        SCRIPTSTATE_CLOSED == m_ssState)
    {
        DPF(L"COM: ERROR ParseScriptText() : unexpected SCRIPTSTATE\n");
        return E_UNEXPECTED;
    }

    //
    // Must init the result value VARIANT (NOT clear, because this is an out-only param)
    //
    if (NULL != pvarResult)
        VariantInit(pvarResult);

    SNamedItemData* pnid = NULL;

    //
    // If a NamedItem was specified, is it a valid one?
    //
    if (NULL != pcwszItemName && pcwszItemName[0] != L'\0')
    {
        pnid = _FindNamedItem(pcwszItemName);

        //
        // Unable to find named item for script
        //
        if (NULL == pnid)
        {
            DPF(L"COM: ERROR ParseScriptText() : nameditem not found\n");
            return E_INVALIDARG;
        }

        //
        // Now let's see if it "has code." This relates to the SCRIPTITEM_CODEONLY
        // meaning of "code" which means a namespace that other script code can
        // be executed in. If the caller is attempting to execute code in a
        // namespace that does not have code, then the call is invalid.
        //
        if (!pnid->fHasCode)
        {
            DPF(L"COM: ERROR ParseScriptText() : nameditem does not \"have code\".\n");
            return E_INVALIDARG;
        }
    }

    int dResult = 0;
    ErrInfo errinfo;                // Dscript error info

    assert(NULL != m_pProgram);

    dResult = m_pProgram->parse((dchar*)pcwszCode,
                                (wcslen(pcwszCode) + 1) * sizeof(dchar),
                                NULL, &errinfo);
    if (NULL != dResult)
    {
        DPF(L"COM: ERROR ParseScriptText() : parse error : \"%s\"\n", errinfo.message);
        return HandleScriptError(&errinfo, pexcepinfo, dwSourceContext);
    }

    GC_LOG();
    SParsedScriptData*  pCode = new SParsedScriptData;
    if (NULL == pCode)
        return E_OUTOFMEMORY;

    //WPRINTF(L"\tnew pCode = %x\n", pCode);
    pCode->pfdCode = m_pProgram->globalfunction;
    pCode->dwFlags = dwFlags;
    pCode->dwSourceContext = dwSourceContext;

    if (NULL != pnid)
        pCode->uModuleContext = pnid->uModuleNumber;
    else
        pCode->uModuleContext = GLOBAL_MODULE;

    //
    // Can we execute the script now?
    //
    if (SCRIPTSTATE_STARTED == m_ssState ||
        SCRIPTSTATE_CONNECTED == m_ssState ||
        SCRIPTSTATE_DISCONNECTED == m_ssState)
    {
//        DPF(L"COM: ParseScriptText() : executing code immediately.\n");
        //
        // Execute the script
        //
        dResult = _ExecuteParsedScriptBlock(pCode, NULL, pnid, 0, pvarResult, &errinfo);

        //
        // Need to save the parsed script in our list of script blocks
        // if it's persistent.
        //
        if (dwFlags & SCRIPTTEXT_ISPERSISTENT)
        {
	    //WPRINTF(L"\t1adding persistent script %x\n", pCode);
            pCode->fExecuted = true;        // Mark block as already executed
            pCode->pNext = m_pParsedScripts;
            m_pParsedScripts = pCode;
        }
        else
        {
            delete pCode;
            pCode = NULL;
        }

        if (0 != dResult)
        {
            DPF(L"COM: ERROR ParseScriptText() : script error encountered : \"%s\"\n", errinfo.message);
            return HandleScriptError(&errinfo, pexcepinfo, dwSourceContext);
        }
    }
    //
    // Can't run the script now, so save it in our list of parsed scripts.
    // Later when we switch states, we'll come through and take care of it.
    //
    else
    {
	//WPRINTF(L"\t3adding persistent? script %x, %x\n", pCode, pCode->dwFlags & SCRIPTTEXT_ISPERSISTENT);
        pCode->fExecuted = false;           // Mark block as pending execution
        pCode->pNext = m_pParsedScripts;
        m_pParsedScripts = pCode;
    }

    ENTRYLOG(L"COM: EXIT ParseScriptText()\n");
    return S_OK;
} // ParseScriptText



//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// *** IActiveScriptParseProcedure Methods ***
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/***************************************************
//
// IActiveScriptParseProcedure
//
//

 This interface is not documented by Microsoft. These comments
 about it are from microsoft.public.scripting.hosting:


 If the script engine doesn't support IActiveScriptParseProcedure, then you'll have
 to fall back on the IActiveScriptParse::AddScriptlet method, just as IE does.
 However, this won't get your events firing until after the immediate script has
 run during the transition through SCRIPTSTATE_STARTED on the way to
 SCRIPTSTATE_CONNECTED.  This is why you need to wrap your immediate script in a
 sub.

 The final method of adding an event handler to script is the
 IActiveScriptParseProcedure::ParseProcedureText method (see other thread).
 If a script engine supports this interface (and VBScript and JScript do),
 IE will use this interface instead of IActiveScriptParse::AddScriptlet to add
 it's event handlers.  It works just like AddScriptlet, except it gives back an
 anonymous IDispatch pointer that can be used to fire the event.  This sometimes
 leads to a cleaner host implmentation, particularly if the host wants to set
 the engine to the SCRIPTSTATE_STARTED state, rather than the
 SCRIPTSTATE_CONNECTED state.

 The IActiveScriptParseProcedure interface is used by Internet Explorer to more
 easily handle button clicks and other events.  Essentially, it the equivalent of
 combining a call to IActiveScriptParse::ParseScriptText and
 IActiveScript::GetScriptDispatch.
 
 From the SamScrpt sample script engine, here's a brief write-up on
 ParseProcedureText, IActiveScriptParseProcedure's only method.
 
  ParseProcedureText -- This method allows an Active Script Host to use
  IDispatch-style function pointers to fire methods instead of using the more
  difficult method of Connection Points.  It parses a scriplet and wraps it in
  an anonymous IDispatch interface, which the host can use in lieu of
  Connection Points to handle events.

  Parameters: pcwszCode -- Address of the script code to evaluate
              pcwszFormalParams -- Address of any formal parameters to the
                                  scriptlet. (ignored)
              pcwszProcedureName -- Name of the event
              pcwszItemName --  The named item that gives this scriptlet its context.
              punkContext -- Address of the context object.  This item is
                            reserved for the debugger.
              pcwszDelimiter -- Address of the delimiter the host used to detect
                              the end of the scriptlet.
              dwSourceContextCookie -- Application defined value for debugging
              ulStartingLineNumber -- zero-based number defining where parsing
                                      began.
              dwFlags -- SCRIPTPROC_HOSTMANAGESSOURCE
                        SCRIPTPROC_IMPLICIT_THIS
                        SCRIPTPROC_IMPLICIT_PARENTS
                        SCRIPTPROC_ALL_FLAGS
              ppDisp -- Address of the pointer that receives the IDispatch
                        pointer the host uses to call this event.
  Returns: S_OK
          DISP_E_EXCEPTION
          E_INVALIDARG
          E_POINTER
          E_NOTIMPL
          E_UNEXPECTED
          OLESCRIPT_E_SYNTAX

 -- Joel Alley Microsoft Developer Support


***************************************************/

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// ParseProcedureText()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
STDMETHODIMP COleScript::ParseProcedureText(/*[in] */ LPCOLESTR     pcwszCode,
                                            /*[in] */ LPCOLESTR     pcwszFormalParams,
                                            /*[in] */ LPCOLESTR     pcwszProcedureName,
                                            /*[in] */ LPCOLESTR     pcwszItemName,
                                            /*[in] */ IUnknown*     punkContext,
                                            /*[in] */ LPCOLESTR     pcwszDelimiter,
                                            /*[in] */ DWORD         dwSourceContextCookie,
                                            /*[in] */ ULONG         ulStartingLineNumber,
                                            /*[in] */ DWORD         dwFlags,
                                            /*[out]*/ IDispatch**   ppDisp)
{
    ENTRYLOG(L"COM: ENTER ParseProcedureText(ulStartingLineNumber = %u, dwFlags = x%x)\n", ulStartingLineNumber, dwFlags);

#if 0
    WPRINTF(L"\tpcwszCode = '%s'\n", pcwszCode);
    WPRINTF(L"\tpcwszFormalParams = '%s'\n", pcwszFormalParams);
    WPRINTF(L"\tpcwszProcedureName = '%s'\n", pcwszProcedureName);
    WPRINTF(L"\tpcwszItemName = '%s'\n", pcwszItemName);
#endif

    invariant();
    mem.setStackBottom((void *)&ppDisp);

    UNREFERENCED_PARAMETER(pcwszFormalParams);
    UNREFERENCED_PARAMETER(punkContext);
    UNREFERENCED_PARAMETER(pcwszDelimiter);
    UNREFERENCED_PARAMETER(ulStartingLineNumber);


    if (NULL == pcwszCode || NULL == ppDisp || *ppDisp)
        return E_INVALIDARG;

    *ppDisp = NULL;         // Clear the out-parameter

    if (dwFlags & ~(SCRIPTPROC_HOSTMANAGESSOURCE | SCRIPTPROC_IMPLICIT_THIS | SCRIPTPROC_IMPLICIT_PARENTS))
    {
        DPF(L"COM: ERROR ParseScriptText() : unrecognized flags\n");
        return E_INVALIDARG;
    }

    SNamedItemData* pnid = NULL;

    UINT    uModule = GLOBAL_MODULE;
    HRESULT hr = E_UNEXPECTED;
    int  dResult = 0;
    ErrInfo errinfo;

    if (NULL != pcwszItemName && L'\0' != pcwszItemName[0])
    {
        pnid = _FindNamedItem(pcwszItemName);

        //
        // If a named item was given but we didn't find a record for it, that's an error.
        //
        if (NULL == pnid)
        {
            DPF(L"COM: ERROR ParseProcedureText() : nameditem not found\n");
            return E_UNEXPECTED;
        }

        uModule = pnid->uModuleNumber;
    }

    assert(NULL != m_pProgram);

    static const wchar_t defaultname[] = L"__ParsedProcedure";
    wchar_t procname[sizeof(defaultname)/sizeof(wchar_t) + 5 + 2];
    if (!pcwszProcedureName || !*pcwszProcedureName)
    {
	// Create our own procedure name
	SWPRINTF(procname, sizeof(procname) / sizeof(wchar_t) - 1,
		L"%s%i", defaultname, m_dwRuntimeUniqueNumber);
	m_dwRuntimeUniqueNumber++;
	pcwszProcedureName = procname;
    }

    int len = 25 + wcslen(pcwszProcedureName) + wcslen(pcwszCode) + 1;
    LPOLESTR pwszWrapped = (WCHAR*)mem.malloc(sizeof(pcwszCode[0]) * len);
    if (NULL == pwszWrapped)
        return E_OUTOFMEMORY;

    SWPRINTF(pwszWrapped, len, L"function %s(){%s}", pcwszProcedureName, pcwszCode);

    FunctionDefinition *fd;
    dResult = m_pProgram->parse((dchar*)pwszWrapped,
                                (wcslen(pwszWrapped) + 1) * sizeof(dchar),
                                &fd, &errinfo);
    mem.free(pwszWrapped);

    if (0 != dResult)
    {
        DPF(L"COM: ERROR ParseScriptText() : parse error : \"%s\"\n", errinfo.message);
        return HandleScriptError(&errinfo, NULL, dwSourceContextCookie);
    }

    GC_LOG();
    CParsedProcedure*   pCode = new CParsedProcedure;
    if (NULL == pCode)
        return E_OUTOFMEMORY;

    hr = pCode->Initialize(this, fd, pnid,
                           SCRIPTPROC_PRIV_IMPLEMENTATIONCODE | dwFlags,
			   dwSourceContextCookie);
    if (FAILED(hr))
    {
        DPF(L"COM: ERROR ParseScriptText() : ParsedProcedure initialization failed (0x%08lx)\n", hr);
        return E_FAIL;
    }

    hr = pCode->QueryInterface(IID_IDispatch, (void**)ppDisp);
    pCode->Release();       // Release the ref we had from creating the object

    ENTRYLOG(L"COM: EXIT ParseProcedureText() = x%x\n", hr);
    return hr;
} // ParseProcedureText



//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// *** Helper Methods ***
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _AddScriptTextToNamedItem()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::_AddScriptTextToNamedItem(SNamedItemData* pnid,
                                              const WCHAR* pcwszCode,
                                              DWORD dwFlags,
                                              EXCEPINFO* pexcepinfo,
					      DWORD dwSourceContext)
{
    ENTRYLOG(L"COM: ENTER _AddScriptTextToNamedItem(...)\n");
    invariant();

    int  dResult = 0;
    ErrInfo errinfo;        // Dscript error info

    assert(NULL != m_pProgram);

    dResult = m_pProgram->parse((dchar*)pcwszCode,
                                (wcslen(pcwszCode) + 1) * sizeof(dchar),
                                NULL, &errinfo);
    if (0 != dResult)
    {
        DPF(L"COM: ERROR _AddScriptTextToNamedItem() : parser error : \"%s\"\n", errinfo.message);
        return HandleScriptError(&errinfo, pexcepinfo, dwSourceContext);
    }

    GC_LOG();
    SParsedScriptData*  pCode = new SParsedScriptData;
    if (NULL == pCode)
        return E_OUTOFMEMORY;

    pCode->pfdCode = m_pProgram->globalfunction;
    pCode->dwFlags = dwFlags;
    pCode->dwSourceContext = dwSourceContext;

    if (NULL != pnid)
        pCode->uModuleContext = pnid->uModuleNumber;
    else
        pCode->uModuleContext = GLOBAL_MODULE;

    //
    // Can we execute the script now?
    //
    if (SCRIPTSTATE_STARTED == m_ssState ||
        SCRIPTSTATE_CONNECTED == m_ssState ||
        SCRIPTSTATE_DISCONNECTED == m_ssState)
    {
        DPF(L"COM: _AddScriptTextToNamedItem() : executing immediately\n");
        //
        // Execute the script
        //
        dResult = _ExecuteParsedScriptBlock(pCode, NULL, pnid, 0, NULL, &errinfo);

        //
        // Need to save the parsed script in our list of script blocks
        // if it's persistent.
        //
        if (dwFlags & SCRIPTTEXT_ISPERSISTENT)
        {
	    //WPRINTF(L"\t2adding persistent script %x\n", pCode);
            pCode->fExecuted = true;        // Mark block as already executed
            pCode->pNext = m_pParsedScripts;
            m_pParsedScripts = pCode;
        }
        else
        {
            pCode->pNext = NULL;
            delete pCode;
            pCode = NULL;
        }

        if (0 != dResult)
        {
            DPF(L"COM: ERROR _AddScriptTextToNamedItem() : execution error : \"%s\"\n", errinfo.message);
            return HandleScriptError(&errinfo, pexcepinfo, dwSourceContext);
        }
    }
    //
    // Can't run the script now, so save it in our list of parsed scripts.
    // Later when we switch states, we'll come through and take care of it.
    //
    else
    {
	//WPRINTF(L"\t4adding non-persistent script %x\n", pCode);
        pCode->fExecuted = false;           // Mark block as pending execution
        pCode->pNext = m_pParsedScripts;
        m_pParsedScripts = pCode;
    }

    ENTRYLOG(L"COM: EXIT _AddScriptTextToNamedItem()\n");
    return S_OK;
} // _AddScriptTextToNamedItem


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _BuildScriptDispatchWrapper()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::_BuildScriptDispatchWrapper(SNamedItemData* pnid,
                                                IDispatch** ppDisp,
						DWORD dwSourceContext)
{
    ENTRYLOG(L"COM: ENTER _BuildScriptDispatchWrapper(pnid = x%x, dwSourceContext = x%x)\n", pnid, dwSourceContext);
    invariant();

    IDispatch**     ppDispWrapper = NULL;
    CallContext*    pcc = NULL;
    HRESULT         hr = E_UNEXPECTED;
    bool	    fGlobal;

    if (NULL == pnid || pnid->uModuleNumber == GLOBAL_MODULE)
    {
        ppDispWrapper = &m_pGlobalModuleDisp;
	fGlobal = true;
    }
    else
    {
        ppDispWrapper = &pnid->pModuleDisp;
	fGlobal = false;
    }

    if (NULL == *ppDispWrapper)
    {
        if (fGlobal)
        {
	    //pcc = _CopyGlobalContext();
	    pcc = m_pccGlobal;
        }
        else
        {
            pcc = _BuildNamedItemContext(pnid);
        }

        if (NULL == pcc)
            return E_OUTOFMEMORY;

	GC_LOG();
        CNamedItemDisp* pNewDisp = new CNamedItemDisp();
        if (NULL == pNewDisp)
            return E_OUTOFMEMORY;

        //
        // Initialize the namespace dispatch wrapper
        //
        hr = pNewDisp->Initialize(this, pcc, fGlobal, dwSourceContext);
        if (FAILED(hr))
        {
            pNewDisp->Release();
            return E_FAIL;
        }

        hr = pNewDisp->QueryInterface(IID_IDispatch, (void**)ppDispWrapper);
        assert(SUCCEEDED(hr));

        pNewDisp->Release();
        pNewDisp = NULL;
        if (FAILED(hr))
        {
            return E_FAIL;
        }
    }

    //
    // Either we had a dispatch wrapper, or we just created one.
    //
    assert(NULL != *ppDispWrapper);

    hr = (*ppDispWrapper)->QueryInterface(IID_IDispatch, (void**)ppDisp);
    assert(SUCCEEDED(hr));

    ENTRYLOG(L"COM: EXIT _BuildScriptDispatchWrapper()\n");
    return hr;
} // _BuildScriptDispatchWrapper


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _ExecutePendingScripts()
//
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::_ExecutePendingScripts()
{
    ENTRYLOG(L"COM: ENTER _ExecutePendingScripts()\n");
    invariant();

    SParsedScriptData*  pCode = m_pParsedScripts;
    SParsedScriptData** ppLink = &m_pParsedScripts;

    mem.setStackBottom((void *)&pCode);
    while (NULL != pCode)
    {
        if (!pCode->fExecuted)
        {
            SNamedItemData* pnid = NULL;
            
            if (pCode->uModuleContext != GLOBAL_MODULE)
            {
                pnid = _FindNamedItem(pCode->uModuleContext);

                //
                // ASSERT: If we can't find information on the namespace that this
                // code was added for, we're in trouble.  There's no way we
                // should have added the record if we didn't know the namespace
                // existed, and there's no way to delete a namespace, so it
                // should have still been here.
                //
                assert(NULL != pnid);
                return E_FAIL;
            }

            //
            // Execute the script
            //
            ErrInfo errinfo;
            int dResult = _ExecuteParsedScriptBlock(pCode, NULL, pnid, 0, NULL, &errinfo);

	    // Save so it survives the deletion of pCode
	    DWORD dwSourceContext = pCode->dwSourceContext;

            //
            // Need to save the parsed script in our list of script blocks
            // if it's persistent, otherwise we should just delete it.
            //
            if (pCode->dwFlags & SCRIPTTEXT_ISPERSISTENT)
            {
                pCode->fExecuted = true;        // Mark block as already executed
                ppLink = &pCode->pNext;
                pCode = pCode->pNext;
            }
            else
            {
                //
                // Unlink this record
                //
                *ppLink = pCode->pNext;
                pCode->pNext = NULL;
                delete pCode;
                pCode = *ppLink;
            }

            if (0 != dResult)
            {
                DPF(L"COM: ERROR _ExecutePendingScripts() : execution error : \"%s\"\n", errinfo.message);
                return HandleScriptError(&errinfo, NULL, dwSourceContext);
            }
        }
        else
        {
            ppLink = &pCode->pNext;
            pCode = pCode->pNext;
        }
    }

    ENTRYLOG(L"COM: EXIT _ExecutePendingScripts()\n");
    return S_OK;
} // _ExecutePendingScripts


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _CopyCallContext()
//
// Creates a new CallContext that's a copy
// of the context in pccRight.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
CallContext* COleScript::_CopyCallContext(CallContext* pccRight)
{
    GC_LOG();
    CallContext*  pccNew = new CallContext();
    if (pccNew)
    {
	memset(pccNew, 0, sizeof(CallContext));
	GC_LOG();
	pccNew->scope = new Array();
	if (NULL == pccNew->scope)
	{
	    pccNew = NULL;
	    return NULL;
	}

	pccNew->variable = pccRight->variable;
	pccNew->scoperoot = pccRight->scoperoot;
	pccNew->globalroot = pccRight->globalroot;
	pccNew->global = pccRight->global;
	pccNew->lastnamedfunc = pccRight->lastnamedfunc;
	pccNew->prog = pccRight->prog;

	//
	// Copy the current scope chain so we can restore it later.
	// We're going to modify it and insert the "this" for the
	// NamedItem namespace we're going to evaluate in.
	//
	pccNew->scope->reserve(pccRight->scope->allocdim);
	assert(pccRight->scope->dim <= pccRight->scope->allocdim);

	memcpy(pccNew->scope->data, pccRight->scope->data,
	       pccRight->scope->dim * sizeof(pccRight->scope->data[0]));
	pccNew->scope->dim = pccRight->scope->dim;
    }

    return pccNew;
} // _CopyCallContext


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _CopyGlobalContext()
//
// Creates a new CallContext that's a copy
// of the global context in m_pccGlobal.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
CallContext* COleScript::_CopyGlobalContext()
{
    invariant();
    return _CopyCallContext(m_pccGlobal);
} // _CopyGlobalContext


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// SetGlobalProgramContext()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void COleScript::SetGlobalProgramContext(CallContext* pccNew)
{
    invariant();
    assert(NULL != pccNew);

    m_pProgram->callcontext = pccNew;
}


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// SwitchGlobalProgramContext()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
CallContext* COleScript::SwitchGlobalProgramContext(CallContext* pccNew)
{
    invariant();
    CallContext* pccSave = m_pProgram->callcontext;

    //
    // Now that we've saved a complete copy of the CallContext in m_pProgram,
    // overwrite the m_pProgram context with the one we need to use for
    // this namespace. We'll replace it later with pccSave. We have to do this
    // because even though we pass the appropriate CallContext* into IR::call,
    // some functions in the engine get their CallContext directly from
    // Program::getProgram()->callcontext. :-(
    //
    SetGlobalProgramContext(pccNew);
    return pccSave;
} // SwitchGlobalProgramContext


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// SwitchToOurGlobalProgram()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
Program* COleScript::SwitchToOurGlobalProgram()
{
    invariant();
    Program* pSave = Program::getProgram();

    Program::setProgram(m_pProgram);
    return pSave;
} // SwitchToOurGlobalProgram


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// RestoreSavedGlobalProgram()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void COleScript::RestoreSavedGlobalProgram(Program* pProgram)
{
    invariant();
    Program::setProgram(pProgram);
} // RestoreSavedGlobalProgram


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _BuildNamedItemContext()
//
// Create a new CallContext that contains
// the namespace for the given NamedItem.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
CallContext* COleScript::_BuildNamedItemContext(SNamedItemData* pnid)
{
    ENTRYLOG(L"COM: ENTER _BuildNamedItemContext()\n");
    invariant();
    assert(NULL != pnid);

    CallContext*  pccNew = _CopyGlobalContext();
    if (NULL == pccNew)
        return NULL;

    if (NULL != pnid->pWrapper)
    {
	GC_LOG();
        Dobject*    actobj = new Dobject(NULL);
        //
        // Now put the COM object that corresponds to this NamedItem
        // on the scope stack as the topmost item.
        //
        pccNew->scope->push(pnid->pWrapper);
        pccNew->scope->push(actobj);

        pccNew->variable = actobj;
        pccNew->global = actobj;

        //
        // Increment the scoperoot counter, indicating these scope objects
        // are part of the scope chain that should be preserved between
        // function calls, but do not increment globalroot, because named
        // item namespace objects are not in the global scope.
        //
        pccNew->scoperoot += 2;
    }
    else
    {
    }

    ENTRYLOG(L"COM: EXIT  _BuildNamedItemContext()\n");
    return pccNew;
} // _BuildNamedItemContext


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _ExecuteParsedScriptBlock()
//
// Execute the block of parsed script described
// by pCode in the context of the NamedItem pnid.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
int COleScript::_ExecuteParsedScriptBlock(SParsedScriptData* pCode,
                                          Dobject* poThis, SNamedItemData* pnid,
                                          DWORD dwFlags, VARIANT* pvarResult,
                                          ErrInfo *perrinfo)
{
    invariant();
    //
    // ASSERT: We should only be called when we're at SCRIPTSTATE_STARTED or later,
    //         and we can only get there if we got a site (so we could be INITIALIZED).
    //
    assert(NULL != m_pScriptSite);

    //
    // ASSERT: The caller had better give us some code to work with here...
    //
    assert(NULL != pCode);

    //
    // Now, if the script is not for the global module, we should have
    // been given a pnid (or possibly if it is in the context of an item
    // that IS in the global module...).
    //
    if (NULL != pnid)
    {
        // Double-check that the namespace "has code" so that
        // script can be executed in the context of this NamedItem.
        //
        assert(pnid->fHasCode);

        //
        // These better match, or the caller gave us a wrong NamedItem!
        //
        assert(pnid->uModuleNumber == pCode->uModuleContext);
    }

    return ExecuteParsedScript(pCode->pfdCode, poThis, pnid, dwFlags, pvarResult, perrinfo);
} // _ExecuteParsedScriptBlock


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// ExecuteParsedScript()
//
// Execute the block of parsed script described
// by pfd in the context of the NamedItem pnid.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
int COleScript::ExecuteParsedScript(FunctionDefinition* pfdCode, Dobject* poThis,
                                    SNamedItemData* pnid, DWORD dwFlags, VARIANT* pvarResult, ErrInfo *perrinfo)
{
    ENTRYLOG(L"COM: ENTER ExecuteParsedScript()\n");
    //FuncLog funclog(L"ExecuteParsedScript()");
    //PerfTimer pt(L"ExecuteParsedScript()");
    invariant();
    mem.setStackBottom((void *)&perrinfo);
    Value *dResult = NULL;
    Value*      pvalLocals = NULL;

    //
    // ASSERT: We should only be called when we're at SCRIPTSTATE_STARTED or later,
    //         and we can only get there if we got a site (so we could be INITIALIZED).
    //
    assert(NULL != m_pScriptSite);

    //
    // ASSERT: The caller had better give us some code to work with here...
    //
    assert(NULL != pfdCode);

    // Allocate space for locals outside the EnterScript/ExitScript in case it fails
    int argc = pfdCode->nlocals;
    if (0 != argc)
    {
        if (argc >= 32 || (pvalLocals = (Value *)alloca(sizeof(Value) * argc)) == NULL)
        {
	    // Not enough stack space - use mem
	    pvalLocals = (Value *)mem.malloc(sizeof(Value) * argc);
        }
    }


    //
    // Mark the script as executing.
    //
    // WARNING!!! You CANNOT RETURN from this function UNLESS YOU CALL ExitScript().
    //            We must always match Enter/Leave callbacks to the script site.
    //
    EnterScript();

    CallContext* pccCall = NULL;
    CallContext* pccSave = NULL;

    Value       valRet;
    Program*    program_save;

    m_pProgram->initContext();

    //
    // Now, if the script is not for the global module, we should have
    // been given a pnid (or possibly if it is in the context of an item
    // that IS in the global module...).
    //
    if (NULL != pnid && GLOBAL_MODULE != pnid->uModuleNumber)
    {
        pccCall = _BuildNamedItemContext(pnid);
    }
    else
    {
        pccCall = _CopyGlobalContext();
    }

    if (NULL == pccCall)
    {
        perrinfo->message = L"Failure allocating memory for CallContext";
	ExitScript();
        return 1;
    }

    // Instantiate global variables as properties of global
    // object with 0 attributes
    pfdCode->instantiate(pccCall->scope, pccCall->scope->dim, pccCall->variable, 0);

    pccCall->scope->reserve(pfdCode->withdepth + 1);

    pccSave = m_pProgram->callcontext;
    m_pProgram->callcontext = pccCall;

    program_save = Program::getProgram();
    Program::setProgram(m_pProgram);

    if (NULL == poThis)
        poThis = (Dobject*)(pccCall->scope->data[pccCall->scoperoot-1]);
    else
    {
        if ((dwFlags & (SCRIPTPROC_PRIV_IMPLEMENTATIONCODE | SCRIPTPROC_IMPLICIT_THIS))
                == (SCRIPTPROC_PRIV_IMPLEMENTATIONCODE | SCRIPTPROC_IMPLICIT_THIS))
        {
            pccCall->scope->push(poThis);
            pccCall->scoperoot++;
        }
    }

#if LOG
    WPRINTF(L"+IR::call: this=%x, program = %x\n", this, m_pProgram);
#endif
    Value::copy(&valRet, &vundefined);
    dResult = (Value *)IR::call(m_pProgram->callcontext, /*pccCall,*/
                                poThis,
                                pfdCode->code,
                                &valRet, pvalLocals);
#if LOG
    WPRINTF(L"-IR::call done\n");
#endif
    unsigned linnum = m_pProgram->callcontext->linnum;
    m_pProgram->callcontext->linnum = 0;

    Program::setProgram(program_save);
    m_pProgram->callcontext = pccSave;

    // Kill the call context we created for this call and restore the
    // one in m_pProgram to be whatever it was when we got here.
    {
        pccCall->global = NULL;
        pccCall->lastnamedfunc = NULL;
        pccCall->variable = NULL;
        pccCall->scope->zero();
    }

    ExitScript();

    if (dResult)
    {
	dResult->getErrInfo(perrinfo, linnum);
        return 1;
    }

    if (pvarResult)
    {
        ValueToVariant(pvarResult, &valRet);
    }
    clearStack();
    ENTRYLOG(L"COM: EXIT ExecuteParsedScript()\n");
    return 0;
} // ExecuteParsedScript


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// EnterScript()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void COleScript::EnterScript()
{
    invariant();
    int dummy;
    mem.setStackBottom((void *)&dummy);
    m_stsState = SCRIPTTHREADSTATE_RUNNING;
    m_pScriptSite->OnEnterScript();
} // EnterScript


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// ExitScript()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void COleScript::ExitScript()
{
    invariant();
    int dummy;
    mem.setStackBottom((void *)&dummy);
    m_pScriptSite->OnLeaveScript();
    m_stsState = SCRIPTTHREADSTATE_NOTINSCRIPT;
} // ExitScript


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// HandleScriptError()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::HandleScriptError(ErrInfo *perrinfo, EXCEPINFO* pei, DWORD dwSourceContextCookie)
{
    HRESULT hr = DISP_E_EXCEPTION;
    EXCEPINFO   ei;
    bool        fReturnException = true;

    //FuncLog funclog(L"HandleScriptError()");
#if LOG
    WPRINTF(L"HandleScriptError('%s')\n", perrinfo->message);
#endif
    invariant();
    mem.setStackBottom((void *)&pei);
    memset(&ei, 0, sizeof(ei));

//WPRINTF(L"h1\n");
    // Return error code in wCode or scode, but not both
#if 0
    ei.wCode = 1001;       // random (!) number greater than 1000
#else
    unsigned code = perrinfo->code ? perrinfo->code : 1001;
    if ((code & 0xFFFF0000) == 0)
	code = MAKE_HRESULT(SEVERITY_ERROR,FACILITY_CONTROL,code);
    ei.scode = code;
#endif

    ei.bstrSource = SysAllocString(L"Digital Mars DMDScript");
    ei.bstrDescription = SysAllocString(perrinfo->message);

//WPRINTF(L"h2\n");
    if (NULL != m_pScriptSite)
    {
        //
        //   Create new IActiveScriptError object
        //
	GC_LOG();
        CActiveScriptError* pErrorObj = new CActiveScriptError;

//WPRINTF(L"h3, linnum = %d\n", perrinfo->linnum);
        if (NULL != pErrorObj)
        {
            HRESULT hrTest;
	    int nLine;
	    int nChar;
            
	    // Our line numbers are 1 based. Convert to 0 based
	    nLine = perrinfo->linnum;
	    if (nLine)			// we use 0 as "not available"
		nLine--;
	    nChar = perrinfo->charpos - 1;	// use -1 as "not available"
	    //WPRINTF(L"nLine = %d, nChar = %d\n", nLine, nChar);

            //
            //   Give object the EXCEPINFO for it to copy.
            //
//WPRINTF(L"h4\n");
            hrTest = pErrorObj->Initialize(&ei, perrinfo->srcline, nLine, nChar, dwSourceContextCookie);

            if (SUCCEEDED(hrTest))
            {
                IActiveScriptError* pase = NULL;

//WPRINTF(L"h5\n");
                hrTest = pErrorObj->QueryInterface(IID_IActiveScriptError, (void**)&pase);
//WPRINTF(L"h6\n");
                assert(SUCCEEDED(hrTest) && NULL != pase);
//WPRINTF(L"h7\n");
                hrTest = m_pScriptSite->OnScriptError(pase);
//WPRINTF(L"h8\n");
                pase->Release();
                pase = NULL;

                //
                // We're checking explicitly for S_OK here rather than just
                // SUCCEEDED() because many times when people want to stub
                // out OnScriptError(), they'll return S_FALSE. In that case we
                // should still return the exception instead of assuming
                // that OnScriptError() handled it.
                //
//WPRINTF(L"h9\n");
                if (S_OK == hrTest)
                {
                    fReturnException = false;
                    hr = SCRIPT_E_REPORTED;
                }
            }
	    pErrorObj->Release();
	    pErrorObj = NULL;
        }
    }

    //
    // We allocated some BSTR's to fill in our temporary EXCEPINFO structure. If we
    // are going to return a copy of that EXCEPINFO to the caller, then we just
    // copy over the structure and the receiver owns the BSTR's. Otherwise, if the
    // caller didn't give us an out-param for the EXCEPINFO or if we already reported
    // the error through OnScriptError(), then we still own these BSTR's and
    // therefore we must free them before we return.
    //
//WPRINTF(L"h10\n");
    if (fReturnException && NULL != pei)
    {
//WPRINTF(L"h11\n");
        memcpy(pei, &ei, sizeof(*pei));
    }
    else
    {
//WPRINTF(L"h12\n");
        if (NULL != ei.bstrSource)
	    SysFreeString(ei.bstrSource);

        if (NULL != ei.bstrDescription)
	    SysFreeString(ei.bstrDescription);
    }
//WPRINTF(L"h13\n");

//SetScriptState(SCRIPTSTATE_CLOSED);
//Release();
    return hr;
} // HandleScriptError


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _ChangeScriptState()
//
// Call this ONLY in the base thread,
// whenever you want to change the SCRIPTSTATE.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void COleScript::_ChangeScriptState(SCRIPTSTATE ss)
{
    invariant();
    m_ssState = ss;

    if (NULL != m_pScriptSite)
        m_pScriptSite->OnStateChange(m_ssState);
} // _ChangeScriptState


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _FindNamedItem()
//
// Find an SNamedItemData given a NamedItem string.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
SNamedItemData* COleScript::_FindNamedItem(/*[in] */ LPCOLESTR pcwszName)
{
    invariant();
    SNamedItemData* pnidCur = m_pNamedItems;

    while (NULL != pnidCur)
    {
        if (0 == wcscmp(pcwszName, pnidCur->bszItemName))
        {
            return pnidCur;
        }

        pnidCur = pnidCur->pNext;
    }

    return NULL;
} // _FindNamedItem


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _FindNamedItem()
//
// Find an SNamedItemData given a Module number identifier.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
SNamedItemData* COleScript::_FindNamedItem(/*[in] */ UINT uModule)
{
    invariant();

    //
    // ASSERT: There is never a NamedItem record for the global module.
    //         This will not work this way.
    //
    assert(GLOBAL_MODULE != uModule);

    SNamedItemData* pnidCur = m_pNamedItems;

    while (NULL != pnidCur)
    {
        if (uModule == pnidCur->uModuleNumber)
            return pnidCur;

        pnidCur = pnidCur->pNext;
    }

    return NULL;
} // _FindNamedItem


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _ResetNamedItems()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::_ResetNamedItems()
{
    SNamedItemData*     pnidCur = NULL;
    SNamedItemData**    ppnidLink = NULL;

    HRESULT hr = S_OK;

    ENTRYLOG(L"COM: ENTER _ResetNamedItems()\n");
    invariant();

    // delete non-persistent named items:

    pnidCur = m_pNamedItems;
    ppnidLink = &m_pNamedItems;

    //
    // Walk through each item and decide which ones to keep
    //
    while (NULL != pnidCur)
    {
        if (pnidCur->dwAddFlags & SCRIPTITEM_ISPERSISTENT)
        {
            //
            // Keep this item; move to next
            //
            pnidCur->fRegistered = false;   // Reset this so we re-register it
            //delete pnidCur->pWrapper;       // Delete the wrapper so we can
	    if (pnidCur->pWrapper)
	    {
		pnidCur->pWrapper->finalizer(); // don't delete, just Release()
		pnidCur->pWrapper = NULL;       // build it again when we re-register
	    }
            ppnidLink = &(pnidCur->pNext);  // Move link ptr to next link
        }
        else
        {
            //
            // Update the link pointer that was pointing to
            // what we're going to delete to now skip that entry
            //
            *ppnidLink = pnidCur->pNext;

            delete pnidCur;
        }
	pnidCur = *ppnidLink;
    }

    ENTRYLOG(L"COM: EXIT _ResetNamedItems()\n");
    return hr;
} // _ResetNamedItems


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _ResetScriptItems()
//
// Walk through our cache of scripts and only keep the ones that
// are marked as persistent. Mark all these keepers as not having
// been executed.
//
// WARNING: RESET THE GLOBAL PROGRAM AND CALLCONTEXT OBJECTS
//          BEFORE DOING THIS, OR RESULTS ARE UNPREDICTABLE.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::_ResetScriptItems()
{
    SParsedScriptData*  psdCur = NULL;
    SParsedScriptData** ppsdLink = NULL;

    HRESULT hr = S_OK;

    ENTRYLOG(L"COM: ENTER _ResetScriptItems()\n");
    invariant();

    // delete non-persistent script items:

    psdCur = m_pParsedScripts;
    ppsdLink = &m_pParsedScripts;

    //
    // Walk through each item and decide which ones to keep
    //
    while (NULL != psdCur)
    {
        if (psdCur->dwFlags & SCRIPTTEXT_ISPERSISTENT)
        {
            //
            // Keep this item; move to next
            //
	    //WPRINTF(L"\tkeeping persistent script %x '%ls'\n", psdCur, psdCur->pfdCode->name ? psdCur->pfdCode->name->string : L"(null)");
            psdCur->fExecuted = false;   // Reset this so we re-run it
            ppsdLink = &(psdCur->pNext);
        }
        else
        {
            //
            // Update the link pointer that was pointing to what we're going to delete
            //
            *ppsdLink = psdCur->pNext;

            delete psdCur;
        }
	psdCur = *ppsdLink;
    }

    ENTRYLOG(L"COM: EXIT _ResetScriptItems()\n");
    return hr;
} // _ResetScriptItems


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _RegisterPersistentNamedItems()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::_RegisterPersistentNamedItems()
{
    invariant();

    HRESULT hr = S_OK;
    HRESULT hr2 = E_UNEXPECTED;

    SNamedItemData*     pnidCur = NULL;
    //
    // Re-register persistent named items
    //
    pnidCur = m_pNamedItems;
    while(NULL != pnidCur)
    {
        hr2 = _RegisterNamedItem(pnidCur);
        if (FAILED(hr2))
        {
            hr = hr2;
        }

        pnidCur = pnidCur->pNext;
    }

    ENTRYLOG(L"COM: EXIT _RegisterPersistentNamedItems() = x%x\n", hr);
    return hr;
} // _RegisterPersistentNamedItems


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _RegisterNamedItem()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::_RegisterNamedItem(SNamedItemData* pnid)
{
    ENTRYLOG(L"COM: ENTER _RegisterNamedItem(pnid = x%x, pnid->bszItemName = '%s')\n", pnid, pnid->bszItemName);
    invariant();

    HRESULT hr = E_FAIL;
    IUnknown*   pUnk = NULL;
    IDispatch*  pDisp = NULL;

    if (NULL == m_pScriptSite)
	return E_UNEXPECTED;

    //
    // ASSERT: Something is seriously wrong if we're trying to register a
    //         named item again that we've already registered.
    //
    assert(!pnid->fRegistered);

    //
    // Get the object pointer
    //
    hr = m_pScriptSite->GetItemInfo(pnid->bszItemName,      // LPCOLESTR pcszName,
                                    SCRIPTINFO_IUNKNOWN,    // DWORD dwReturnMask,
                                    &pUnk,                  // IUnknown **ppiunkItem,
                                    NULL);                  // ITypeInfo **ppti

    if (FAILED(hr))
    {
        DPF(L"COM: ERROR _RegisterNamedItem() : script site failed to return unknown for \"%s\"\n", pnid->bszItemName);
        return S_OK;        // Just return now and ignore this item.
    }

    //
    // Get the object's IDispatch
    //
    hr = pUnk->QueryInterface(IID_IDispatch, (void **)&pDisp);
    pUnk->Release();
    pUnk = NULL;

    if (FAILED(hr) || NULL == pDisp)
        return S_OK;    // Just return now and ignore this item.

    d_string    dstrName = d_string_ctor(pnid->bszItemName);
    GC_LOG();
    Dcomobject* pWrapper = new Dcomobject(dstrName, pDisp, NULL);
    //WPRINTF(L"1Dcomobject = %x, pDisp = %x\n", pWrapper, pDisp);

    //
    // ASSERT: Overwriting here causes a leaked object because
    //         the wrapper's dtor won't get called...
    //
    assert(NULL == pnid->pWrapper);
    pnid->pWrapper = pWrapper;

    //
    // See if we are supposed to add the object globally
    //
    if (pnid->dwAddFlags & SCRIPTITEM_GLOBALMEMBERS)
    {
        CallContext*    pcc = m_pccGlobal;

        pcc->scope->insert(0, pWrapper);
        pcc->scoperoot++;
        pcc->globalroot++;

        //
        // Add the named item's name as a property in
        // the global namespace if the item is visible.
        //
        if (pnid->dwAddFlags & SCRIPTITEM_ISVISIBLE)
        {
            ((Dobject*)(pcc->global))->Put(dstrName, pWrapper, ReadOnly | DontOverride);
        }
    }
    else
    {
        //
        // Don't need to do anything here to actually create the namespace.
        // This is a namespace that's not global. We have the pWrapper
        // in the SNamedItemData, and when script is evaluated in this
        // namespace, we will create a proper scope chain, insert the
        // pWrapper in it, and be golden. We do need to do one thing though,
        // and that's to add the name to the root object of the global scope
        // if it's visible, so that you can do "name.property" from global
        // scope.
        //
        if (pnid->dwAddFlags & SCRIPTITEM_ISVISIBLE)
        {
            // Put into pcc->global namespace
            Dobject*    pobjScope = (Dobject *)m_pccGlobal->global;

            pobjScope->Put(dstrName, pWrapper, ReadOnly | DontOverride);
        }
    }

    pnid->fRegistered = true;       // Mark the pnid entry as registered
    hr = S_OK;

    if (NULL != pDisp)
    {
        pDisp->Release();
        pDisp = NULL;
    }

    ENTRYLOG(L"COM: EXIT _RegisterNamedItem()\n");
    return hr;
} // _RegisterNamedItem


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Run()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::Run()
{
    HRESULT hr = S_OK;

    ENTRYLOG(L"COM: ENTER Run()\n");
    invariant();

    int dummy;
    mem.setStackBottom((void *)&dummy);

    //
    // ASSERT: This is an internal function used by the state management
    //         routines, so any breach of the below conditions should be
    //         raised immediately.
    //
    assert(m_ssState == SCRIPTSTATE_INITIALIZED ||
           m_ssState == SCRIPTSTATE_STARTED);

    //
    // Connect the script to events
    //
    hr = _ConnectEventHandlers();
    if (FAILED(hr))
    {
	return E_FAIL;  // Failed to ConnectEventHandlers
    }

    _ChangeScriptState(SCRIPTSTATE_CONNECTED);

    ENTRYLOG(L"COM: EXIT Run() = x%x\n", S_OK);
    return S_OK;
} // Run


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Stop()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::Stop()
{
    ENTRYLOG(L"COM: ENTER Stop()\n");
    invariant();

    int dummy;
    mem.setStackBottom((void *)&dummy);

    if (m_ssState != SCRIPTSTATE_CONNECTED &&
        m_ssState != SCRIPTSTATE_DISCONNECTED)
    {
        return E_UNEXPECTED;
    }

    // This can be called in any thread.  We need to wait for all running event handlers to complete,
    // then disconnect from all connection points. The following stuff should
    // be done in the base thread!

    Disconnect();

    _ChangeScriptState(SCRIPTSTATE_INITIALIZED);
    ENTRYLOG(L"COM: EXIT Stop() = x%x\n", S_OK);
    return S_OK;
} // Stop


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// PseudoDisconnect()
//
// Go into a mode where we pretend that events are disconnected, but really
// we're just silently ignoring any events that get fired to our sinks.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::PseudoDisconnect(void)
{
    ENTRYLOG(L"COM: ENTER PseudoDisconnect()\n");
    invariant();

    int dummy;
    mem.setStackBottom((void *)&dummy);

    if (m_ssState != SCRIPTSTATE_CONNECTED)
    {
        return E_UNEXPECTED;
    }

    m_fIsPseudoDisconnected = true;

    _ChangeScriptState(SCRIPTSTATE_DISCONNECTED);
    ENTRYLOG(L"COM: EXIT PseudoDisconnect() = x%x\n", S_OK);
    return S_OK;
} // PseudoDisconnect


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Disconnect()
//
// Actually disconnect our event handlers from their source interfaces.
// (as opposed to pseudo-disconnecting)
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::Disconnect()
{
    ENTRYLOG(L"COM: ENTER Disconnect()\n");
    invariant();

    int dummy;
    mem.setStackBottom((void *)&dummy);

    if (m_ssState != SCRIPTSTATE_CONNECTED &&
        m_ssState != SCRIPTSTATE_DISCONNECTED)
    {
        return E_UNEXPECTED;
    }

    _DisconnectEventHandlers();
    m_fIsPseudoDisconnected = false;

    _ChangeScriptState(SCRIPTSTATE_DISCONNECTED);
    ENTRYLOG(L"COM: EXIT Disconnect() = x%x\n", S_OK);
    return S_OK;
} // Disconnect


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Reconnect()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::Reconnect()
{
    ENTRYLOG(L"COM: ENTER Reconnect()\n");
    invariant();

    int dummy;
    mem.setStackBottom((void *)&dummy);

    if (m_ssState != SCRIPTSTATE_DISCONNECTED)
    {
        return E_UNEXPECTED;
    }

    //
    // Connect to connection points
    //
    if (m_fIsPseudoDisconnected)
    {
        m_fIsPseudoDisconnected = false;
    }
    else
    {
        _ConnectEventHandlers();
    }

    _ChangeScriptState(SCRIPTSTATE_CONNECTED);
    ENTRYLOG(L"COM: EXIT Reconnect() = x%x\n", S_OK);
    return S_OK;
} // Reconnect


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Reset() - Deletes non-persistent named items and script blocks
//           and resets the run-time environment. Optionally
//           immediately re-registers persistent named items
//           if told to do so.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::Reset(bool fReRegister)
{
    HRESULT hr = S_OK;
    HRESULT hr2 = S_OK;

    ENTRYLOG(L"COM: ENTER Reset() this = x%x\n", this);
    invariant();

    int dummy;
    mem.setStackBottom((void *)&dummy);

    //
    // Reset the runtime
    //
    if (NULL != m_pccGlobal)
    {
        if (NULL != m_pccGlobal->scope)
	    m_pccGlobal->scope->zero();
	//delete m_pccGlobal;
        memset(m_pccGlobal, 0, sizeof(CallContext));
    }
    m_pProgram->globalfunction = NULL;
    m_pProgram->errors = 0;
    m_pProgram->callcontext = NULL;
    m_pProgram->initContext();
    m_pccGlobal = _CopyCallContext(m_pProgram->callcontext);


    if (NULL != m_pGlobalModuleDisp)
    {
        m_pGlobalModuleDisp->Release();
        m_pGlobalModuleDisp = NULL;
    }

    //
    // Delete non-persistent named items
    //
    hr = _ResetNamedItems();
    if (FAILED(hr))
    {
        DPF(L"Unexpected failure resetting named items, 0x%x", hr);
    }

    //
    // Delete non-persistent script text, and put persistent
    // script text back in the "haven't run yet" state.
    //
   
    hr2 = _ResetScriptItems();
    if (FAILED(hr2))
    {
        DPF(L"Unexpected failure code resetting script items, 0x%x", hr2);
    }

    if (SUCCEEDED(hr) && SUCCEEDED(hr2))
    {
        if (fReRegister)
        {
            hr = _RegisterPersistentNamedItems();
            if (FAILED(hr))
            {
                DPF(L"Unexpected failure re-registering named items! 0x%x", hr);
            }
        }
    }

    // turning this on causes a full collect here and in the destructor
    // when caching is turned off. Too slow to do it twice.
#if 0
    //if (cacheOn)
    {
	//WPRINTF(L"Reset() fullcollect\n");
	//PerfTimer pt(L"fullcollect");
#if 1
	// Free the string table
	ThreadContext *tc = ThreadContext::getThreadContext();
	if (tc->stringtable)
	{   tc->stringtable->~StringTable();
	    tc->stringtable = NULL;
	}
#endif

	// NOTE: We know at this point that we have NO valid roots
	// on the stack. Every root should be in ThreadContext. Since
	// Chcom uses large amounts of stack that is never initialized,
	// scanning the stack results in the pinning of large amounts of
	// memory due to leftover stackframe garbage.
	mem.fullcollectNoStack();
    }
#endif

    ENTRYLOG(L"COM: EXIT Reset() = 0x%x\n", hr);
    return hr;
} // Reset


#ifndef DISABLE_EVENT_HANDLING

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _BuildHandlerName()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::_BuildHandlerName(const WCHAR* pcwszItemName,
                                      const WCHAR* pcwszSubItemName,
                                      WCHAR** ppwszHandler)
{
    invariant();
    int cchHandler = wcslen(pcwszItemName) + 1;         // Item Name and '\0'

    if (NULL != pcwszSubItemName)
         cchHandler += 1 +  wcslen(pcwszSubItemName);   // "_" and subitem name text

    WCHAR*  pwsz = (WCHAR*)mem.malloc(sizeof(WCHAR)*cchHandler);
    if (NULL == pwsz)
        return E_OUTOFMEMORY;

    wcscpy(pwsz, pcwszItemName);

    if (NULL != pcwszSubItemName)
    {
        wcscat(pwsz, L"_");
        wcscat(pwsz, pcwszSubItemName);
    }

    *ppwszHandler = pwsz;
    return S_OK;
} // _BuildHandlerName


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _BuildFunctionNameAndBody()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::_BuildFunctionNameAndBody(const WCHAR* pcwszHandlerName,
                                              const WCHAR* pcwszEventName,
                                              const WCHAR* pcwszCode,
                                              const WCHAR* pcwszDelimiter,
                                              WCHAR** ppwszFuncName,
                                              WCHAR** ppwszFunc)
{
    invariant();
    assert(NULL != pcwszHandlerName);
    assert(NULL != pcwszEventName);
    assert(NULL != pcwszCode);

    int cchName = wcslen(pcwszHandlerName) +        // ItemName or ItemName_SubItemName
                  1 +                               // "_"
                  wcslen(pcwszEventName) +          // EventName
                  1;                                // "\0"

    int cchCode = 9 +                               // "function " == 9 chars
                  cchName-1 +                       // Full function name minus NULL term
                  5 +                               // "()\n{\n" == 5 chars
                  wcslen(pcwszCode) +               // Code
                  4 +                               // "\n}\n\0" == 4 chars
                  2;                                // Extra room in case we need to add newlines
                                                    // to handle HTML comments


    WCHAR*  pwszFuncName = (WCHAR*)mem.malloc(sizeof(WCHAR)*cchName);
    if (NULL == pwszFuncName)
    {
        return E_OUTOFMEMORY;
    }

    WCHAR*  pwszFunc = (WCHAR*)mem.malloc(sizeof(WCHAR)*cchCode);
    if (NULL == pwszFunc)
    {
        pwszFuncName = NULL;
        return E_OUTOFMEMORY;
    }

    //
    // Build a name as "Item_SubItem_Event" or just "Item_Event"
    //
    wcscpy(pwszFuncName, pcwszHandlerName);
    wcscat(pwszFuncName, L"_");
    wcscat(pwszFuncName, pcwszEventName);

    bool fDoNormalBuild = true;

    if (_IsHTMLScriptDelimiter(pcwszDelimiter))
    {
        const WCHAR*    pcwszTemp = pcwszCode;
        WCHAR*          pwszWrite = pwszFunc;

        while (NULL != pcwszTemp && '\0' != *pcwszTemp)
        {
            if ('<' == pcwszTemp[0] &&
                '!' == pcwszTemp[1] &&
                '-' == pcwszTemp[2] &&
                '-' == pcwszTemp[3])
            {
                fDoNormalBuild = false;

                wcsncpy(pwszWrite, pcwszTemp, 4);
                pwszWrite += 4;
                pcwszTemp += 4;

                *pwszWrite++ = '\n';
                wcscpy(pwszWrite, L"function ");
                wcscat(pwszWrite, pwszFuncName);
                wcscat(pwszWrite, L"()\n{\n");

                pwszWrite += wcslen(pwszWrite);

                while (NULL != pcwszTemp && '\0' != *pcwszTemp)
                {
                    if ('-' == pcwszTemp[0] &&
                        '-' == pcwszTemp[1]&&
                        '>' == pcwszTemp[2])
                    {
                        *pwszWrite++ = '\n';
                        *pwszWrite++ = '}';
                        *pwszWrite++ = '\n';

                        //
                        // Don't break: loop will continue processing for us, thus
                        // finishing the copy step for all the code text.
                        //

                        wcsncpy(pwszWrite, pcwszTemp, 3);
                        pwszWrite += 3;
                        pcwszTemp += 3;
                    }
                    else
                    {
                        *pwszWrite++ = *pcwszTemp++;
                    }
                }

                break;
            }
            else
            {
                *pwszWrite++ = *pcwszTemp++;
            }
        }
    }

    if (fDoNormalBuild)
    {
        //
        // Now build the full function body
        //
        wcscpy(pwszFunc, L"function ");
        wcscat(pwszFunc, pwszFuncName);
        wcscat(pwszFunc, L"()\n{\n");
        wcscat(pwszFunc, pcwszCode);
        wcscat(pwszFunc, L"\n}\n");
    }

    *ppwszFuncName = pwszFuncName;
    *ppwszFunc = pwszFunc;

    return S_OK;
} // __BuildFunctionNameAndBody


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _IsHTMLScriptDelimiter()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
bool COleScript::_IsHTMLScriptDelimiter(const WCHAR* pcwszDelim)
{
    invariant();
    if (NULL != pcwszDelim)
    {
        if ((pcwszDelim[0] == '<') &&
            (pcwszDelim[1] == '/') &&
            (pcwszDelim[2] == 'S' || pcwszDelim[2] == 's') &&
            (pcwszDelim[3] == 'C' || pcwszDelim[2] == 'c') &&
            (pcwszDelim[4] == 'R' || pcwszDelim[2] == 'r') &&
            (pcwszDelim[5] == 'I' || pcwszDelim[2] == 'i') &&
            (pcwszDelim[6] == 'P' || pcwszDelim[2] == 'p') &&
            (pcwszDelim[7] == 'T' || pcwszDelim[2] == 't') &&
            (pcwszDelim[8] == '>'))
            return true;
    }

    return false;
} // _IsHTMLScriptDelimiter


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _BuildDispatchEventSink()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::_BuildDispatchEventSink(SNamedItemData* pnid,
                                            const WCHAR* pcwszSubItemName,
                                            const WCHAR* pcwszHandlerName,
                                            CDispatchEventSink** ppSink)
{
    invariant();
    assert(NULL != pnid);
    assert(NULL != ppSink);

    GC_LOG();
    CDispatchEventSink* pSink = new CDispatchEventSink;

    if (NULL == pSink)
        return E_OUTOFMEMORY;

    HRESULT hr = _InitializeEventSink(pnid, pcwszSubItemName, pcwszHandlerName, pSink);
    if (FAILED(hr))
    {
        *ppSink = NULL;
        pSink->Release();
        return hr;
    }

    *ppSink = pSink;
    return S_OK;
} // _BuildDispatchEventSink


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _InitializeEventSink()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::_InitializeEventSink(SNamedItemData* pnid,
                                         const WCHAR* pcwszSubItemName,
                                         const WCHAR* pcwszHandlerName,
                                         CDispatchEventSink* pSink)
{
    invariant();
    assert(NULL != pnid);
    assert(NULL != pSink);

    ITypeInfo*                  ptiDispatch = NULL;     // TypeInfo of the source interface
    ITypeInfo*                  ptiCoClass = NULL;      // TypeInfo of the CoClass
    IDispatch*                  pdispItem = NULL;
    IDispatch*                  pdispModule = NULL;
    IProvideMultipleClassInfo*  ppmciItem = NULL;
    IProvideClassInfo*          ppciItem = NULL;

    HRESULT hr = E_UNEXPECTED;
    ULONG   cTypeInfos = 0;     // Number of coclass type infos
    ULONG   idxTypeInfo = 0;

    bool    fIsExtender = false;

    if (!(pnid->dwAddFlags & SCRIPTITEM_ISSOURCE))
        return E_FAIL;

    if (GLOBAL_MODULE == pnid->uModuleNumber)
        pdispModule = m_pGlobalModuleDisp;
    else
        pdispModule = pnid->pModuleDisp;

    if (NULL == pdispModule)
        return E_FAIL;

    //
    // Get the object pointer
    //
    hr = _GetNamedItemDispatch(pnid->bszItemName, pcwszSubItemName, &pdispItem);
    if (FAILED(hr) || NULL == pdispItem)
        return E_INVALIDARG;

    //
    // Now do work to grovel for the type information...
    //

    // See if this is an extender object:
    //
    hr = pdispItem->QueryInterface(IID_IProvideMultipleClassInfo, (void **)&ppmciItem);

    if (SUCCEEDED(hr) && NULL != ppmciItem)
    {
        // Extender object -- Get array of type infos

        hr = ppmciItem->GetMultiTypeInfoCount(&cTypeInfos);

        if (FAILED(hr) || 0 == cTypeInfos)
            fIsExtender = false;
        else
            fIsExtender = true;
    }
    else
    {
        fIsExtender = false;
    }

    if (!fIsExtender)
    {
        //
        // Look for IProvideClassInfo before completely giving up and asking
        // the scriptsite for a typeinfo.
        //

        hr = pdispItem->QueryInterface(IID_IProvideClassInfo, (void**)&ppciItem);

        if (SUCCEEDED(hr))
        {
            hr = ppciItem->GetClassInfo(&ptiCoClass);

            cTypeInfos = 1;

            ppciItem->Release();
            ppciItem = NULL;

            //
            // In case the object screws up and returns success without
            // a TypeInfo, convert to failure so next codepath takes over.
            //
            if (SUCCEEDED(hr) && NULL == ptiCoClass)
                hr = E_NOINTERFACE;
        }


        //
        // Object doesn't support any classinfo interfaces, so we can't
        // hope to receive events from it. Sorry, game over.
        //
        if (FAILED(hr))
        {
            goto Exit;
        }

    } // if (!fIsExtender)

    for (idxTypeInfo = 0; idxTypeInfo < cTypeInfos; idxTypeInfo++)
    {
        if (fIsExtender)
        {
            DWORD dwTIFlags;

            if (ptiCoClass)
            {
                ptiCoClass->Release();
                ptiCoClass = NULL;
            }

            // Get the next coclass type info
            hr = ppmciItem->GetInfoOfIndex(idxTypeInfo, MULTICLASSINFO_GETTYPEINFO,
                                           &ptiCoClass, &dwTIFlags,
                                           NULL, NULL, NULL);
            if (FAILED(hr))
                goto Exit;
        }

        //
        // Cleanup any leftover from the last iteration...
        //
        if (ptiDispatch)
        {
            ptiDispatch->Release();
            ptiDispatch = NULL;
        }

        //
        // We have a coclass type info.  Use it to find the dispatch type info.
        // Failure here just means there's no default source interface on this
        // this particular typeinfo. Not really a failure.
        //
        HRESULT hrTemp = _GetDefaultSourceTypeInfo(ptiCoClass, &ptiDispatch);

        //
        // As soon as we successfully find one event interface, we can create our
        // event sink and return.
        //
        if (SUCCEEDED(hrTemp))
        {
            //
            // What?  Success, but we weren't given an interface? That's whacked...
            //
            assert(NULL != ptiDispatch);
            if (NULL == ptiDispatch)
            {
                hr = E_NOINTERFACE;
                goto Exit;
            }

            //
            // We successfully got a [default, source] event interface for the object.
            // Now it's time to hand off the typeinfo so we can build an event sink.
            //
            hr = pSink->Initialize(pcwszHandlerName, pdispItem, ptiDispatch, pdispModule);
            if (FAILED(hr))
            {
                pSink->Release();
                pSink = NULL;
                goto Exit;
            }

            hr = S_OK;
            goto Exit;
        }
    } // For each extended type info

Exit:
    if (ppmciItem) {
        ppmciItem->Release();
    }
    if (ptiDispatch) {
        ptiDispatch->Release();
    }
    if (ptiCoClass) {
        ptiCoClass->Release();
    }
    if (pdispItem) {
        pdispItem->Release();
    }
    return hr;
} // _InitializeEventSink


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _GetDefaultSourceTypeInfo()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::_GetDefaultSourceTypeInfo(ITypeInfo *ptiCoClass, ITypeInfo **pptiSource)
{
    TYPEATTR*   pTypeAttr = NULL;
    HREFTYPE    hreftype = NULL;
    HRESULT     hr = E_UNEXPECTED;
    int         iFlags = 0;
    UINT        cImplTypes = 0;

    invariant();
    assert(NULL != ptiCoClass);
    assert(NULL != pptiSource);

    *pptiSource = NULL;

    hr = ptiCoClass->GetTypeAttr(&pTypeAttr);
    if (FAILED(hr))
        return hr;
    if (NULL == pTypeAttr)
        return E_FAIL;

    //
    // Make sure the typeinfo is a CoClass (consisting of a set of interfaces)
    //
    assert(TKIND_COCLASS == pTypeAttr->typekind);

    cImplTypes = pTypeAttr->cImplTypes;

    (void) ptiCoClass->ReleaseTypeAttr(pTypeAttr);

    //
    // ...search through the set of interfaces in the CoClass
    //
    for (UINT iImplType = 0; iImplType < cImplTypes ; iImplType++)
    {
        iFlags = 0;
        hr = ptiCoClass->GetImplTypeFlags(iImplType, &iFlags);
        if (FAILED(hr))
            return hr;

        //
        // Look for the first [source, default] interface on the coclass
        // that isn't marked as restricted.
        //
        if ((iFlags & (IMPLTYPEFLAG_FDEFAULT | IMPLTYPEFLAG_FSOURCE | IMPLTYPEFLAG_FRESTRICTED))
                == (IMPLTYPEFLAG_FDEFAULT | IMPLTYPEFLAG_FSOURCE))
        {
            //
            // Get the handle of the implemented interface.
            //
            hr = ptiCoClass->GetRefTypeOfImplType(iImplType, &hreftype);
            if (FAILED(hr))
                return hr;

            //
            // Get the typeinfo implementing the interface.
            //
            ITypeInfo*  pti = NULL;

            hr = ptiCoClass->GetRefTypeInfo(hreftype, &pti);

            if (FAILED(hr))
                return hr;

            //
            // Transform pti to a TypeInfo for a Dispatch interface, if we have a
            // TypeInfo for a dual interface.
            //
            hr = pti->GetTypeAttr(&pTypeAttr);

            if (SUCCEEDED(hr))
            {
                //
                // Should always be true of a typekind that comes off a coclass, I think.
                // We're assuming it is...
                //
                assert(pTypeAttr->typekind == TKIND_DISPATCH || pTypeAttr->typekind == TKIND_INTERFACE);

                if (pTypeAttr->typekind == TKIND_DISPATCH
                    || (pTypeAttr->typekind == TKIND_INTERFACE && (pTypeAttr->wTypeFlags & TYPEFLAG_FDISPATCHABLE)))
                {
                    *pptiSource = pti;
                    (*pptiSource)->AddRef();
                }
                else if(pTypeAttr->typekind == TKIND_INTERFACE)
                {
                    if (pTypeAttr->wTypeFlags & TYPEFLAG_FDUAL)
                    {
                        //
                        // If this is a dual interface, get the corresponding Disp interface;
                        // -1 is a special value which does this for us.
                        //
                        hr = pti->GetRefTypeOfImplType((UINT)-1, &hreftype);

                        if (SUCCEEDED(hr))
                        {
                            hr = pti->GetRefTypeInfo(hreftype, pptiSource);
                        }

                        //
                        // If either of the above two calls failed, then clean up and return
                        //
                        if (FAILED(hr))
                        {
                            pti->ReleaseTypeAttr(pTypeAttr);
                            pti->Release();
                            return hr;
                        }
                    }
                }

                pti->ReleaseTypeAttr(pTypeAttr);
                pTypeAttr = NULL;
            }

            //
            // If we made it here
            //
            pti->Release();
            return hr;
        }
    }

    return E_FAIL; // Not found!
} // _GetDefaultSourceTypeInfo


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _GetNamedItemEventSink()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::_GetNamedItemEventSink(SNamedItemData* pnid, const WCHAR* pcwszSubItemName,
                                          const WCHAR* pcwszHandlerName, CDispatchEventSink** ppSink)
{
    invariant();
    //
    // Look for an existing CDispatchEventSink based on this handler name
    //
    CDispatchEventSink* pSink = _FindEventSink(pcwszHandlerName);
    HRESULT hr = E_UNEXPECTED;
    bool    fTryToConnect = false;

    if (NULL == pSink)
    {
        hr = _BuildDispatchEventSink(pnid, pcwszSubItemName, pcwszHandlerName, &pSink);
        if (SUCCEEDED(hr))
        {
            this->m_rgEventSinks.push(pSink);
            fTryToConnect = true;
        }
    }
    else
    {
        if (!pSink->IsInitialized())
        {
            hr = _InitializeEventSink(pnid, pcwszSubItemName, pcwszHandlerName, pSink);
            if (SUCCEEDED(hr))
            {
                fTryToConnect = true;
            }
        }
        else
        {
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr))
    {
        if (fTryToConnect)
        {
            if (m_ssState == SCRIPTSTATE_CONNECTED)
            {
                hr = pSink->Connect();
            }
            else if (m_ssState == SCRIPTSTATE_DISCONNECTED)
            {
            }
            else
            {
                // Do nothing. Handler will be connected when we transition states
            }
        }

        *ppSink = pSink;
    }
    else
    {
        *ppSink = NULL;
    }

    return hr;
} // _GetNamedItemEventSink


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _GetNamedItemDispatch()
//
// Retrieves an IDispatch for the Item Name passed in. If the given SubItem name is
// non-NULL and non-empty, tries to get that sub item and return that instead.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::_GetNamedItemDispatch(const WCHAR* pcwszItem, const WCHAR* pcwszSubItem, IDispatch** ppDisp)
{
    ENTRYLOG(L"COM: ENTER _GetNamedItemDispatch(\"%s\", \"%s\")\n", pcwszItem, pcwszSubItem);
    invariant();

    HRESULT hr = E_UNEXPECTED;

    *ppDisp = NULL;

    if (NULL == m_pScriptSite)
        return hr;

    IUnknown*   punk = NULL;
    IDispatch*  pdispItem = NULL;
    IDispatchEx*pdispexItem = NULL;

    hr = m_pScriptSite->GetItemInfo(pcwszItem, SCRIPTINFO_IUNKNOWN, &punk, NULL);
    if (FAILED(hr))
        goto Error;

    hr = punk->QueryInterface(IID_IDispatchEx, (void**)&pdispexItem);
    if (FAILED(hr) || NULL == pdispexItem)
    {
        hr = punk->QueryInterface(IID_IDispatch, (void**)&pdispItem);
        punk->Release();
        punk = NULL;

        if (FAILED(hr))
            goto Error;
    }

    //
    // If there's a subitem name, try and get that instead of the Named Item object
    //
    if (NULL != pcwszSubItem && wcslen(pcwszSubItem) > 0)
    {
        DISPID      idSub = 0;
        DISPPARAMS  dp = { NULL, NULL, 0, 0 };
        VARIANT     var;

        if (NULL != pdispexItem)
        {
            BSTR    bszItem = SysAllocString(pcwszSubItem);
            if (NULL == bszItem)
            {
                hr = E_OUTOFMEMORY;
                goto Error;
            }

            hr = pdispexItem->GetDispID(bszItem, fdexNameCaseSensitive, &idSub);
            if (FAILED(hr))
                goto Error;

            VariantInit(&var);
            hr = pdispexItem->InvokeEx(idSub, LCID_ENGLISH_US,
                                   DISPATCH_METHOD | DISPATCH_PROPERTYGET,
                                   &dp, &var, NULL, NULL);

            if (FAILED(hr) || VT_DISPATCH != V_VT(&var) || NULL == V_DISPATCH(&var))
                goto Error;

            pdispexItem->Release();
            pdispexItem = NULL;

            pdispItem = V_DISPATCH(&var);
            //
            // NOTE: Do NOT VariantClear(&var) here, or the refcount
            //       we just assumed control of will be lost
            //
        }
        else
        {
            hr = pdispItem->GetIDsOfNames(IID_NULL, const_cast<WCHAR**>(&pcwszSubItem),
                                          1, LCID_ENGLISH_US, &idSub);
            if (FAILED(hr))
                goto Error;

            VariantInit(&var);
            hr = pdispItem->Invoke(idSub, IID_NULL, LCID_ENGLISH_US,
                                   DISPATCH_METHOD | DISPATCH_PROPERTYGET,
                                   &dp, &var, NULL, NULL);

            if (FAILED(hr) || VT_DISPATCH != V_VT(&var) || NULL == V_DISPATCH(&var))
                goto Error;

            pdispItem->Release();
            pdispItem = V_DISPATCH(&var);
            //
            // NOTE: Do NOT VariantClear(&var) here, or the refcount
            //       we just assumed control of will be lost
            //
        }
    }

    //
    // Return our reference to the caller
    //
    *ppDisp = pdispItem;

    ENTRYLOG(L"COM: EXIT _GetNamedItemDispatch()\n");
    return S_OK;

Error:
    if (NULL != pdispexItem)
        pdispexItem->Release();

    if (NULL != pdispItem)
        pdispItem->Release();

    if (NULL != punk)
        punk->Release();

    return hr;
} // _GetNamedItemDispatch


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _FindEventSink()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
CDispatchEventSink* COleScript::_FindEventSink(const WCHAR* pcwszHandlerName)
{
    invariant();
    if (NULL != pcwszHandlerName)
    {
        for (unsigned i = 0; i < m_rgEventSinks.dim; i++)
        {
            CDispatchEventSink* pSink = ((CDispatchEventSink*)m_rgEventSinks.data[i]);

            if (!pSink->IsInitialized() || 0 == wcscmp(pcwszHandlerName, pSink->GetName()))
            {
                return (CDispatchEventSink*)m_rgEventSinks.data[i];
            }
        }
    }

    return NULL;
} // _FindEventSink

#endif // DISABLE_EVENT_HANDLING

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _ConnectEventHandlers()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::_ConnectEventHandlers(void)
{
    invariant();
#ifdef DISABLE_EVENT_HANDLING
    return S_FALSE;
#else
    //
    // Completely reset the state of all event sinks
    // (frees any back pointers to this engine or any
    //  named items or objects)
    //
    for (unsigned i = 0; i < m_rgEventSinks.dim; i++)
    {
        ((CDispatchEventSink*)m_rgEventSinks.data[i])->Connect();
    }

    return S_OK;
#endif // DISABLE_EVENT_HANDLING
} // ConnectEventHandlers


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _DisconnectEventHandlers()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::_DisconnectEventHandlers(void)
{
    invariant();
#ifdef DISABLE_EVENT_HANDLING
    return S_FALSE;
#else
    //
    // Completely reset the state of all event sinks
    // (frees any back pointers to this engine or any
    //  named items or objects)
    //
    for (unsigned i = 0; i < m_rgEventSinks.dim; i++)
    {
        ((CDispatchEventSink*)m_rgEventSinks.data[i])->Disconnect();
    }

    return S_OK;
#endif // DISABLE_EVENT_HANDLING
} // _DisconnectEventHandlers


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _TeardownEventHandlers()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT COleScript::_TeardownEventHandlers(void)
{
    invariant();
#ifdef DISABLE_EVENT_HANDLING
    return S_FALSE;
#else
    //
    // Completely reset the state of all event sinks
    // (frees any back pointers to this engine or any
    //  named items or objects)
    //
    for (unsigned i = 0; i < m_rgEventSinks.dim; i++)
    {
        ((CDispatchEventSink*)m_rgEventSinks.data[i])->Teardown();
    }

    return S_OK;
#endif // DISABLE_EVENT_HANDLING
} // _TeardownEventHandlers


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _SetLocale()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void COleScript::_SetLocale(void)
{
    invariant();
    int cch1 = 0;
    int cch2 = 0;

    cch1 = ::GetLocaleInfoW(m_lcid, LOCALE_SENGLANGUAGE, NULL, 0);
    cch2 = ::GetLocaleInfoW(m_lcid, LOCALE_SENGCOUNTRY, NULL, 0);

    WCHAR* pwsz = (WCHAR*)mem.malloc(sizeof(WCHAR)*(cch1+cch2+2));   // 2 is for underscore and \0

    if (NULL == pwsz)
        return;

    pwsz[cch1+cch2+1] = L'\0';
    cch1 = ::GetLocaleInfoW(m_lcid, LOCALE_SENGLANGUAGE, pwsz, cch1);
    if (cch1)
    {
        pwsz[cch1-1] = L'_';
        ::GetLocaleInfoW(m_lcid, LOCALE_SENGCOUNTRY, &pwsz[cch1], cch2);

#if defined linux
	{
	    char* psz = (char*)mem.malloc(sizeof(char)*(wcslen(pwsz)+1));
	    if(psz)
	    {
		wcstombs(psz, pwsz, wcslen(pwsz));
		setlocale(LC_ALL, psz);
	    }
	    else
	    {
		return;
	    }
	}
#else
        _wsetlocale(LC_ALL, pwsz);
#endif
    }
} // _SetLocale


// We can use this function when we're stopped at a breakpoint in VC
// to cause the contents of the string parameter to be written to disk.
// Simply evaluate the expression "SaveSourceCode(...);" in the watch
// window with a parameter that points to some string you want to read.
//
#ifdef DEBUG


#include <stdio.h>
void SaveSourceCode(const WCHAR* pcwszCode)
{
    if (NULL == pcwszCode)
        return;

    int     cch = wcslen(pcwszCode);
    char*   psz = (char*)mem.malloc(sizeof(char)*(cch+2));
    FILE*   pf = NULL;

    psz[cch+1] = '\0';

    pf = fopen("c:\\dscript.ds", "w");

    if (NULL == pf)
    {
        mem.free(psz);
        return;
    }

    WideCharToMultiByte(CP_ACP, 0, pcwszCode, -1, psz, cch+1, 0, 0);

    fwrite(psz, 1, lstrlenA(psz), pf);

    fclose(pf);
    pf = NULL;

    mem.free(psz);
}

#endif // DEBUG

HRESULT COleScript::UpdateInfo(hostinfo hostinfoNew)
{
    HRESULT hr;
    IHostInfoProvider *pHostInfoProvider = NULL;
    LCID *pinfo = NULL;
    Program *p;

#if LOG
    WPRINTF(L"COleScript::UpdateInfo(this = %x, hostinfoNew = %d)\n", this, hostinfoNew);
    WPRINTF(L"\tm_pScriptSite = %x\n", m_pScriptSite);
#endif
    if (NULL == m_pScriptSite)
        return E_UNEXPECTED;
    hr = m_pScriptSite->QueryInterface(IID_IHostInfoProvider, (void **)&pHostInfoProvider);
    if (FAILED(hr))
    {
#if LOG
	WPRINTF(L"\tQueryInterface failed\n");
#endif
        goto Error;
    }

    hr = pHostInfoProvider->GetHostInfo(hostinfoNew,(void **)&pinfo);
    if (FAILED(hr))
    {
#if LOG
	WPRINTF(L"\tGetHostInfo failed with x%x\n", hr);
#endif
        goto Error;
    }

    switch (hostinfoNew)
    {
        case hostinfoLocale:
        case hostinfoErrorLocale:
#if LOG
	    WPRINTF(L"Setting LCID to %x\n", *pinfo);
#endif
	    p = Program::getProgram();
	    if (!p)
		p = m_pProgram;
	    if (p)
	    {
		p->lcid = *pinfo;
#if LOG
		WPRINTF(L"\tthis = %x, program = %x\n", this, p);
#endif
	    }
            m_lcid = *pinfo;
            break;
        case hostinfoCodePage:
#if LOG
	    WPRINTF(L"\thostinfoCodePage\n");
#endif
	    // We never use the code page for anything
            break;
	default:
#if LOG
	    WPRINTF(L"\tdefault\n");
#endif
	    hr = E_FAIL;
	    break;
    }

Error:
    if (pHostInfoProvider != NULL)
        pHostInfoProvider->Release();
    if (pinfo != NULL)
        CoTaskMemFree(pinfo);
    return hr;
}

