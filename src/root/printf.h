/* Digital Mars DMDScript source code.
 * Copyright (c) 2000-2001 by Chromium Communications
 * Copyright (c) 2004-2007 by Digital Mars
 * All Rights Reserved, written by Walter Bright
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

// Implement our own versions of printf to avoid problems with
// the vagaries of various compiler libraries.

#include <stdarg.h>

#ifndef _PRINTF_H_
#define _PRINTF_H_

#ifdef __cplusplus
extern "C" {
#endif

   int    PRINTF(const char *format, ...);
   int   VPRINTF(const char *format, va_list args);
   int   SPRINTF(char *buffer, int buffer_len, const char *format, ...);
   int  VSPRINTF(char *buffer, int buffer_len, const char *format, va_list args);

   int   WPRINTF(const wchar_t *format, ...);
   int  VWPRINTF(const wchar_t *format, va_list args);
   int  SWPRINTF(wchar_t *buffer, int buffer_len, const wchar_t *format, ...);
   int VSWPRINTF(wchar_t *buffer, int buffer_len, const wchar_t *format, va_list args);

#ifdef __cplusplus
}
#endif

void printf_tologfile();
void printf_tostdout();
void printf_tobitbucket();

// Do our own assert().
// Do different versions for different compilers to eliminate spamming warnings.

#undef assert
#if _MSC_VER
#define assert(e)	((e) || (_printf_assert(__FILE__, __LINE__),0))
#else
#define assert(e)	if (!(e)) _printf_assert(__FILE__, __LINE__)
#endif
void _printf_assert(const char *file, unsigned line);


// Primitive function entry/exit logging system

struct FuncLog
{
    const wchar_t *msg;

#if _MSC_VER
// Disable useless warnings about unreferenced inlines
#pragma warning (disable : 4514)
#endif

    FuncLog(const wchar_t *m)
    {
	msg = m;
	WPRINTF(L"+ %s\n", msg);
    }

    ~FuncLog()
    {
	WPRINTF(L"- %s\n", msg);
    }
};

#endif

