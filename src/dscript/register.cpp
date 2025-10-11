/* Digital Mars DMDScript source code.
 * Copyright (c) 2000-2001 by Chromium Communications
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

#include <windows.h>
#include <objbase.h>
#include <comcat.h>      // for category registration stuff
#include <activscp.h>

#include "register.h"
#include "dscript.h"

#if defined linux
#  include <string.h>
#endif

#define LOG 0

//
// Forward declarations
//
static BOOL
GetKeyStringValue(const char* pszKeyPath,
                  const char* szValueName,
                  const char* szValue,
                  const UINT cchValue);

static BOOL
SetKeyStringValue(const char* pszKeyPath,
                  const char* szSubkey,
                  const char* szValueName,
                  const char* szValue);

static BOOL
SetKeyDefaultValue(const char* pszPath,
                   const char* szSubkey,
                   const char* szValue);

static void
CLSIDToString(const CLSID& clsid,
              char* szCLSID,
              int length);

static LONG
DeleteKeys(HKEY hKeyParent,
           const char* szKeyChild);

//
// Constant declarations
//
static const int CLSID_STRING_SIZE = 39;


////////////////////////////////////////
//
// RegisterCategories()
//
////////////////////////////////////////
static HRESULT RegisterCategories(CLSID clsid, bool fRegister)
{
    CoInitialize(NULL);

    ICatRegister*   pCR = NULL;
    CATID           rgIDs[2];
    HRESULT         hr = E_UNEXPECTED;

    rgIDs[0] = CATID_ActiveScript;
    rgIDs[1] = CATID_ActiveScriptParse;

    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
                          NULL, CLSCTX_INPROC_SERVER, IID_ICatRegister, (void**)&pCR);
    if (SUCCEEDED(hr)) {
       //
        // Register these category as being "implemented" by the class.

        if (fRegister)
            hr = pCR->RegisterClassImplCategories(clsid, sizeof(rgIDs)/sizeof(rgIDs[0]), rgIDs);
        else
            hr = pCR->UnRegisterClassImplCategories(clsid, sizeof(rgIDs)/sizeof(rgIDs[0]), rgIDs);

        pCR->Release();
        pCR = NULL;
    }

    CoUninitialize();

    return hr;
} // RegisterCategories


#if defined linux

static const char* gs_pcszExtraProgIDs[] = { "LiveScript",
                                             "JavaScript",
                                             "JavaScript1.1",
                                             "JavaScript1.2" };
#endif // linux

////////////////////////////////////////
//
// RegisterServer()
//
////////////////////////////////////////
HRESULT
RegisterServer(HMODULE hModule,
               const CLSID& clsid,
               const char* szFriendlyName,
               const char* szVerIndProgID,
               const char* szProgID)
{
    char    szCLSID[CLSID_STRING_SIZE];
    char    szModule[MAX_PATH * 2];
    char    szKey[MAX_PATH];
    DWORD   dwResult = 0;

#if LOG
    PRINTF("RegisterServer(hModule = x%x)\n", hModule);
#endif
    dwResult = GetModuleFileNameA(hModule,
                                  szModule,
                                  sizeof(szModule)/sizeof(char));
    if (0 == dwResult)
    {
	int err = GetLastError();
#if LOG
	PRINTF("GetModuleFileNameA() failed with %d\n", err);
	if (err == ERROR_ALREADY_EXISTS)
	    PRINTF("ERROR_ALREADY_EXISTS\n");
#endif
        return HRESULT_FROM_WIN32(err);
    }

    CLSIDToString(clsid, szCLSID, sizeof(szCLSID));

    strcpy(szKey, "CLSID\\");
    strcat(szKey, szCLSID);

    SetKeyDefaultValue(szKey,
                       NULL,
                       szFriendlyName);

    SetKeyDefaultValue(szKey,
                       "InprocServer32",
                       szModule);

    SetKeyDefaultValue(szKey,
                       "OLEScript",
                       NULL);

    SetKeyStringValue(szKey,
                      "InprocServer32",
                      "ThreadingModel",
                      "Apartment");

#if defined linux
    //
    //         Linux COM doesn't support Version Independent ProgID's,
    //         so link the CLSID only to what we would normally call
    //         VerInd ProgID, but call it the ProgID.
    //
    SetKeyDefaultValue(szKey,
                       "ProgID",
                       szVerIndProgID);

    //
    // Now setup the ProgID to point back at the CLSID
    //
    SetKeyDefaultValue(szVerIndProgID,
                       NULL,
                       szFriendlyName);

    SetKeyDefaultValue(szVerIndProgID,
                       "CLSID",
                       szCLSID);

    for (unsigned jj = 0; jj < sizeof(gs_pcszExtraProgIDs)/sizeof(gs_pcszExtraProgIDs[0]); jj++)
    {
        SetKeyDefaultValue(gs_pcszExtraProgIDs[jj],
                           NULL,
                           szFriendlyName);

        SetKeyDefaultValue(gs_pcszExtraProgIDs[jj],
                           "CLSID",
                           szCLSID);
    }

#else // !defined(linux)

    //
    // On Windows, proceed as normal
    //

    //
    // Link from the CLSID to the ProgID and VerInd ProgID.
    //
    SetKeyDefaultValue(szKey,
                       "ProgID",
                       szProgID);

    SetKeyDefaultValue(szKey,
                       "VersionIndependentProgID",
                       szVerIndProgID);

    //
    // Now link back from both ProgIDs to the CLSID, and from
    // the VerInd to the ProgID through CurVer
    //
    SetKeyDefaultValue(szVerIndProgID,
                       NULL,
                       szFriendlyName);

    SetKeyDefaultValue(szVerIndProgID,
                       "CLSID",
                       szCLSID);

    SetKeyDefaultValue(szVerIndProgID,
                       "CurVer",
                       szProgID);

    SetKeyDefaultValue(szProgID,
                       NULL,
                       szFriendlyName);

    SetKeyDefaultValue(szProgID,
                       "CLSID",
                       szCLSID);
#endif // !defined(linux)

//
// Only do protocol handler and Component categories stuff on Windows
//
#if !defined(linux)
    //
    // Now look and see if we can find a registered "javascript:"
    // Asynchronous Pluggable Protocol handler. If we find one, then
    // piggyback off IE's object that implements this to do one for
    // dScript as well (because we can, so why write our own?).
    //
    // NOTE: IE uses the same object for both "javascript:" and "vbscript:"
    //       which is how I surmised that it will work for any script.
    //       Therefore, we don't need to change this code for any other
    //       languages we might want to support in the future.
    //
    if (GetKeyStringValue("PROTOCOLS\\Handler\\javascript",
                          "CLSID",
                          szKey, sizeof(szKey)/sizeof(szKey[0])))
    {
        char    szProtKey[64];
        char*   pszEnd = NULL;
        const char*   pszAppend = szVerIndProgID;

        strcpy(szProtKey, "PROTOCOLS\\Handler\\");
        //
        // The convention is to register the APP's as lowercase strings,
        // but the ProgID might not be lowercase, so convert and append
        // it to our buffer in one pass.
        //
        pszEnd = szProtKey + strlen(szProtKey);
        while('\0' != *pszAppend)
        {
            *pszEnd++ = (char)tolower(*pszAppend);
            pszAppend++;
        }
        *pszEnd = '\0';

        SetKeyStringValue(szProtKey,
                          NULL,
                          "CLSID",
                          szKey);
    }

    HRESULT hr = RegisterCategories(clsid, true);
    if (FAILED(hr))
    {
        PRINTF("Register categories failed.");
    }
#if LOG
    PRINTF("-RegisterServer() = x%x\n", hr);
#endif
    return hr;
#else // !defined(linux)
    return S_OK;
#endif
} // RegisterServer


////////////////////////////////////////
//
// UnregisterServer()
//
////////////////////////////////////////
HRESULT
UnregisterServer(const CLSID& clsid,
                 const char* szVerIndProgID,
                 const char* szProgID)
{
    char    szCLSID[CLSID_STRING_SIZE];
    char    szKey[MAX_PATH];
    HRESULT hr = S_OK;
    LONG    lResult = ERROR_SUCCESS;

#if LOG
    PRINTF("UnregisterServer()\n");
#endif

    CLSIDToString(clsid, szCLSID, sizeof(szCLSID));

    strcpy(szKey, "CLSID\\");
    strcat(szKey, szCLSID);

    //
    // Don't fail and return if key doesn't exist - keep trying to clean up
    //

#if !defined(linux)
    //
    // Must do Component Category unregister before we unceremoniously
    // kill all the reg keys under our HKCR\CLSID\{...} key, which
    // the ComCat manager requires to do its work...
    //
    if (FAILED(RegisterCategories(clsid, false)))
    if (FAILED(hr))
    {
        PRINTF("Register categories failed.");
        hr = E_FAIL;
    }
#endif // !defined(linux)

    //
    // Delete the CLSID subtree and VerInd ProgID regardless of platform
    //
    lResult = DeleteKeys(HKEY_CLASSES_ROOT,
                         szKey);
    if (ERROR_SUCCESS != lResult) {
        hr = E_FAIL;
    }

    lResult = DeleteKeys(HKEY_CLASSES_ROOT,
                         szVerIndProgID);
    if (ERROR_SUCCESS != lResult) {
        hr = E_FAIL;
    }

#if defined linux

    for (unsigned jj = 0; jj < sizeof(gs_pcszExtraProgIDs)/sizeof(gs_pcszExtraProgIDs[0]); jj++)
    {
        lResult = DeleteKeys(HKEY_CLASSES_ROOT,
                             gs_pcszExtraProgIDs[jj]);
        if (ERROR_SUCCESS != lResult) {
            hr = E_FAIL;
        }
    }

#else // !linux

    lResult = DeleteKeys(HKEY_CLASSES_ROOT,
                         szProgID);
    if (ERROR_SUCCESS != lResult) {
        hr = E_FAIL;
    }

#endif // !linux

#if !defined(linux)
    char    szProtKey[64];

    strcpy(szProtKey, "PROTOCOLS\\Handler\\");
    //
    // When we registered, we made sure to lowercase the progid,
    // but since reg keys aren't case sensitive, that doesn't matter
    // just to find the key for deletion, so skip it.
    //
    strcat(szProtKey, szVerIndProgID);

    lResult = DeleteKeys(HKEY_CLASSES_ROOT,
                         szProtKey);
    if (ERROR_SUCCESS != lResult) {
        hr = E_FAIL;
    }
#endif // !defined(linux)

    return hr;
} // UnregisterServer


////////////////////////////////////////
//
// CLSIDToString()
//
////////////////////////////////////////
static void
CLSIDToString(const CLSID& clsid,
              char* szCLSID,
              int length)
{
    LPOLESTR wszCLSID = NULL;
    HRESULT hr;

    if (length < CLSID_STRING_SIZE) {
        return;
    }

//    asm("int3");
    hr = StringFromCLSID(clsid, &wszCLSID);
    if (FAILED(hr)) {
#if LOG
	PRINTF("CLSIDToString() failed with hr = x%x\n", hr);
#endif
        return;
    }

    wcstombs(szCLSID,
             wszCLSID,
             length);

    CoTaskMemFree(wszCLSID);
#if LOG
    PRINTF("CLSIDToString() returns '%s'\n", szCLSID);
#endif
} // CLSIDToString


////////////////////////////////////////
//
// DeleteKeys()
//
////////////////////////////////////////
static LONG
DeleteKeys(HKEY hKeyParent,
           const char* lpszKeyChild)
{
    HKEY hKeyChild = NULL;
    LONG lRes;
    FILETIME time;
    char szBuffer[MAX_PATH];
    DWORD dwSize;

#if LOG
    PRINTF("DeleteKeys()\n");
#endif
    lRes= RegOpenKeyExA(hKeyParent,
                        lpszKeyChild,
                        0,
                        KEY_ALL_ACCESS,
                        &hKeyChild);

    if (ERROR_SUCCESS != lRes) {
        return lRes;
    }

    dwSize = MAX_PATH;
    while (RegEnumKeyExA(hKeyChild,
                         0,
                         szBuffer,
                         &dwSize,
                         NULL,
                         NULL,
                         NULL,
                         &time) == S_OK) {

        lRes = DeleteKeys(hKeyChild, szBuffer);

        if (ERROR_SUCCESS != lRes) {
            RegCloseKey(hKeyChild);
            RegCloseKey(hKeyParent);
            return lRes;
        }

        dwSize = MAX_PATH;
    }

    RegCloseKey(hKeyChild);

    return RegDeleteKeyA(hKeyParent, lpszKeyChild);
} // DeleteKeys


////////////////////////////////////////
//
// GetKeyStringValue()
//
// Open a reg key and get a string (REG_SZ) value within it.
//
////////////////////////////////////////
static BOOL
GetKeyStringValue(const char* szKeyPath,
                  const char* szValueName,
                  const char* szValue,
                  const UINT cchValue)
{
#if LOG
    PRINTF("GetKeyStringValue(szKeyPath = '%s', szValueName = '%s')\n",
	szKeyPath, szValueName);
#endif

    HKEY hKey = NULL;
    long lResult;

    if (NULL == szValue)
        return FALSE;

    lResult = RegOpenKeyExA(HKEY_CLASSES_ROOT,
                            szKeyPath,
                            0,
                            KEY_QUERY_VALUE,
                            &hKey);
    if (ERROR_SUCCESS != lResult) {
        PRINTF("GetKeyStringValue : failed to open key '%s'", szKeyPath);
        return FALSE;
    }

    DWORD   dwType;
    DWORD   cchBuffer = cchValue;

    lResult = RegQueryValueExA(hKey,
                               szValueName,
                               NULL,
                               &dwType,
                               (BYTE *)szValue,
                               &cchBuffer);
    RegCloseKey(hKey);

    if ((ERROR_SUCCESS != lResult) ||
        (REG_SZ != dwType))
    {
        PRINTF("GetKeyStringValue : failed to open value '%s'", szValueName);
        return FALSE;
    }

    return TRUE;
} // GetKeyStringValue


////////////////////////////////////////
//
// SetKeyStringValue()
//
// Create/open a reg key and set a string (REG_SZ) value within it.
//
////////////////////////////////////////
static BOOL
SetKeyStringValue(const char* szKeyPath,
                  const char* szSubkey,
                  const char* szValueName,
                  const char* szValue)
{
#if LOG
    PRINTF("SetKeyStringValue(szKeyPath = '%s', szSubkey = '%s', szValueName = '%s', szValue = '%s')\n",
	szKeyPath, szSubkey, szValueName, szValue);
#endif
    HKEY hKey = NULL;
    char szKeyBuf[MAX_PATH * 2];
    long lResult;

//    asm("int3");

    strcpy(szKeyBuf, szKeyPath);

    if (NULL != szSubkey) {
        strcat(szKeyBuf, "\\");
        strcat(szKeyBuf, szSubkey);
    }

    lResult = RegCreateKeyExA(HKEY_CLASSES_ROOT,
                              szKeyBuf,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hKey,
                              NULL);
    if (ERROR_SUCCESS != lResult) {
        PRINTF("SetKeyStringValue : failed to create key '%s'", szKeyBuf);
        return FALSE;
    }

    if (NULL != szValue) {
        lResult = RegSetValueExA(hKey,
                                 szValueName,
                                 0,
                                 REG_SZ,
                                 (BYTE *)szValue,
                                 strlen(szValue)+1);
        if (ERROR_SUCCESS != lResult) {
            PRINTF("SetKeyStringValue : failed to set key value '%s'", szValueName);
            RegCloseKey(hKey);
            return FALSE;
        }
    }

    RegCloseKey(hKey);

    return TRUE;
} // SetKeyStringValue


////////////////////////////////////////
//
// SetKeyDefaultValue()
//
// Create/open a reg key and set the value
// of it's "default" (unnamed) value.
//
////////////////////////////////////////
static BOOL
SetKeyDefaultValue(const char* szKey,
                   const char* szSubkey,
                   const char* szValue)
{
#if LOG
    PRINTF("SetKeyDefaultValue(szKey = '%s', szSubkey = '%s', szValue = '%s')\n", szKey, szSubkey, szValue);
#endif
    return SetKeyStringValue(szKey, szSubkey, NULL, szValue);
} // SetKeyDefaultValue

