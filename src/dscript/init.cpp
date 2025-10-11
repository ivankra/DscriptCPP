/* Digital Mars DMDScript source code.
 * Copyright (c) 2000-2002 by Chromium Communications
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

#if !defined linux
#define INITGUID
#endif

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#include <assert.h>

#if defined linux
#include <pthread.h>
#endif

#include "dscript.h"
#include "program.h"
#include "identifier.h"
#include "lexer.h"
#include "mem.h"
#include "printf.h"
#include "date.h"
#include "text.h"

#if defined __DMC__
#include "dmc_rpcndr.h"
#endif

#include <objbase.h>

#if _MSC_VER
#pragma warning(disable:4268)
#endif

#include <activscp.h>

#if _MSC_VER
#include <objsafe.h>
#pragma warning(default:4268)
#endif

#include "dsfactory.h"
#include "register.h"

#if _MSC_VER
#include <crtdbg.h>
#endif

#define LOG 0

//
// Global data
//
HMODULE g_hModule = 0;		// our DLL handle
LONG g_cComponents = 0;		// number of total instances of all outstanding
                            // COM objects for this DLL
LONG g_cServerLocks = 0;	// number of locks on DSFactory

//
// Component information
//
const char g_szFriendlyName[] = "DMDScript Scripting Engine";
#if defined linux
const char g_szVerIndProgID[] = "JScript";
const char g_szProgID[] = "JScript.1";
#else
const char g_szVerIndProgID[] = "DMDScript";
const char g_szProgID[] = "DMDScript.1";
#endif

//
// Module data
//
static const GUID CLSID_DScript_1 =
 { 0x440c0dbc, 0x4a0c, 0x4ed2, { 0xa5, 0xed, 0x13, 0x42, 0xed, 0xb, 0x50, 0x7c } };

extern "C"
BOOL
WINAPI
DllMain(
  HINSTANCE hinstDLL,
  DWORD fdwReason,
  LPVOID lpvReserved)
{
    printf_tologfile();		// send printf's to a log file instead of stdout

#if LOG
#if defined linux
    WPRINTF(L"Thread %lx: ", pthread_self());
#endif
    WPRINTF(L"DllMain(hinstDLL = x%x, fdwReason = %d)\n", hinstDLL, fdwReason);
    //WPRINTF(L"DllMain(lpvReserved = x%x, GetModuleHandle() = x%x)\n", lpvReserved, GetModuleHandle(NULL));
#endif

    mem.init();
    mem.setStackBottom((void *)&hinstDLL);

    ThreadContext::init();
    Date::init();

    BOOL bResult = TRUE;

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
       g_hModule = hinstDLL;

#if !defined linux
       static char dummy = 0;
       char*    pStart;
       SYSTEM_INFO  si;
       MEMORY_BASIC_INFORMATION mbi;

       GetSystemInfo(&si);
       pStart = (char*)((unsigned int)(&dummy) & ~(si.dwPageSize-1));
       if (sizeof(mbi) == VirtualQuery(pStart, &mbi, sizeof(mbi)))
       {
           mem.addroots((char*)mbi.BaseAddress, (char*)mbi.BaseAddress + mbi.RegionSize);
       }
#endif

       //
       // Intitialize script engine
       //
       //Lexer::initKeywords();

#if 0 && _MSC_VER
       // Get current flag
       int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );

       // Turn on leak checking, CRT block checking bit and check memory at every allocation.
       tmpFlag |= _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_CRT_DF | _CRTDBG_CHECK_ALWAYS_DF;

       // Set flag to the new value
       _CrtSetDbgFlag( tmpFlag );
#endif

#if !defined linux
       //
       // We don't handle thread attach/detach messages, so we don't need to get them at all.
       //
       DisableThreadLibraryCalls(hinstDLL);
#endif
    }
	    break;
	case DLL_PROCESS_DETACH:
	    break;
#if defined linux
	case DLL_THREAD_ATTACH:
	    // Notify garbage collector?
	    // Get thread stack base?
	    PRINTF("DLL_THREAD_ATTACH\n");
	    break;
	case DLL_THREAD_DETACH:
	    // Notify garbage collector?
	    PRINTF("DLL_THREAD_DETACH\n");
	    break;
#endif
    }

    return bResult;
}

extern "C"
STDAPI
DllCanUnloadNow()
{
#if LOG
    PRINTF("DllCanUnloadNow()\n");
#endif
    if ((0 == g_cComponents) &&
        (0 == g_cServerLocks)) {
        return S_OK;
    }

    return S_FALSE;
}

extern "C"
STDAPI
DllGetClassObject(const CLSID& clsid, const IID& iid, void ** ppv)
{
#if LOG
    PRINTF("DllGetClassObject()\n");
#endif
    mem.setStackBottom((void *)&ppv);

    if (NULL == ppv)
        return E_INVALIDARG;

    *ppv = NULL;        // Initialize the out-param to something reasonable

    if (clsid == CLSID_DScript_1)
    {

        DSFactory*  pFactory = NULL;
        HRESULT     hr = E_UNEXPECTED;

        pFactory = new DSFactory;
        if (NULL == pFactory) {
            return E_OUTOFMEMORY;
        }

        hr = pFactory->QueryInterface(iid, ppv);
        pFactory->Release();        // Release the ref from creation

        return hr;
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}

extern "C"
STDAPI
DllRegisterServer()
{
#if LOG
    PRINTF("DllRegisterServer()\n");
#endif
    return RegisterServer(g_hModule,
                          CLSID_DScript_1,
                          g_szFriendlyName,
                          g_szVerIndProgID,
                          g_szProgID);
}

extern "C"
STDAPI
DllUnregisterServer()
{
#if LOG
    PRINTF("DllUnregisterServer()\n");
#endif
    return UnregisterServer(CLSID_DScript_1,
                            g_szVerIndProgID,
                            g_szProgID);
}


/*******************************************
 * Pop up a message box.
 */

