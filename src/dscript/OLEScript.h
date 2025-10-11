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

#ifndef __OLESCRIPT_H__
#define __OLESCRIPT_H__

#include <activscp.h>
#include <objsafe.h>


#include "program.h"
#include "dcomobject.h"
#include "OLECommon.h"
#include "NamedItemDisp.h"
#ifndef DISABLE_EVENT_HANDLING
#include "EventHandlers.h"
#endif // !DISABLE_EVENT_HANDLING
#include "hostinfo.h"

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// SCRIPT_E_REPORTED???
// --------------------
//
// This is a "special" HRESULT that Microsoft engines sometimes return.
//
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// NOTE: This error method is used for methods that otherwise have an out param
//       of type EXCEPINFO*.
//

const HRESULT SCRIPT_E_REPORTED     = 0x80020101;

//
// We need to use this to tell ExecuteParseScript() what mode it should run
// in. It gets stored in the same dwFlags fields as other SCRIPTPROC_* flags.
//
const DWORD SCRIPTPROC_PRIV_IMPLEMENTATIONCODE  = (0x80000000);

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// MODULES
//
// Modules are the namespaces that contain various named items, and are
// used for resolving references to functions, properties, and such. Normally
// the correlation is 1:1 between NamedItems and Modules/Namespaces, except
// that if the NamedItem is added with SCRIPTITEM_GLOBALMEMBERS then it is in
// the global module, which also contains any names at global scope in the
// script (typically function names that are defined and variables). If the
// NamedItem at global scope is also SCRIPTITEM_ISVISIBLE, its name is also
// visible as an item in the global module.
//
// We use UINT's to assign unique numbers to modules. The global module is 0.
//
const UINT GLOBAL_MODULE            = 0;


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// struct SNamedItemData
//
struct SNamedItemData : public Mem
{
public:
#if INVARIANT
#   define SNAMEDITEMDATA_SIGNATURE	0xAEAE5554
    unsigned signature;

    void invariant()
    {
	this->check(this);
	assert(signature == SNAMEDITEMDATA_SIGNATURE);
    }
#else
    void invariant() { }
#endif


    SNamedItemData()
      : pNext(NULL),
        bszItemName(NULL),
        dwAddFlags(0),
        uModuleNumber(0),
        pWrapper(NULL),
        pModuleDisp(NULL),
        fHasCode(false),
        fRegistered(false)
    {
#if INVARIANT
	signature = SNAMEDITEMDATA_SIGNATURE;
#endif
    }

    ~SNamedItemData()
    {
	invariant();
        if (NULL != bszItemName)
        {
            SysFreeString(bszItemName);
            bszItemName = NULL;
        }

        //
        // We're holding a COM reference because otherwise the object could be deleted
        // from under us if the COM client were to Release it's last reference.
        //
        if (NULL != pModuleDisp)
        {
            pModuleDisp->Release();
            pModuleDisp = NULL;
        }

        if (NULL != pWrapper)
        {
            //delete pWrapper;
	    pWrapper->finalizer();	// call Release() on all it holds

            pWrapper = NULL;
        }

#if INVARIANT
	signature = 0;
#endif
    }

public:
    SNamedItemData* pNext;

    BSTR        bszItemName;    // Store as BSTR since GetItemInfo() requires a BSTR
    DWORD       dwAddFlags;
    UINT        uModuleNumber;
    Dcomobject* pWrapper;
    IDispatch*  pModuleDisp;

    bool        fHasCode;
    bool        fRegistered;
}; // struct SNamedItemData


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// struct SParsedScriptData
//
struct SParsedScriptData : public Mem
{
public:
#if INVARIANT
#   define SPARSEDSCRIPTDATA_SIGNATURE	0xAEBF5754
    unsigned signature;

    void invariant()
    {
	this->check(this);
	assert(signature == SPARSEDSCRIPTDATA_SIGNATURE);
    }
#else
    void invariant() { }
#endif

    SParsedScriptData()
      : pNext(NULL),
        pfdCode(NULL),
        dwFlags(0),
	dwSourceContext(0),
        uModuleContext(0),
        fExecuted(true)
    {
#if INVARIANT
	signature = SPARSEDSCRIPTDATA_SIGNATURE;
#endif
    }

