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

#ifndef _EVENTHANDLERS_H_
#define _EVENTHANDLERS_H_

#include "BaseDispatch.h"
#include "OLECommon.h"
#if defined linux
#include "ocidl.h"
#endif

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// class CDispatchEventSink
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
class CDispatchEventSink : public CBaseDispatch
{
public:
#if INVARIANT
#   define CDISPATCHEVENTSINK_SIGNATURE	0xFAEE3154
    unsigned signature;

    void invariant()
    {
	assert(signature == CDISPATCHEVENTSINK_SIGNATURE);
    }
#else
    void invariant() { }
#endif

    //
    // Constructors/destructor
    //
    CDispatchEventSink();

    HRESULT Initialize(const WCHAR* pcwszName, IDispatch* pdispSource,
                       ITypeInfo* ptiSource, IDispatch* pdispSink);

    HRESULT Connect();
    HRESULT Disconnect();
    HRESULT Teardown();

    HRESULT AttachEvent(const WCHAR* pcwszEvent, const WCHAR* pcwszHandler);

    inline const WCHAR* GetName()   { return m_pwszName; }
    inline bool IsInitialized()     { return m_fInitialized; }

protected:
    virtual ~CDispatchEventSink();  // We delete ourselves through Release() to zero

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    //
    // struct CDispatchEventSink::SEventMethodData
    //
    // Data on one method in an event sink interface. There's one of these
    // for each method on a ConnectionPoint interface -- SEventSet stores
    // a table of them. Not all of them will be connected to a handler though.
    //
    // PERF: Make a static _cdecl compare method so we can qsort & bsearch these?
    //
    // PERF: Or, we could re-sort the SEventSet each event firing so it's in MRU order
    //
    // This is allocated on the global heap, not the garbage collected one.
    struct SEventMethodData
    {
    public:
        SEventMethodData();
        ~SEventMethodData();

        BSTR        pbszName;       // Event's text name (e.g. "onload")
        DISPID      idEvent;        // DISPID on the ConnectionPoint interface

        DISPID      idHandler;      // DISPID of handler function in handler namespace
        bool        fHasHandler;    // Have we attached a handler for this method?
    };

    virtual HRESULT _DoGetIDsOfNames(REFIID riid, OLECHAR** prgszNames, UINT cNames,
                                     LCID lcid, DISPID* prgIDs);

    virtual HRESULT _DoInvoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
                              DISPPARAMS* pDispParams, VARIANT* pvarResult,
                              EXCEPINFO* pExcepInfo, UINT* puArgErr);

protected:
    WCHAR*      m_pwszName;         // The name of this control (== "item name" in the engine)

    IDispatch*  m_pdispSource;      // An IDispatch to the source object (for connecting)
    IConnectionPoint*   m_pCP;      // The connection point we're hooked up to
    IDispatch*  m_pdispSink;        // The engine's namespace dispatch for handlers

    IID         m_iidInterface;     // Source interface IID
    DWORD       m_dwCookie;         // Source ConnectionPoint Advise() cookie

    SEventMethodData* m_prgMethods; // Array of SEventMethodData's
    int         m_cMethods;

    bool        m_fInitialized;     // Have we been successfully initialized?
    bool        m_fConnected;       // Are we connected to the source?
}; // class CDispatchEventSink


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// *** Methods ***
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

inline CDispatchEventSink::SEventMethodData::SEventMethodData()
  : pbszName(NULL),
    idEvent(-1),
    idHandler(-1),
    fHasHandler(false)
{
}

inline CDispatchEventSink::SEventMethodData::~SEventMethodData()
{
    if (NULL != pbszName)
    {
        SysFreeString(pbszName);
        pbszName = NULL;
    }
}


#endif // _EVENTHANDLERS_H_



