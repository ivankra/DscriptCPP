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

//<DOC>
// dshell.cpp
// ----------
// SYNOPSIS
//      dshell.exe [OPTIONS]
//
// DESCRIPTION
//      dshell is an executable wrapper around ds.lib which parses and executes
//      javascript program text.  The following options can be used:
//
//      -f filename
//              File to be parsed and executed.  Can be used multiple times to
//              include as many files as necessary.  Files will be parsed in the
//              order which they are specified on the commandline.
//
//      -i
//              Specifies interactive mode.  In interactive mode the user enters
//              single line ECMAScript programs.  This option can be combined with
//              the -f option to preload functions which can then be called by the
//              user.  Note that variables and commands do not carry over from one
//              single line program to the next.
//</DOC>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <locale.h>

#include "dscript.h"
#include "dshell.h"
#include "program.h"
#include "lexer.h"
#include "mem.h"
#include "dchar.h"

//
// Fake definitions required to stub out COM stuff
//
struct IUnknown;
struct Dcomobject;
typedef struct tagVARIANT
{
} VARIANTARG, VARIANT;
typedef long HRESULT;


//<DOC>
// exception - exception function required by ds.lib
//
// Inputs:
//      msg - the exception called
//      ... - other relevant data to the exception
// Outputs:
//      n/a
//</DOC>
void exception( dchar *msg, ... )
{
#if defined UNICODE
    wprintf(L"Exception: %ls\n", msg);
#else
    printf("Exception: %s\n", msg);
#endif
    exit( EXITCODE_RUNTIME_ERROR );
} // exception


//<DOC>
// RuntimeErrorx - function handler for runtime errors.  Required by ds.lib. 
//
// Inputs:
//      format - printf style error string describing error
//      ... - relevant arguments needed by the format string
// Outputs:
//      n/a
//</DOC>
void RuntimeErrorx( char *format, ... )
{
    va_list ap;

    printf("\n");
    fflush(stdout);
    va_start(ap, format);
    printf("DMDScript fatal runtime error: ");
#if defined UNICODE
    wchar_t *format2 = ascii2unicode(format);
    vwprintf(format2, ap);
#else
    vprintf(format, ap);
#endif
    va_end( ap );
    printf("\n");
    fflush(stdout);

    exit( EXITCODE_RUNTIME_ERROR );
}  // RuntimeErrorx


//<DOC>
// Dcomobject_init - COM handler required by ds.lib.
//
// Inputs:
//      n/a
// Outputs:
//      n/a
//</DOC>
void Dcomobject_init()
{
}  // Dcomobject_init


//<DOC>
// Dcomobject_addCOM - COM handler required by ds.lib.
//
// Inputs:
//      n/a
// Outputs:
//      n/a
//</DOC>
void Dcomobject_addCOM(Dobject *)
{
}  // Dcomobject_addCOM


//<DOC>
// dcomobject_call - COM handler required by ds.lib.
//
// Inputs:
//      s - ignored
// Outputs:
//      n/a
//</DOC>
void *
dcomobject_call(d_string s, CALL_ARGS)
{
    UNREFERENCED_PARAMETER( s );
    assert(0);
    return NULL;
}  // dcomobject_call


//<DOC>
// dcomobject_isNull - COM handler required by ds.lib.
//
// Inputs:
//      s - ignored
// Outputs:
//      n/a
//</DOC>
d_boolean dcomobject_isNull(Dcomobject* pObj)
{
    UNREFERENCED_PARAMETER(pObj);
    assert(0);
    return FALSE;
}  // dcomobject_isNull

d_boolean dcomobject_areEqual(Dcomobject* pLeft, Dcomobject* pRight)
{
    UNREFERENCED_PARAMETER(pLeft);
    UNREFERENCED_PARAMETER(pRight);
    assert(0);
    return FALSE;
}

void ValueToVariant(VARIANTARG *variant, Value *value)
{
}

HRESULT VariantToValue(Value *ret, VARIANTARG *variant, d_string PropertyName, Dcomobject *pParent)
{
    assert(0);
    return 0;
}