    ~SParsedScriptData()
    {
	invariant();

	pNext = NULL;
        pfdCode = NULL;		// Help the GC by clearing this pointer
	dwSourceContext = 0;
#if INVARIANT
	signature = 0;
#endif
    }

public:
    SParsedScriptData*  pNext;
    FunctionDefinition* pfdCode;            // The IR representing the code itself.

    DWORD               dwFlags;            // Code was added with these flags
    DWORD		dwSourceContext;    // host-defined context
    UINT                uModuleContext;     // Code is in the context of what module?

    bool                fExecuted;          // Code has been executed (therefore, it's not pending)?
}; // struct SParsedScriptData


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// class COleScript
//
// Manages and maintains the state of the script.
//
//
class COleScript :  public CBaseComObject,
#ifndef DISABLE_OBJECT_SAFETY
                    public IObjectSafety,
#endif // !DISABLE_OBJECT_SAFETY
		    public IHostInfoUpdate,
                    public IActiveScript,
                    public IActiveScriptParse,
                    public IActiveScriptParseProcedure
{
public:
#if INVARIANT
#   define COLESCRIPT_SIGNATURE	0xEAE5A545
    unsigned signature;

    void invariant()
    {
	assert(signature == COLESCRIPT_SIGNATURE);
    }
#else
    void invariant() { }
#endif

    //
    // *** Constructor / Destructor ***
    //
    COleScript();

protected:
    virtual ~COleScript();

public:
    //
    // *** IUnknown Methods ***
    //
    STDMETHODIMP_(ULONG) AddRef() {return CBaseComObject::AddRef();};
    STDMETHODIMP_(ULONG) Release() {return CBaseComObject::Release();};
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);

    //
    // *** IActiveScript Methods ***
    //
    STDMETHOD(SetScriptSite)(/*[in] */ IActiveScriptSite* passSite);

    STDMETHOD(GetScriptSite)(/*[in] */ REFIID riid,
                             /*[out]*/ void** ppvSite);

    STDMETHOD(SetScriptState)(/*[in] */ SCRIPTSTATE ss);

    STDMETHOD(GetScriptState)(/*[out]*/ SCRIPTSTATE* pssState);

    STDMETHOD(Close)(void);

    STDMETHOD(AddNamedItem)(/*[in] */ LPCOLESTR pcwszName,
                            /*[in] */ DWORD dwFlags);

    STDMETHOD(AddTypeLib)(/*[in] */ REFGUID rguidTypeLib,
                          /*[in] */ DWORD dwMajor,
                          /*[in] */ DWORD dwMinor,
                          /*[in] */ DWORD dwFlags);

    STDMETHOD(GetScriptDispatch)(/*[in] */ LPCOLESTR pcwszItemName,
                                 /*[out]*/ IDispatch** ppDisp);

    STDMETHOD(GetCurrentScriptThreadID)(/*[out]*/ SCRIPTTHREADID* pstidThread);

    STDMETHOD(GetScriptThreadID)(/*[in] */ DWORD dwWin32ThreadId,
                                 /*[out]*/ SCRIPTTHREADID* pstidThread);

    STDMETHOD(GetScriptThreadState)(/*[in] */ SCRIPTTHREADID stidThread,
                                    /*[out]*/ SCRIPTTHREADSTATE* pstsState);

    STDMETHOD(InterruptScriptThread)(/*[in] */ SCRIPTTHREADID stidThread,
                                     /*[in] */ const EXCEPINFO* pexcepinfo,
                                     /*[in] */ DWORD dwFlags);

    STDMETHOD(Clone)(/*[out]*/ IActiveScript** ppscript);

    //
    // *** IActiveScriptParse Methods ***
    //
    STDMETHOD(InitNew)();

    STDMETHOD(AddScriptlet)(/*[in] */ LPCOLESTR   pcwszDefaultName,
                            /*[in] */ LPCOLESTR   pcwszCode,
                            /*[in] */ LPCOLESTR   pcwszItemName,
                            /*[in] */ LPCOLESTR   pcwszSubItemName,
                            /*[in] */ LPCOLESTR   pcwszEventName,
                            /*[in] */ LPCOLESTR   pcwszDelimiter,
                            /*[in] */ DWORD       dwSourceContext,
                            /*[in] */ ULONG       ulStartingLineNumber,
                            /*[in] */ DWORD       dwFlags,
                            /*[out]*/ BSTR*       pbstrName,
                            /*[out]*/ EXCEPINFO*  pexcepinfo);

    STDMETHOD(ParseScriptText)(/*[in] */ LPCOLESTR   pcwszCode,
                               /*[in] */ LPCOLESTR   pcwszItemName,
                               /*[in] */ IUnknown*   punkContext,
                               /*[in] */ LPCOLESTR   pcwszDelimiter,
                               /*[in] */ DWORD       dwSourceContext,
                               /*[in] */ ULONG       ulStartingLineNumber,
                               /*[in] */ DWORD       dwFlags,
                               /*[out]*/ VARIANT*    pvarResult,
                               /*[out]*/ EXCEPINFO*  pexcepinfo);

    //
    // *** IActiveScriptParseProcedure Methods ***
    //
    STDMETHOD(ParseProcedureText)(/*[in] */ LPCOLESTR   pcwszCode,
                                  /*[in] */ LPCOLESTR   pcwszFormalParams,
                                  /*[in] */ LPCOLESTR   pcwszProcedureName,
                                  /*[in] */ LPCOLESTR   pcwszItemName,
                                  /*[in] */ IUnknown*   punkContext, 
                                  /*[in] */ LPCOLESTR   pcwszDelimiter,
                                  /*[in] */ DWORD       dwSourceContextCookie,
                                  /*[in] */ ULONG       ulStartingLineNumber,
                                  /*[in] */ DWORD       dwFlags,
                                  /*[out]*/ IDispatch** ppDisp);

