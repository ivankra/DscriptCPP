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

#ifndef _REGISTER_H_

#define _REGISTER_H_

HRESULT 
RegisterServer(HMODULE hModule, 
               const CLSID& clsid, 
               const char* szFriendlyName,
               const char* szVerIndProgID,
               const char* szProgID) ;

HRESULT 
UnregisterServer(const CLSID& clsid,
                 const char* szVerIndProgID,
                 const char* szProgID) ;

#endif //_REGISTER_H_