//<DOC>
// CreateUnicode - reads in character text and determines if it is Unicode.  If not,
//          the function converts the ASCII string to Unicode.  If so,
//          converts the text to wide characters.  Note that this function allocs 
//          memory for returned string.  It is the caller's responsibility to free this
//          memory.
//
// Inputs:
//      szText - buffer of text to be checked
//      iSize - size of the text buffer
//      iWSize - pointer to a integer that holds the strings new size
// Outputs:
//      returns a double character string if the conversion completes without
//      error.  Returns NULL, otherwise.
//</DOC>
wchar_t *
CreateUnicode( const char *szText, int iSize, int *iWSize )
{
    const unsigned short BOM = 0xFEFF;  // Byte Order Marker for Unicode
    wchar_t *szWideText = NULL;

    assert( iWSize != NULL );

    if( NULL == szText || 0 == iSize )
    {
        *iWSize = 0;
        return NULL;
    }

    // Check for the BOM to see if this is already Unicode
    if( iSize > 1 && *((unsigned short *) szText) == BOM )
    {
        if( iSize & 1 )  // Ensure number of bytes is even
        {
            return NULL;
        }
        *iWSize = ( (iSize / sizeof( szWideText[0] )) - 1);
        // Strip off BOM when assigning to szWideText
        szWideText = (wchar_t *) mem.malloc( *iWSize * sizeof( szWideText[0] ) );
        assert( szWideText != NULL );
        memcpy( szWideText, szText + 2, iSize - 2 );
    }
    else
    {
        // Convert ASCII to Unicode -- we'll need an extra character for the end sentinel
        szWideText = (wchar_t *) mem.malloc( iSize * sizeof( szWideText[0] ) );
        assert( szWideText != NULL );
        for( int i = 0; i < iSize; i++ )
        {
            szWideText[i] = (wchar_t) (szText[i] & 0xFF);
        }
        *iWSize = iSize;
        szWideText[*iWSize] = 0x001A;   // End Sentinel
        (*iWSize)++;
    }
    return szWideText;
}  // CreateUnicode