void vexception(dchar *format, va_list args)
{
    dchar buffer[128];
    dchar *p;
    int psize;
    int count;

    p = buffer;
    psize = sizeof(buffer) / sizeof(buffer[0]);
    for (;;)
    {
	count = VSWPRINTF(p,psize,format,args);
	if (count == -1)
	    psize *= 2;
	else if (count >= psize)
	    psize = count + 1;
	else
	    break;
	p = (dchar *) alloca(psize * sizeof(dchar));	// buffer too small, try again with larger size
	assert(p);
    }

    WPRINTF(L"%s\n", p);
#if defined _WIN32 && !defined linux
    MessageBox(NULL, p, DTEXT("DMDScript Error"), MB_OK);
#endif
}

/***************************************
 * Print out a message and continue on.
 */

void exception(dchar *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vexception(format,ap);
    va_end(ap);
}

/************************************
 * Convert from enum to dchar*
 */

dchar *errmsg(int msgnum)
{
#if defined UNICODE
    return ascii2unicode(errmsgtbl[msgnum]);
#else
    return errmsgtbl[msgnum];
#endif
}

/*****************************************
 * Use this for fatal runtime errors.
 */

void RuntimeErrorx(int msgnum, ...)
{
    WPRINTF(L"RuntimeErrorx()\n");
    va_list ap;
    va_start(ap, msgnum);
    vexception(errmsg(msgnum),ap);
    va_end(ap);
    exit(EXIT_FAILURE);			// cannot recover
}

/************************************
 * Assertion failures go here.
 */

#if defined __DMC__

extern "C"
{

void vexceptionc(char *format, va_list args)
{
    char buffer[128];
    char *p;
    unsigned psize;
    int count;

    p = buffer;
    psize = sizeof(buffer) / sizeof(buffer[0]);
    for (;;)
    {
#if defined linux
	count = vsnprintf(p,psize,format,args);
	if (count == -1)
	    psize *= 2;
	else if (count >= psize)
	    psize = count + 1;
	else
	    break;
#elif defined _WIN32
	count = _vsnprintf(p,psize,format,args);
	if (count != -1)
	    break;
	psize *= 2;
#endif
	p = (char *) alloca(psize * sizeof(buffer[0]));	// buffer too small, try again with larger size
	assert(p);
    }

    MessageBoxA(NULL, p, "DMDScript Error", MB_OK);
}

void exceptionc(char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vexceptionc(format,ap);
    va_end(ap);
}


extern void __cdecl _assert(const void *exp, const void *file, unsigned line)
{
    //*(char *)0 = 0;
    exceptionc("Assert fail %s(%d): '%s'", file, line, exp);
    *(char *)0 = 0;
}

}

#endif