#ifndef DISABLE_OBJECT_SAFETY
    //
    // *** IObjectSafety Methods ***
    //
    STDMETHOD(GetInterfaceSafetyOptions)(/*[in] */ REFIID riid,
                                         /*[out]*/ DWORD* pdwSupportedOptions,
                                         /*[out]*/ DWORD* pdwEnabledOptions);

    STDMETHOD(SetInterfaceSafetyOptions)(/*[in] */ REFIID riid,
                                         /*[in] */ DWORD  dwOptionsSetMask,
                                         /*[in] */ DWORD  dwEnabledOptions);
#endif // !DISABLE_OBJECT_SAFETY

    //
    // *** IHostUpdateInfo Methods ***
    //

    STDMETHOD(UpdateInfo) (/*[in] */ hostinfo hostinfoNew);

public:
    int ExecuteParsedScript(FunctionDefinition* pfdCode,
                               Dobject* poThis, SNamedItemData* pnid,
                               DWORD dwFlags, VARIANT* pvarResult, ErrInfo *perrinfo);
    HRESULT HandleScriptError(ErrInfo *perrinfo, EXCEPINFO* pei, DWORD dwSourceContextCookie);

    void EnterScript();
    void ExitScript();

    CallContext* SwitchGlobalProgramContext(CallContext* pccNew);
    Program* SwitchToOurGlobalProgram();
    void RestoreSavedGlobalProgram(Program* pProgram);
    void SetGlobalProgramContext(CallContext* pccNew);

    static CallContext* _CopyCallContext(CallContext* pccRight);

private:
    // Call ChangeScriptState ONLY in the base thread, whenever you want to change the
    // Script state
    void _ChangeScriptState(SCRIPTSTATE ss);
    void _SetLocale(void);

    HRESULT _ExecutePendingScripts();

    CallContext* _CopyGlobalContext();
    CallContext* _BuildNamedItemContext(SNamedItemData* pnid);

    int _ExecuteParsedScriptBlock(SParsedScriptData* pCode,
                                     Dobject* poThis, SNamedItemData* pnid,
                                     DWORD dwFlags, VARIANT* pvarResult, ErrInfo *perrinfo);

    SNamedItemData* _FindNamedItem(/*[in] */ LPCOLESTR pcwszName);
    SNamedItemData* _FindNamedItem(/*[in] */ UINT uModule);

    HRESULT _ResetNamedItems();
    HRESULT _RegisterPersistentNamedItems();
    HRESULT _RegisterNamedItem(SNamedItemData* pnid);

    HRESULT _ResetScriptItems();

    HRESULT Run();
    HRESULT Stop();
    HRESULT Disconnect();
    HRESULT PseudoDisconnect();
    HRESULT Reconnect();
    HRESULT Reset(bool fReRegister);

    HRESULT SetDefaultDispatch(UINT uModule, IDispatch* pDisp);
    HRESULT SetDefaultThis(IDispatch* pDisp);

    HRESULT _AddScriptTextToNamedItem(SNamedItemData* pnid,
                                      const WCHAR* pcwszCode,
                                      DWORD dwFlags,
                                      EXCEPINFO* pexcepinfo,
				      DWORD dwSourceContext);

    HRESULT _BuildScriptDispatchWrapper(SNamedItemData* pnid, IDispatch** ppDisp,
		DWORD dwSourceContext);