//<DOC>
// main - entry point for dshell executable
//</DOC>
int main( int argc, char *argv[] )
{
    const char *PROMPT = "dshell>";
    char szBuffer[1024] = "";
    unsigned int iTextSize = 1024;
    char *szProgramText = NULL;
    unsigned int iPos = 0;
    bool bInteractive = false;
    int nRetVal = 0;
    int result = 0;
    Program prog( NULL, NULL );
    ErrInfo errinfo;
    int i;

#if defined UNICODE
    dchar *szWideText = NULL;
#endif

#if defined UNICODE
    _wsetlocale(LC_ALL, L"");
#else
    setlocale(LC_ALL, "");
#endif

    szProgramText = (char *) mem.malloc( iTextSize * sizeof( szProgramText[0] ) );
    if( NULL == szProgramText )
    {
        nRetVal = EXITCODE_INIT_ERROR;
        goto CLEAN_EXIT;
    }

    for( i = 1; i < argc; i++ )
    {
        if( strncmp( argv[i], "-f", 2 ) == 0 )
        {
            if( (i + 1) == argc )
            {
                nRetVal = EXITCODE_INVALID_ARGS;
                goto CLEAN_EXIT;
            }

            FILE *pFile = fopen( argv[++i], "r" );
            if( NULL == pFile )
            {
                nRetVal = EXITCODE_FILE_NOT_FOUND;
                goto CLEAN_EXIT;
            }
            while( !feof( pFile ) )
            {
                szBuffer[0] = '\0';
                size_t rd = fread( szBuffer, sizeof( szBuffer[0] ), sizeof( szBuffer ), pFile ); 
                if( ferror( pFile ) )
                {
                    nRetVal = EXITCODE_INIT_ERROR;
                    fclose( pFile );
                    goto CLEAN_EXIT;
                }
                while( (rd + iPos) >= iTextSize )
                {
                    iTextSize *= 2;
                    szProgramText = (char *) mem.realloc( szProgramText, iTextSize * sizeof( szProgramText[0] ) );
                    if( NULL == szProgramText )
                    {
                        nRetVal = EXITCODE_INIT_ERROR;
                        fclose( pFile );
                        goto CLEAN_EXIT;
                    }
                }
                memcpy( (szProgramText + (iPos * sizeof( szProgramText[0] ))), szBuffer, rd );
                iPos += rd;
            }
            fclose( pFile );
        }
        else if( strncmp( argv[i], "-i", 2 ) == 0 )
        {
            bInteractive = true;
        }
        else
        {
            nRetVal = EXITCODE_INVALID_ARGS;
            goto CLEAN_EXIT;
        }
    }
    // Add space for null character
    if( (iPos + 1) > iTextSize )
    {
        iTextSize = iPos + 1;
        szProgramText = (char *) mem.realloc( szProgramText, iTextSize * sizeof( szProgramText[0] ) );
        if( NULL == szProgramText )
        {
            nRetVal = EXITCODE_INIT_ERROR;
            goto CLEAN_EXIT;
        }
    }
    iPos++;
    szProgramText[iPos - 1] = '\0';

    Lexer::initKeywords();
    do
    {
        char szTextLine[2048] = "";
        unsigned int iLineSize = 0;
        unsigned int iSize = 0;
        if( szProgramText != NULL )
        {
            szProgramText[iPos - 1] = '\0';
            iSize = iPos;
        } 
        if( bInteractive )  
        {
            printf( PROMPT );
            fgets( szTextLine, sizeof( szTextLine ), stdin );
            if( strnicmp( szTextLine, "quit\n", 5 ) == 0 )
            {
                bInteractive = false;
                continue;
            }
            iLineSize = strlen( szTextLine ); 
            iSize = iLineSize + iPos;
            if( iSize > iTextSize )
            {
                iTextSize = iSize;
                szProgramText = (char *) mem.realloc( szProgramText, iTextSize * sizeof( szProgramText[0] ) );
                if( NULL == szProgramText )
                {
                    nRetVal = EXITCODE_INIT_ERROR;
                    goto CLEAN_EXIT;
                }
            }
            int offset = (iPos != 0) ? (iPos - 1) : 0;
            memcpy( szProgramText + ((offset) * sizeof( szProgramText[0] )), szTextLine, iLineSize ); 
            szProgramText[iSize - 1] = '\0';
        }
#if defined UNICODE
        int iWSize = 0;
        dchar *szWideText = CreateUnicode( szProgramText, iSize, &iWSize );
        result = prog.parse_common( "dshell test", szWideText, iWSize, NULL, &errinfo );
#else
        result = prog.parse_common( "dshell test", (dchar *) szProgramText, iSize, NULL, &errinfo );
#endif
        if( result )
        {
            FILE *hOutput = bInteractive ? stdout : stderr;
#if defined UNICODE
            fwprintf( hOutput, L"%ls\n", errinfo.message );
#else
            fprintf( hOutput, "%s\n", errinfo.message );
#endif
            fflush( hOutput );
            if( !bInteractive )
            {
                nRetVal = EXITCODE_RUNTIME_ERROR;
                goto CLEAN_EXIT;
            }
        }
        errinfo.message = NULL;  // Garbage collection handles deallocation
        result = prog.execute( 0, NULL, &errinfo );
        if( bInteractive && result )
        {
#if defined UNICODE
            wprintf( L"%ls\n", errinfo.message );
#else
            printf( "%s\n", errinfo.message );
#endif
            errinfo.message = NULL;  // Garbage collection handles deallocation
        }
    }  while( bInteractive );


    nRetVal = 0;
    if( result )
    {
        nRetVal = EXITCODE_RUNTIME_ERROR;
    }

CLEAN_EXIT:
    errinfo.message = NULL;  // Garbage collection handles deallocation
#if defined UNICODE
    if( szWideText != NULL )
    {
        mem.free( szWideText );
        szWideText = NULL;
    }
#endif
    if( szProgramText != NULL )
    {
        mem.free( szProgramText );
        szProgramText = NULL;
    }

    mem.fullcollect();   // Required for garbage collector

    return nRetVal;
    // No code beyond this point is reached.
}  // main



extern "C"
{

int __cdecl VPRINTF(const char *format, va_list args)     { return vprintf(format, args); }

int __cdecl PRINTF(const char *format, ...)
{   int result;

    va_list ap;
    va_start(ap, format);
    result = vprintf(format,ap);
    va_end(ap);
    return result;
}

int __cdecl VWPRINTF(const wchar_t *format, va_list args)   { return vwprintf(format, args); }

int __cdecl WPRINTF(const wchar_t *format, ...)
{   int result;

    va_list ap;
    va_start(ap, format);
    result = vwprintf(format,ap);
    va_end(ap);
    return result;
}

};
