/* Digital Mars DMDScript source code.
 * Copyright (c) 2000-2001 by Chromium Communications
 * Copyright (c) 2004-2007 by Digital Mars
 * All Rights Reserved.
 * written by Walter Bright
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


/* This file implements printf for GUI apps that sends the output
 * to logfile rather than stdout.
 * The file is closed after each printf, so that it is complete even
 * if the program subsequently crashes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <windows.h>
#if defined linux
#include <string.h>
#include <wchar.h>
#endif

#define LOG 0		// disable logging by setting this to 0

#if defined linux
char logfile[] = "dscript.log";
#elif _WIN32
char logfile[] = "c:\\dscript.log";
#else
char logfile[] = "dscript.log";
#endif

/*********************************************
 * Append to a file.
 * Returns:
 *	0	success
 */

int File_append(char *filename, unsigned char *buffer, unsigned length)
{
#if defined linux
    FILE *fp;
    unsigned i;

    fp = fopen(logfile, "a");
    if (!fp)
	goto err;

    for (i = 0; i < length; i++)
	fprintf(fp, "%c", buffer[i]);
    fclose(fp);
    return 0;

err:
    return 1;
#elif defined _WIN32
    HANDLE h;
    DWORD numwritten;

    h = CreateFileA(filename,GENERIC_WRITE,0,NULL,OPEN_ALWAYS,
	FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,NULL);
    if (h == INVALID_HANDLE_VALUE)
	goto err;

#if 1
    SetFilePointer(h, 0, NULL, FILE_END);
#else // INVALID_SET_FILE_POINTER doesn't seem to have a definition
    if (SetFilePointer(h, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER)
	goto err;
#endif

    if (WriteFile(h,buffer,length,&numwritten,NULL) != TRUE)
	goto err2;

    if (length != numwritten)
	goto err2;

    if (!CloseHandle(h))
	goto err;
    return 0;

err2:
    CloseHandle(h);
err:
    return 1;
#endif // _WIN32
}


void LogfileAppend(char *buffer)
{
    File_append(logfile, (unsigned char *)buffer, strlen(buffer));
}

void LogfileAppend(wchar_t *buffer)
{
    // Convert unicode to ascii
    unsigned i;
    unsigned wlen;
    unsigned char *buffera;

    wlen = wcslen(buffer);
    buffera = (unsigned char *)alloca(wlen);
    for (i = 0; i < wlen; i++)
    {
	buffera[i] = (unsigned char) buffer[i];		// convert Unicode to ASCII
    }
    File_append(logfile, buffera, wlen);
}

extern "C"
{

#undef PRINTF
#undef VPRINTF
#undef WPRINTF
#undef VWPRINTF

int __cdecl VPRINTF(const char *format, va_list args)
{
#if LOG
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
#endif // _WIN32
	p = (char *) alloca(psize * sizeof(buffer[0]));	// buffer too small, try again with larger size
    }

    LogfileAppend(p);
#endif
    return 0;
}

int __cdecl PRINTF(const char *format, ...)
{
#if LOG
    int result;

    va_list ap;
    va_start(ap, format);
    result = VPRINTF(format,ap);
    va_end(ap);
    return result;
#else
    return 0;
#endif
}

int __cdecl VWPRINTF(const wchar_t *format, va_list args)
{
#if LOG
    wchar_t buffer[128];
    wchar_t *p;
    unsigned psize;
    int count;

    p = buffer;
    psize = sizeof(buffer) / sizeof(buffer[0]);
    for (;;)
    {
#if defined linux
	count = vswprintf(p,psize,format,args);
	if (count == -1)
	    psize *= 2;
	else if (count >= psize)
	    psize = count + 1;
	else
	    break;
#elif defined _WIN32
	count = _vsnwprintf(p,psize,format,args);
	if (count != -1)
	    break;
	psize *= 2;
#endif
	p = (wchar_t *) alloca(psize * sizeof(buffer[0]));	// buffer too small, try again with larger size
    }

    LogfileAppend(p);
#endif
    return 0;
}

int __cdecl WPRINTF(const wchar_t *format, ...)
{
#if LOG
    int result;

    va_list ap;
    va_start(ap, format);
    result = VWPRINTF(format,ap);
    va_end(ap);
    return result;
#else
    return 0;
#endif
}

}

