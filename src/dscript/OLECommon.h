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

#ifndef __OLECOMMON_H__
#define __OLECOMMON_H__

#include "dscript.h"

#if defined linux
#define UNREFERENCED_PARAMETER(x) (void) (x)
#endif

//#define COM_LOG_ALL
//#define COM_LOG_ENTER_EXIT

#ifdef COM_LOG_ALL
#define DPF     WPRINTF
#else
#define DPF	(void)
#endif // COM_LOG_ALL

#ifdef COM_LOG_ENTER_EXIT
#define ENTRYLOG    WPRINTF
#else
#define ENTRYLOG
#endif // COM_LOG_ENTER_EXIT

/*
	Handy reference for common HRESULT values:

	winerror.h

	S_OK				0
	S_FALSE				1

	DISP_E_UNKNOWNINTERFACE         0x80020001
	DISP_E_MEMBERNOTFOUND           0x80020003
	DISP_E_PARAMNOTFOUND            0x80020004
	DISP_E_TYPEMISMATCH             0x80020005
	DISP_E_UNKNOWNNAME              0x80020006
	DISP_E_NONAMEDARGS              0x80020007
	DISP_E_BADVARTYPE               0x80020008
	DISP_E_EXCEPTION                0x80020009
	DISP_E_OVERFLOW                 0x8002000A
	DISP_E_BADINDEX                 0x8002000B
	DISP_E_UNKNOWNLCID              0x8002000C
	DISP_E_ARRAYISLOCKED            0x8002000D
	DISP_E_BADPARAMCOUNT            0x8002000E
	DISP_E_PARAMNOTOPTIONAL         0x8002000F
	DISP_E_BADCALLEE                0x80020010
	DISP_E_NOTACOLLECTION           0x80020011
	DISP_E_DIVBYZERO                0x80020012
 */


#include "mem.h"

struct NoGCBase
{
public:
    //
    // COM objects are uncollectable -- this prevents the GC from finalizing
    // them if it so happens that the only references to them are pointers
    // that exist somewhere we can't see, like inside some COM subsystem
    // table the OS keeps or somewhere in the process' data area that we
    // don't scan with the GC (IOW, any memory we didn't allocate). The
    // GC can still collect pointers to collectable objects that were in
    // uncollectable objects. We call delete on ourselves when the COM ref
    // count goes to zero, as most COM implementations do.
    //
    inline void* operator new(size_t size)
    {
        return mem.malloc_uncollectable(size);
    }

    inline void operator delete(void* pv)
    {
        mem.free_uncollectable(pv);
    }
    inline void* operator new[](size_t size)
    {
        return operator new(size);
    }

    inline void operator delete[](void* pv)
    {
        operator delete(pv);
    }
}; // NoGCBase


#endif // __OLECOMMON_H__