#ifndef DISABLE_EVENT_HANDLING

    HRESULT _BuildHandlerName(const WCHAR* pcwszItemName,
                              const WCHAR* pcwszSubItemName,
                              WCHAR** ppwszHandler);

    HRESULT _BuildFunctionNameAndBody(const WCHAR* pcwszHandlerName,
                                      const WCHAR* pcwszEventName,
                                      const WCHAR* pcwszCode,
                                      const WCHAR* pcwszDelimiter,
                                      WCHAR** ppwszFuncName,
                                      WCHAR** ppwszFunc);

    bool _IsHTMLScriptDelimiter(const WCHAR* pcwszDelim);

    HRESULT _BuildDispatchEventSink(SNamedItemData* pnid, const WCHAR* pcwszSubItemName,
                                    const WCHAR* pcwszHandlerName, CDispatchEventSink** ppSink);

    HRESULT _InitializeEventSink(SNamedItemData* pnid, const WCHAR* pcwszSubItemName,
                                 const WCHAR* pcwszHandlerName, CDispatchEventSink* pSink);

    HRESULT _GetDefaultSourceTypeInfo(ITypeInfo* ptiCoClass, ITypeInfo** pptiSource);

    HRESULT _GetNamedItemEventSink(SNamedItemData* pnid, const WCHAR* pcwszSubItemName,
                                   const WCHAR* pcwszHandlerName, CDispatchEventSink** ppSink);

    HRESULT _GetNamedItemDispatch(const WCHAR* pcwszItem,
                                  const WCHAR* pcwszSubItem,
                                  IDispatch** ppDisp);

    CDispatchEventSink* _FindEventSink(const WCHAR* pcwszHandlerName);

#endif // !DISABLE_EVENT_HANDLING

    //
    // These are outside the DISABLE_EVENT_HANDLING so I can stub them
    // to empty functions in non-event handler builds.
    //
    HRESULT _ConnectEventHandlers();
    HRESULT _DisconnectEventHandlers();
    HRESULT _TeardownEventHandlers();


private:
    IActiveScriptSite*  m_pScriptSite;      // Interface back to the client of this engine
    SNamedItemData*     m_pNamedItems;      // List of named item objects we currently know about
    SParsedScriptData*  m_pParsedScripts;   // List of parsed script text blobs we have

    IDispatch*          m_pGlobalModuleDisp;// Dispatch wrapper for the global module
#ifndef DISABLE_EVENT_HANDLING
    Array               m_rgEventSinks;     // Array of CDispatchEventSink*'s
#endif // !DISABLE_EVENT_HANDLING

    Program*            m_pProgram;
    CallContext*        m_pccGlobal;

    SCRIPTTHREADSTATE   m_stsState;
    SCRIPTSTATE         m_ssState;          // Script engine state

#ifndef DISABLE_OBJECT_SAFETY
    DWORD       m_dwCurrentSafetyOptions;   // Current IObjectSafety option flags
#endif // !DISABLE_OBJECT_SAFETY

    DWORD       m_dwRuntimeUniqueNumber;    // Used for generating unique event handler function names
    UINT        m_uNextModuleNumber;        // Used for assigning unique "module" ID's for NamedItem's

    LCID        m_lcid;                     // Our locale identifier (unused right now)

    bool        m_fPersistLoaded;           // Were we Init'ed from a Persist or Parse interface?
    bool        m_fIsPseudoDisconnected;
};


#endif // __OLESCRIPT_H__



