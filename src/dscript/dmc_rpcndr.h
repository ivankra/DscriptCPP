// Copyright (C) 2000-2001 by Chromium Communications
// All Rights Reserved

// There are some things missing from the DMC version of rpcndr.h

#if defined __DMC__

// This version of the rpcproxy.h file corresponds to MIDL version 3.3.106
// used with NT5 beta env from build #1574 on.

#ifndef __RPCNDR_H_VERSION__
#define __RPCNDR_H_VERSION__        ( 450 )
#endif // __RPCNDR_H_VERSION__


/****************************************************************************
 * Special things for VC5 Com support
 ****************************************************************************/

#if _MSC_VER >= 1100
#define DECLSPEC_UUID(x)    __declspec(uuid(x))
#define MIDL_INTERFACE(x)   struct __declspec(uuid(x)) __declspec(novtable)
#else
#define DECLSPEC_UUID(x)
#define MIDL_INTERFACE(x)   struct
#endif

#if _MSC_VER >= 1100
#define EXTERN_GUID(itf,l1,s1,s2,c1,c2,c3,c4,c5,c6,c7,c8)  \
  EXTERN_C const IID __declspec(selectany) itf = {l1,s1,s2,{c1,c2,c3,c4,c5,c6,c7,c8}}
#else
#define EXTERN_GUID(itf,l1,s1,s2,c1,c2,c3,c4,c5,c6,c7,c8) EXTERN_C const IID itf
#endif

#endif
