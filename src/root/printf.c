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


/* This file implements printf for GUI apps that sends the output
 * to logfile rather than stdout.
 * The file is closed after each printf, so that it is complete even
 * if the program subsequently crashes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "wchar.h"
#include <assert.h>
#if defined _WIN32
#include <windows.h>
#endif

#include "printf.h"

#if defined(VBSCRIPT)
#define LOG 1		// disable all print by setting this to 0
char logfile[] = "/tmp/bscript.log";
#else
#define LOG 1		// disable logging by setting this to 0
#if linux
char logfile[] = "/var/log/dscript.log";
#else
char logfile[] = "\\dscript.log";
#endif
#endif

/*********************************************
 * Route printf() to a log file rather than stdio.
 */

enum Plog
{
	TOBITBUCKET,
	TOLOGFILE,
	TOSTDOUT,
};

static Plog printf_logging = TOBITBUCKET;

void printf_tologfile()
{
    printf_logging = TOLOGFILE;
}

void printf_tostdout()
{
    printf_logging = TOSTDOUT;
}

void printf_tobitbucket()
{
    printf_logging = TOBITBUCKET;
}

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

    fp = fopen(filename, "a");
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
#else
#error Only linux and _WIN32 are supported
#endif
}


void LogfileAppend(char *buffer)
{
    if (printf_logging == TOLOGFILE)
	File_append(logfile, (unsigned char *)buffer, strlen(buffer));
    else
    {
	fputs(buffer, stdout);
	fflush(stdout);
    }
}

void LogfileAppend(wchar_t *buffer)
{
    // Convert unicode to ascii
    size_t size;
    size_t len;
    char *buffera;

    size = wcstombs(NULL, buffer, 0);	// compute required size
    if (size != (size_t)-1)
    {	// Valid conversion
	buffera = (char*)alloca(size + 1);
	len = wcstombs(buffera, buffer, size + 1);	// convert to ASCII
	//printf("len = %d, size = %d, wcslen = %d\n", len, size, wcslen(buffer));
	assert(len == size);		// loopback test
    }
    else
    {	// Invalid conversion. Do it ourselves
	size = wcslen(buffer);
	buffera = (char *)alloca(size + 1);
	for (size_t i = 0; i <= size; i++)
	    buffera[i] = (char)buffer[i];
	len = size;
    }


    if (printf_logging == TOLOGFILE)
	File_append(logfile, (unsigned char *)buffera, len);
    else
    {
	fputs(buffera, stdout);
	fflush(stdout);
    }
}

extern "C"
{

int VPRINTF(const char *format, va_list args)
{
#if LOG
    if (printf_logging != TOBITBUCKET)
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
	    else if (count >= (int)psize)
		psize = count + 1;
	    else
		break;
#elif defined _WIN32
	    count = _vsnprintf(p,psize,format,args);
	    if (count != -1)
		break;
	    psize *= 2;
#else
#error Only linux and _WIN32 are supported
#endif
	    p = (char *) alloca(psize * sizeof(buffer[0]));	// buffer too small, try again with larger size
	}

	LogfileAppend(p);
    }
#endif
    return 0;
}

int PRINTF(const char *format, ...)
{
#if LOG
    int result = 0;

    if (printf_logging != TOBITBUCKET)
    {

	va_list ap;
	va_start(ap, format);
	result = VPRINTF(format,ap);
	va_end(ap);
    }
    return result;
#else
    return 0;
#endif
}

int SPRINTF(char *buffer, int buffer_len, const char *format, ...)
{
    int result;

    va_list ap;
    va_start(ap, format);
    result = VSPRINTF(buffer, buffer_len, format, ap);
    va_end(ap);

    return result;
}

int  VSPRINTF(char *buffer, int buffer_len, const char *format, va_list args)
{
#if defined linux
    return vsnprintf(buffer, buffer_len, format, args);
#elif _WIN32
    return _vsnprintf(buffer, buffer_len, format, args);
#else
#error Only linux and _WIN32 are supported
#endif
}

#if defined UNICODE
int VWPRINTF(const wchar_t *format, va_list args)
{
#if LOG
    if (printf_logging != TOBITBUCKET)
    {
	wchar_t buffer[128];
	wchar_t *p;
	unsigned psize;
	int count;

	p = buffer;
	psize = sizeof(buffer) / sizeof(buffer[0]);
	for (;;)
	{
#if defined linux && !defined CHILICOM
	    count = vswprintf(p,psize,format,args);
	    if (count == -1)
		psize *= 2;
	    else if (count >= psize)
		psize = count + 1;
	    else
		break;
#else
	    count = _vsnwprintf(p,psize,format,args);
	    if (count != -1)
		break;
	    psize *= 2;
#endif
	    p = (wchar_t *) alloca(psize * sizeof(buffer[0]));	// buffer too small, try again with larger size
	}

	LogfileAppend(p);
    }
#endif
    return 0;
}

int WPRINTF(const wchar_t *format, ...)
{
#if LOG
    int result = 0;

    if (printf_logging != TOBITBUCKET)
    {
	va_list ap;
	va_start(ap, format);
	result = VWPRINTF(format,ap);
	va_end(ap);
    }
    return result;
#else
    return 0;
#endif
}

int SWPRINTF(wchar_t *buffer, int buffer_len, const wchar_t *format, ...)
{   int result;

    va_list ap;
    va_start(ap, format);
    result = VSWPRINTF(buffer, buffer_len, format, ap);
    va_end(ap);
    return result;
}

int VSWPRINTF(wchar_t *buffer, int buffer_len, const wchar_t *format, va_list args)
{
#if defined linux && !defined CHILICOM
    return vswprintf(buffer, buffer_len, (wchar_t*)format, args);
#elif defined _WIN32
    return _vsnwprintf(buffer, buffer_len, format, args);
#else
#error Only linux and _WIN32 are supported
#endif
}
#endif

}

void _printf_assert(const char *file, unsigned line)
{
    PRINTF("assert fail: %s(%d)\n", file, line);
    *(unsigned char *)0 = 0;	// seg fault to ensure it isn't overlooked
    exit(0);
}


