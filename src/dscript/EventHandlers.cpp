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
#include <dispex.h>

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

#if defined __DMC__
#include "dmc_rpcndr.h"
#endif

#include "EventHandlers.h"

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// CDispatchEventSink()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
CDispatchEventSink::CDispatchEventSink()
  : m_pwszName(NULL),
    m_pdispSource(NULL),
    m_pCP(NULL),
    m_pdispSink(NULL),
    m_iidInterface(IID_NULL),
    m_dwCookie(0),
    m_prgMethods(NULL),
    m_cMethods(0),
    m_fInitialized(false),
    m_fConnected(false)
{
    m_iidInterface = IID_NULL;
#if INVARIANT
    signature = CDISPATCHEVENTSINK_SIGNATURE;
#endif
} // CDispatchEventSink


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// ~CDispatchEventSink()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
CDispatchEventSink::~CDispatchEventSink()
{
    invariant();
    Teardown();
#if INVARIANT
    signature = 0;
#endif
} // ~CDispatchEventSink


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Initialize()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT CDispatchEventSink::Initialize(const WCHAR* pcwszName, IDispatch* pdispSource,
                                       ITypeInfo* ptiSource, IDispatch* pdispSink)
{
    //ENTRYLOG(L"COMEVT: ENTER Initialize()\n");
    invariant();
    mem.setStackBottom((void *)&pdispSink);

    assert(!m_fInitialized);
    assert(NULL == m_prgMethods && 0 == m_cMethods);

    if (NULL == pcwszName || NULL == pdispSource || NULL == ptiSource || NULL == pdispSink)
        return E_INVALIDARG;

    m_pwszName = (WCHAR*)mem.malloc(sizeof(WCHAR)*(wcslen(pcwszName)+1));
    if (NULL == m_pwszName)
        return E_OUTOFMEMORY;

    wcscpy(m_pwszName, pcwszName);

    //
    // Loop through the information for each method on this interface and call
    // GetNames() and put the results in this object's m_rgMethods array.
    //
    TYPEATTR*   pTypeAttr = NULL;
    HRESULT     hr = E_UNEXPECTED;

    hr = ptiSource->GetTypeAttr(&pTypeAttr);
    if (FAILED(hr))
    {
        DPF(L"COMEVT: ERROR Initialize() : GetTypeAttr() failed at %i\n", __LINE__);
        return hr;
    }

    //
    // Copy the IID of the interface we're going to sink
    //
    m_iidInterface = pTypeAttr->guid;

    //How many events are there in this interface?
    m_cMethods = pTypeAttr->cFuncs;

    ptiSource->ReleaseTypeAttr(pTypeAttr);

    //make sure we have some events to sink
    if (0 == m_cMethods)
    {
        DPF(L"COMEVT: ERROR Initialize() : no methods to sink!\n");
        return E_NOINTERFACE;
    }

    m_prgMethods = new SEventMethodData[m_cMethods];
    if (NULL == m_prgMethods)
    {
        m_cMethods = 0;
        return E_OUTOFMEMORY;
    }

    //
    // Create the list of events
    //
    FUNCDESC*   pfdesc = NULL;
    UINT        cNames;

    //
    // For each function, cache off a BSTR for the name as well as the DISPID. We'll
    // need the name to alias our handler functions later, and the DISPID to handle
    // actual event firings on the sink interface's IDispatch.
    //
    for (int i = 0; i < m_cMethods; i++)
    {
        hr = ptiSource->GetFuncDesc(i, &pfdesc);
        if (FAILED(hr))
            goto ErrorExit;

        //
        // Get the name of the event
        //
        hr = ptiSource->GetNames(pfdesc->memid, &m_prgMethods[i].pbszName, 1, &cNames);
        if (FAILED(hr))
        {
            DPF(L"COMEVT: ERROR Initialize() : GetNames() failed for method %i!\n", i);
            goto ErrorExit;
        }

        m_prgMethods[i].idEvent = pfdesc->memid;

        ptiSource->ReleaseFuncDesc(pfdesc);
    }

    m_pdispSource = pdispSource;
    m_pdispSource->AddRef();

    m_pdispSink = pdispSink;
    m_pdispSink->AddRef();

    m_fInitialized = true;
    //ENTRYLOG(L"COMEVT: EXIT Initialize() (success)\n");
    return S_OK;

ErrorExit:
    m_cMethods = 0;
    if (NULL != m_prgMethods)
    {
        delete[] m_prgMethods;
        m_prgMethods = NULL;
    }

    DPF(L"COMEVT: EXIT Initialize() (ERROR : 0x%08lx)\n", hr);
    return hr;
} // Initialize


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Connect()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT CDispatchEventSink::Connect()
{
    //ENTRYLOG(L"COMEVT: ENTER Connect()\n");
    char dummy;
    invariant();
    mem.setStackBottom((void *)&dummy);

    if (!m_fInitialized)
        return E_UNEXPECTED;
    if (m_fConnected)
        return S_FALSE;

    //
    // How did m_fInitialized get set true if there's no source object?
    //
    assert(NULL != m_pdispSource);
    assert(NULL == m_pCP);

    //
    // Now, get the IConnectionPointContainer of the source object
    // so we can find the CP we want to connect to...
    //
    IConnectionPointContainer*  pCPC = NULL;
    HRESULT hr = E_UNEXPECTED;

    hr = m_pdispSource->QueryInterface(IID_IConnectionPointContainer, (void**)&pCPC);
    if (FAILED(hr))
    {
        DPF(L"COMEVT: ERROR Connect() : CPC QI failed\n");
        return hr;
    }

    hr = pCPC->FindConnectionPoint(m_iidInterface, &m_pCP);
    pCPC->Release();
    if (FAILED(hr))
    {
        DPF(L"COMEVT: ERROR Connect() : FindConnectionPoint() failed (hr = 0x%08lx)\n", hr);
        return hr;
    }

    hr = m_pCP->Advise(static_cast<IUnknown*>(static_cast<CBaseComObject*>(this)), &m_dwCookie);
    if (FAILED(hr))
    {
        DPF(L"COMEVT: ERROR Connect() : CP Advise() failed : hr=0x%08lx\n", hr);
        m_pCP->Release();
        return hr;
    }

    m_fConnected = true;

    //ENTRYLOG(L"COMEVT: EXIT Connect()\n");
    return S_OK;
} // Connect


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Disconnect()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT CDispatchEventSink::Disconnect()
{
    //ENTRYLOG(L"COMEVT: ENTER Disconnect()\n");
    char dummy;
    invariant();
    mem.setStackBottom((void *)&dummy);

    if (!m_fInitialized)
        return E_UNEXPECTED;
    if (!m_fConnected)
        return S_FALSE;

    //
    // Should have a connection point if m_fConnected is true.
    //
    assert(NULL != m_pCP);
    m_pCP->Unadvise(m_dwCookie);
    m_pCP->Release();
    m_pCP = NULL;
    m_dwCookie = 0;
    m_fConnected = false;

    //ENTRYLOG(L"COMEVT: EXIT Disconnect()\n");
    return S_OK;
} // Disconnect


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Teardown()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT CDispatchEventSink::Teardown()
{
    //ENTRYLOG(L"COMEVT: ENTER Teardown()\n");
    char dummy;
    invariant();
    mem.setStackBottom((void *)&dummy);

    if (m_fInitialized)
    {
        if (m_fConnected)
        {
            Disconnect();
        }

        if (NULL != m_pdispSource)
        {
            m_pdispSource->Release();
            m_pdispSource = NULL;
        }
        if (NULL != m_pCP)
        {
            m_pCP->Release();
            m_pCP = NULL;
        }
        if (NULL != m_pdispSink)
        {
            m_pdispSink->Release();
            m_pdispSink = NULL;
        }

        if (NULL != m_prgMethods)
        {
            //
            // Clear the array of method data so it will clear its resources (BSTR's)
            //
            delete[] m_prgMethods;
            m_prgMethods = NULL;
            m_cMethods = 0;
        }

        if (NULL != m_pwszName)
        {
            mem.free(m_pwszName);
            m_pwszName = NULL;
        }

        m_fInitialized = false;
    }

    //ENTRYLOG(L"COMEVT: EXIT Teardown()\n");
    return S_OK;
} // Teardown


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// AttachEvent()
//
// Given the name of a handler function, presumably a function on
// the sink dispatch interface we were initialized with, create a
// relation between that handler and the event method on the source
// interface with the given name (which we got via typeinfo and
// cached at initialization time...).
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT CDispatchEventSink::AttachEvent(const WCHAR* pcwszEvent, const WCHAR* pcwszHandler)
{
    //ENTRYLOG(L"COMEVT: ENTER AttachEvent()\n");
    invariant();
    mem.setStackBottom((void *)&pcwszHandler);

    assert(NULL != pcwszEvent);
    assert(NULL != pcwszHandler);

    HRESULT hr = E_UNEXPECTED;

    if (!m_fInitialized)
        return hr;

    DISPID  idHandler = -1;

    //
    // Use GetIDsOfNames on sink interface to get the corresponding
    // DISPID for the given handler name.
    //
    hr = m_pdispSink->GetIDsOfNames(IID_NULL, const_cast<WCHAR**>(&pcwszHandler),
                                    1, LCID_ENGLISH_US, &idHandler);
    if (FAILED(hr))
        return E_INVALIDARG;

    //
    // Search through m_prgMethods looking for a match on pcwszEvent
    //
    for (int i = 0; i < m_cMethods; i++)
    {
#if defined linux
        if (0 == wcscasecmp(m_prgMethods[i].pbszName, pcwszEvent))
#else
        if (0 == wcsicmp(m_prgMethods[i].pbszName, pcwszEvent))
#endif
        {
            // If we found a match, and that event is already
            // handled, fail to install the new one.
            //
            // (Should we overwrite it with the new one instead?)

            if (m_prgMethods[i].fHasHandler)
            {
                DPF(L"COMEVT: ERROR AttachEvent() : Event already has a handler\n");
                return E_UNEXPECTED;
            }

            //
            // Map handler DISPID to event DISPID on source interface
            // and mark the event as having a handler installed.
            //
            m_prgMethods[i].idHandler = idHandler;
            m_prgMethods[i].fHasHandler = true;

            //ENTRYLOG(L"COMEVT: EXIT AttachEvent()\n");
            return S_OK;
        }
    }

    DPF(L"COMEVT: EXIT AttachEvent() : ERROR: Invalid Args\n");
    return E_INVALIDARG;
} // AttachEvent


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _DoGetIDsOfNames()
//
// We don't support mapping names to DISPID's on event sinks simply because
// we don't have to -- this interface is called from a connection point, and
// always through Invoke with predefined DISPID's that the caller knows about.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT CDispatchEventSink::_DoGetIDsOfNames(REFIID /*riid*/, OLECHAR** /*prgszNames*/, UINT /*cNames*/,
                                             LCID /*lcid*/, DISPID* /*prgIDs*/)
{
    //ENTRYLOG(L"COMEVT: ENTER/EXIT _DoGetIDsOfNames\n");
    invariant();
    //
    // Whatever it is, it's unknown to us...we don't bother with this method.
    //
    return DISP_E_UNKNOWNNAME;
} // _DoGetIDsOfNames


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// _DoInvoke()
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
HRESULT CDispatchEventSink::_DoInvoke(DISPID dispidMember, REFIID riid, LCID lcid,
                                      WORD wFlags, DISPPARAMS* pDispParams,
                                      VARIANT* pvarResult, EXCEPINFO* pExcepInfo,
                                      UINT* puArgErr)
{
    //ENTRYLOG(L"COMEVT: ENTER _DoInvoke(%l)\n", dispidMember);
    invariant();
    HRESULT hr = E_UNEXPECTED;

    //
    // We only support method invocations
    //
    if (!(wFlags & DISPATCH_METHOD))
    {
        DPF(L"COMEVT: ERROR _DoInvoke() DISPATCH_METHOD not supported\n");
        return E_INVALIDARG;
    }

    //
    // Not connected, so just clear the result and return success
    //
    if (!m_fInitialized || !m_fConnected)
    {
        //ENTRYLOG(L"COMEVT: EXIT _DoInvoke() : Not connected, early-out\n");

        if (NULL != pvarResult)
            VariantInit(pvarResult);

        return S_OK;
    }

    //
    // Try to find the record we should have stored at initialization time
    // that tells us about this event. If we find it, see if there's a
    // handler connected for the event. If there's a handler, pass the
    // call off to that handler for further processing.
    //
    for (int i = 0; i < m_cMethods; i++)
    {
        if (m_prgMethods[i].idEvent == dispidMember)
        {
            if (m_prgMethods[i].fHasHandler)
            {
                bool        fFoundThis = false;
                DISPID*     prgIDs = NULL;
                VARIANTARG* prgParams = NULL;
                DISPPARAMS  dpNew;

                for (unsigned int j = 0; j < pDispParams->cNamedArgs; j++)
                {
                    if (pDispParams->rgdispidNamedArgs[i] == DISPID_THIS)
                    {
                        fFoundThis = true;
                        break;
                    }
                }

                if (!fFoundThis)
                {
                    //
                    // GC cleans up these allocs when we're done
                    //
                    prgIDs = (DISPID*)mem.malloc(sizeof(DISPID)*(pDispParams->cNamedArgs+1));
                    if (NULL == prgIDs)
                        return E_OUTOFMEMORY;
                    prgParams = (VARIANTARG*)mem.malloc(sizeof(VARIANTARG)*(pDispParams->cArgs+1));
                    if (NULL == prgParams)
                        return E_OUTOFMEMORY;

                    memcpy(&prgIDs[1], pDispParams->rgdispidNamedArgs,
                            pDispParams->cNamedArgs * sizeof(pDispParams->rgdispidNamedArgs[0]));
                    prgIDs[0] = DISPID_THIS;

                    memcpy(&prgParams[1], pDispParams->rgvarg,
                            pDispParams->cArgs * sizeof(pDispParams->rgvarg[0]));
                    V_VT(&prgParams[0]) = VT_DISPATCH;
                    V_DISPATCH(&prgParams[0]) = m_pdispSource;

                    dpNew.cNamedArgs = pDispParams->cNamedArgs+1;
                    dpNew.cArgs = pDispParams->cArgs+1;
                    dpNew.rgdispidNamedArgs = prgIDs;
                    dpNew.rgvarg = prgParams;

                    hr = m_pdispSink->Invoke(m_prgMethods[i].idHandler, riid, lcid,
                                             wFlags, &dpNew, pvarResult,
                                             pExcepInfo, puArgErr);

                    memset(prgIDs, 0, sizeof(prgIDs[0]) * dpNew.cNamedArgs);
                    memset(prgParams, 0, sizeof(prgParams[0]) * dpNew.cArgs);

                    dpNew.rgdispidNamedArgs = prgIDs = NULL;
                    dpNew.rgvarg = prgParams = NULL;
                }
                else
                {
                    hr = m_pdispSink->Invoke(m_prgMethods[i].idHandler, riid, lcid,
                                             wFlags, pDispParams, pvarResult,
                                             pExcepInfo, puArgErr);
                }
            }
            else
            {
                if (NULL != pvarResult)
                    VariantInit(pvarResult);

                hr = S_OK;
            }

            break;
        }
    }

    //ENTRYLOG(L"COMEVT: EXIT _DoInvoke() hr = 0x%08lx\n", hr);
    return hr;
} // _DoInvoke




