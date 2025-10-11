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

#include "windows.h"
#include "wtypes.h"
#include "oleauto.h"
#include <math.h>
#include <assert.h>
#include "varFormats.h"
#ifdef linux
#include <string.h>
#else
#include <strings.h>
#endif
//#include "VarConvert.h"
#if 0
#include "stdafx.h"
#include "VarConvert.h"
#include "resource.h"       // main symbols
#include <asptlb.h>         // Active Server Pages Definitions
#include "varFormats.h"
#endif

// for debug only:
//#define NT_DEBUG_ONLY
#ifdef NT_DEBUG_ONLY
#include "Convert.h"

// and this is for finding *which* argument error we actually got:
#define BADARG(x) ( 0x8007CC00 + x )

#else // if not debugging
#define BADARG(x) E_INVALIDARG

#endif

// To make this code work identical to VBScript, leave the
// following define alone!
#define COMPENSATE_FOR_LOCALE_INFO

// Support routines!
//
BSTR toBSTR( int codepage, char * lp, int nLen)
{
    //USES_CONVERSION;
    BSTR str = NULL;
    int nConvertedLen = MultiByteToWideChar( codepage, 0, lp, nLen, NULL, 0) - 1;
    str = ::SysAllocStringLen(NULL, nConvertedLen);
    if (str != NULL)
    {
	MultiByteToWideChar(codepage, 0, lp, -1, str, nConvertedLen);
    }
    return str;
}

// *************************************************************************
// Get numeric locale info...
// Since system locale info is always returned as an ASCII string,
// we have a convenience routine to, instead, get numeric info as an integer
//
int getNumericInfo( LCID lcid, int which )
{
    char buff[ 16 ];
    if ( GetLocaleInfoA( lcid, which, buff, 15 ) == 0 )
    {
	return -1; // ??? what better???
    }
    else
    {
	return atoi( buff );
    }
}

// *************************************************************************
// Get single character locale info...
// Since system locale info is always returned as an ASCII string,
// we have a convenience routine to, instead, get single character info
//
char getCharacterInfo( LCID lcid, int which )
{
    char buff[ 16 ];
    if ( GetLocaleInfoA( lcid, which, buff, 15 ) == 0 )
    {
	return 0; // ??? what better???
    }
    else
    {
	return buff[0];
    }
}

// *************************************************************************
int
getCPfromLCID( LCID lcid )
{
    char buff[ 16 ];
    if ( GetLocaleInfoA( lcid, LOCALE_IDEFAULTANSICODEPAGE, buff, 15 ) == 0 )
    {
	return 1252; // ??? what better???
    }
    else
    {
	return atoi( buff );
    }
}

// *************************************************************************
// For testing purposes: Generic getLocaleInfo routine...
//
HRESULT
_VAR_GetLocaleInfo( LCID lcid, int which, BSTR *pbstrOut )
{
    // for all these routines, we need the codepage first:
    int codepage = getCPfromLCID( lcid );

    // and get the multi-byte version of the answer
    char buff[ 64 ];
    int len = GetLocaleInfoA(lcid, which, buff, sizeof(buff)-1 );

    if ( len <= 0 )
    {
	strcpy( buff, "** INVALID ARGUMENT **");
    }

    // convert that to widechar:
    BSTR bstr = toBSTR( codepage, buff, len );
    if ( bstr == NULL ) return E_OUTOFMEMORY;

    *pbstrOut = bstr;
    return S_OK;
}

// *************************************************************************
// Get the name of the weekday that corresponds to the iWeekday argument, 
// assuming that the iFirstDay argument specifies the first day of the week.
//
// 1=Monday, etc., throughout.
//
// fAbbrev true (non zero) means get abbreviated version of the name.
//
HRESULT
_VAR_WeekdayName( LCID lcid, int iWeekday, int fAbbrev, int iFirstDay, ULONG dwFlags, BSTR *pbstrOut )
{
    // validate the arguments first:
    if ( iWeekday < 1 || iWeekday > 7 || iFirstDay < 1 || iFirstDay > 7 )
    {
	return E_INVALIDARG;
    }

    // for all these routines, we need the codepage first:
    int codepage = getCPfromLCID( lcid );

    // dwFlags are ignored at this time!

    // GetLocaleInfoA puts the days of the week in order, from
    // LOCALE_SDAYNAME1 to LOCALE_SDAYNAME7, with Monday as day 1.
    // The abbreviations are similarly given via LOCALE_SABBREVDAYNAMEx
    // And we use either the full or abbreviated name, according to flag
    int selection = ( fAbbrev != 0 ) ? LOCALE_SABBREVDAYNAME1 : LOCALE_SDAYNAME1;

    // so to get the offset from the first dayname in the LocaleInfo
    // table we just play a simple little math game:
    int offset = ( iWeekday + iFirstDay + 4 ) % 7;

    // and then we adjust the selection:
    selection += offset;

    // and get the multi-byte version of the answer
    char buff[ 64 ];
    int len = GetLocaleInfoA(lcid, selection, buff, sizeof(buff)-1 );

    if ( len <= 0 ) return E_INVALIDARG;

    // convert that to widechar:
    BSTR bstr = toBSTR( codepage, buff, len );
    if ( bstr == NULL ) return E_OUTOFMEMORY;

    *pbstrOut = bstr;
    return S_OK;
}

// *************************************************************************
// Get the name of the month that corresponds to the iMonth argument,
//
// 1=January, etc., throughout.
//
// fAbbrev true (non zero) means get abbreviated version of the name.
//
HRESULT
_VAR_MonthName( LCID lcid, int iMonth, int fAbbrev, ULONG dwFlags, BSTR *pbstrOut )
{
    // validate the arguments first:
    if ( iMonth < 1 || iMonth > 12 )
    {
	return E_INVALIDARG;
    }

    // for all these routines, we need the codepage first:
    int codepage = getCPfromLCID( lcid );


    // dwFlags are ignored at this time!

    // GetLocaleInfoA puts the months in order, from
    // LOCALE_SMONTHNAME1 to LOCALE_SMONTHNAME12, with January as month 1.
    // The abbreviations are similarly given via LOCALE_SABBREVMONTHNAMEx
    // And we use either the full or abbreviated name, according to flag
    int selection = ( fAbbrev != 0 ) ? LOCALE_SABBREVMONTHNAME1 : LOCALE_SMONTHNAME1;

    // and then we adjust the selection by requested month:
    selection += (  iMonth - 1 );

    // and get the multi-byte version of the answer
    char buff[ 64 ];
    int len = GetLocaleInfoA(lcid, selection, buff, sizeof(buff)-1 );

    if ( len <= 0 ) return E_INVALIDARG;

    // convert that to widechar:
    BSTR bstr = toBSTR( codepage, buff, len );
    if ( bstr == NULL ) return E_OUTOFMEMORY;

    *pbstrOut = bstr;
    return S_OK;
}

// *************************************************************************
// This routine is the base get-formatted-number routine.
// It is called from the user-invoked format routines and only:
// (1) Rounds the number to the specified number of digits after the dec. pt.
// (2) Puts the decimal point in place, if called for.
// (3) Puts the thousands separators in place, if called for.
//
// This routine leaves the number in ASCII form; the buffer must be passed in
//

HRESULT
_doFormatNumber(
	LCID lcid,
	double num,
	int iNumDig,
	int iIncLead,
	int iGroup,
	char decpt,
	char thousands,
	char *strOut )
{
    int digit;

    // zbuf is a string of 200 zeroes
    char zbuf[201];
    for ( int zx = 0; zx < 200; ++zx ) zbuf[zx] = '0';
    zbuf[200] = '\0';

    // get defaults for this locale if not given
    if ( iIncLead < -1 ) iIncLead = getNumericInfo( lcid, LOCALE_ILZERO );

#ifdef NT_DEBUG_ONLY
    CConvert::infoitem[0] = iNumDig;
    CConvert::infoitem[1] = iIncLead;
    CConvert::infoitem[2] = iGroup;
    CConvert::infoitem[3] = decpt;
    CConvert::infoitem[4] = thousands;
#endif

    // "scale" the number until it is in the range 1.0 <= num < 10
    int scale = 0;
    if ( num != 0 )
    {
	while ( num >= 10.0 )
	{
	    ++scale;
	    num /= 10.0;
	}
	while ( num < 1.0 )
	{
	    --scale;
	    num *= 10.0;
	}
    }

#ifdef NT_DEBUG_ONLY
    CConvert::infoitem[5] = scale;
#endif

    if ( scale > 80 || scale < -80 )
    {
	return E_INVALIDARG; // sorry, but we won't handle more than that!
    }

    // now convert that to ASCII digits in the buffer...
    // decimal point is implied to be between char's 99 and 100
    int index = 99 - scale;

    for ( int cnt = 1; cnt <= 15; ++cnt )
    {
	digit = (int) num; // get one digit
	zbuf[ index++ ] = ( '0' + digit );
	num = ( num - (double)digit ) * 10.0; // shift left
    }

    // this is supposed to round to 15 digits...
    if ( num > 4 )
    {
	++zbuf[ --index ]; // round last digit up if needed...
	// but propagate a round up of xxxx99999 as far as needed...
	while ( zbuf[ index ] > '9' ) // we incremented past 9?
	{
	    zbuf[index] = '0'; // set it back to zero...
	    ++ zbuf[ --index ]; // and bump *next* digit!
	}
    }

    // now round to requisite number of places...
    int roundAt = 99 + iNumDig;

#ifdef NT_DEBUG_ONLY
    CConvert::infoitem[6] = roundAt;
#endif

    if ( zbuf[ roundAt+1 ] > '4' )
    {
	++zbuf[ roundAt ]; // round last digit up if needed...
	// but propagate a round up of xxxx99999 as far as needed...
	while ( zbuf[ roundAt ] > '9' ) // we incremented past 9?
	{
	    zbuf[roundAt] = '0'; // set it back to zero...
	    ++ zbuf[ --roundAt ]; // and bumt *next* digit!
	}
    }
    // now, where does that number start in the buffer?
    int startat;
    for ( startat = 0; startat < 100; ++startat )
    {
	if ( zbuf[ startat ] != '0' ) break;
    }
    // any digits left of decimal point?
    if ( startat > 99 )
    {
	// nope...do we supply a leading zero?
	startat = ( iIncLead != 0 ) ? 99 : 100;
    }
    // and *how many* digits left of decimal point?
    int left = 100 - startat;

#ifdef NT_DEBUG_ONLY
    CConvert::infoitem[7] = left;
#endif

    // ready to copy the number to the output buffer
    int frompos = 0;
    for ( frompos = startat; frompos <= 99; ++frompos )
    {
	*strOut++ = zbuf[ frompos ]; // copy one character
	--left; // one less digit
	if (( iGroup != 0 ) && ( ( left % 3 ) == 0 )
		&& ( (thousands & 0x7F) >= 0x20 ) && ( left != 0 ))
	{
		// need a separator
		*strOut++ = thousands; // place holder!
	}
    }

    // end of digits left of decimal point!
    if ( iNumDig > 0 )
    {
	*strOut++ = decpt; // place holder
	while ( iNumDig > 0 )
	{
	    *strOut++ = zbuf[ frompos++ ];
	    --iNumDig;
	}
    }
    // terminate with a null!
    *strOut = 0;

    // is that all???
    return S_OK;
}

// *******************************************************************
//
HRESULT
_VAR_FormatNumber(
	LCID lcid,
	LPVARIANT pvarIn,
	int iNumDig,
	int iIncLead,
	int iUseParens,
	int iGroup,
	ULONG dwFlags,
	BSTR *pbstrOut)
{
    VARIANTARG vtemp;
    LPVARIANT pv;
    int neg = 0;

    if ( pvarIn->vt == VT_R8 )
    {
	pv = pvarIn;
    }
    else
    {
	pv = &vtemp;
	pv->vt = 0;
	if ( VariantChangeTypeEx( pv, pvarIn, lcid, 0, VT_R8 ) != S_OK )
	{
	    return E_INVALIDARG;
	}
    }

    // for all these routines, we need the codepage first:
    int codepage = getCPfromLCID( lcid );

    // ??? should use macro instead of .dblVal !!!
    double num = pv->dblVal;
    if ( num < 0 )
    {
	neg = 1;
	num = - num;
    }
    char ascBuff[256];
    char decpt = getCharacterInfo( lcid, LOCALE_SDECIMAL );
    char thous = getCharacterInfo( lcid, LOCALE_STHOUSAND );
    if ( iNumDig < 0 ) iNumDig = getNumericInfo( lcid, LOCALE_IDIGITS );
    if ( iGroup < -1 ) iGroup = getNumericInfo( lcid, LOCALE_SGROUPING );

    if ( S_OK != _doFormatNumber( lcid, num, iNumDig, iIncLead, iGroup, decpt,
		thous, ascBuff ))
    {
	return E_INVALIDARG;
    }
    // here's where we add the negative sign or condition...
    if ( neg != 0 )
    {
	if ( iUseParens == -1 )
	{
	    // put parens around number
	    char temp[256];
	    strcpy( temp, ascBuff );
	    ascBuff[0] = '(';
	    strcpy( ascBuff+1, temp );
	    strcat( ascBuff, ")" );
	}
	else
	{
	    int negmode = getNumericInfo( lcid, LOCALE_INEGNUMBER );
	    char temp[256];
	    switch ( negmode )
	    {
	    case 3:
		// minus sign *after* number
		strcpy( ascBuff+strlen(ascBuff), "-" );
		break;
	    default:
	    case 1:
		// minus sign before
		temp[0] = '-';
		strcpy( temp+1, ascBuff );
		strcpy( ascBuff, temp );
		break;
	    case 2:
		// minus sign then space before
		temp[0] = '-';
		temp[1] = ' ';
		strcpy( temp+2, ascBuff );
		strcpy( ascBuff, temp );
		break;
	    }

	}
    }
    // convert that to widechar:
    BSTR bstr = toBSTR( codepage, ascBuff, strlen(ascBuff)+1 );
    if ( bstr == NULL ) return E_OUTOFMEMORY;

    *pbstrOut = bstr;
    return S_OK;
}

// *******************************************************************
//
HRESULT
_VAR_FormatPercent(
	LCID lcid,
	LPVARIANT pvarIn,
	int iNumDig,
	int iIncLead,
	int iUseParens,
	int iGroup,
	ULONG dwFlags,
	BSTR *pbstrOut)
{
    VARIANTARG vtemp;
    LPVARIANT pv;
    int neg = 0;

    if ( pvarIn->vt == VT_R8 )
    {
	pv = pvarIn;
    }
    else
    {
	pv = &vtemp;
	pv->vt = 0;
	if ( VariantChangeTypeEx( pv, pvarIn, lcid, 0, VT_R8 ) != S_OK )
	{
	    return E_INVALIDARG;
	}
    }

    // for all these routines, we need the codepage first:
    int codepage = getCPfromLCID( lcid );

    // ??? should use macro instead of .dblVal !!!
    double num = 100.0 * pv->dblVal;
    if ( num < 0 )
    {
	neg = 1;
	num = - num;
    }
    char ascBuff[256];
    char decpt = getCharacterInfo( lcid, LOCALE_SDECIMAL );
    char thous = getCharacterInfo( lcid, LOCALE_STHOUSAND );
    if ( iNumDig < 0 ) iNumDig = getNumericInfo( lcid, LOCALE_IDIGITS );
    if ( iGroup < -1 ) iGroup = getNumericInfo( lcid, LOCALE_SGROUPING );

    if ( S_OK != _doFormatNumber( lcid, num, iNumDig, iIncLead, iGroup, decpt,
		thous, ascBuff ))
    {
	return E_INVALIDARG;
    }
    // here's where we add the negative sign or condition...
    if ( neg == 0 )
    {
	// positive number...just append a percent sign
	strcat( ascBuff, "%" );
    }
    else
    {
	// negative number...what format to use?
	if ( iUseParens == -1 )
	{
	    // put parens around number...percent sign goes INSIDE
	    char temp[256];
	    strcpy( temp, ascBuff );
	    ascBuff[0] = '(';
	    strcpy( ascBuff+1, temp );
	    strcat( ascBuff, "%)" );
	}
	else
	{
	    int negmode = getNumericInfo( lcid, LOCALE_INEGNUMBER );
	    char temp[256];
	    switch ( negmode )
	    {
	    case 3:
		// minus sign *after* number
		strcpy( ascBuff+strlen(ascBuff), "-" );
		break;
	    default:
	    case 1:
		// minus sign before
		temp[0] = '-';
		strcpy( temp+1, ascBuff );
		strcpy( ascBuff, temp );
		break;
	    case 2:
		// minus sign then space before
		temp[0] = '-';
		temp[1] = ' ';
		strcpy( temp+2, ascBuff );
		strcpy( ascBuff, temp );
		break;
	    }
	    // append a percent sign
	    strcat( ascBuff, "%" );
	}
    }

    // convert that to widechar:
    BSTR bstr = toBSTR( codepage, ascBuff, strlen(ascBuff)+1 );
    if ( bstr == NULL ) return E_OUTOFMEMORY;

    *pbstrOut = bstr;
    return S_OK;
}

HRESULT
_VAR_FormatCurrency(
	LCID lcid,
	LPVARIANT pvarIn,
	int iNumDig,
	int iIncLead,
	int iUseParens,
	int iGroup,
	ULONG dwFlags,
	BSTR *pbstrOut)
{
    VARIANTARG vtemp;
    LPVARIANT pv;
    int neg = 0;

    if ( pvarIn->vt == VT_R8 )
    {
	pv = pvarIn;
    }
    else
    {
	pv = &vtemp;
	pv->vt = 0;
	if ( VariantChangeTypeEx( pv, pvarIn, lcid, 0, VT_R8 ) != S_OK )
	{
	    return E_INVALIDARG;
	}
    }

    // for all these routines, we need the codepage first:
    int codepage = getCPfromLCID( lcid );

    // ??? should use macro instead of .dblVal !!!
    double num = pv->dblVal;
    if ( num < 0 )
    {
	neg = 1;
	num = - num;
    }
    char ascBuff[256];
    char decpt = getCharacterInfo( lcid, LOCALE_SMONDECIMALSEP );

//### HACK HACK HACK
    // there is a bug in the MS GetLocaleInfoA!  For Italian, it gives
    // a null as the decimal point character for money...but VBS knows
    // it should really be a comma, so...we use the non-money one, instead
    if ( decpt == 0 ) decpt = getCharacterInfo( lcid, LOCALE_SDECIMAL );
//### HACK HACK HACK

    char thous = getCharacterInfo( lcid, LOCALE_SMONTHOUSANDSEP );
    if ( iNumDig < 0 ) iNumDig = getNumericInfo( lcid, LOCALE_ICURRDIGITS );
    if ( iGroup < -1 ) iGroup = getNumericInfo( lcid, LOCALE_SMONGROUPING );

    if ( S_OK != _doFormatNumber( lcid, num, iNumDig, iIncLead, iGroup, decpt,
		thous, ascBuff ))
    {
	return E_INVALIDARG;
    }

    // we always need the currency string:
    char currency[ 32 ];
    if ( GetLocaleInfoA( lcid, LOCALE_SCURRENCY, currency, 31 ) == 0 )
    {
	return E_INVALIDARG;
    }

    char *p0, *p1, *p2, *p3, *p4; // the "pieces" of a currency output
    p0 = 0; // except for forced parens, p0 is unused
    p4 = 0; // and p4 usually zero, so why not?

    // Serbian has weird rules about negative currency...
    // If Serbian parens are forced, ignore them and use default
    if ( (iUseParens == -1) && ( lcid == 0x081A || lcid == 0x0C1A ) )
    {
	iUseParens = -2;
    }

    // process the format:
    if ( neg == 0 || iUseParens == -1 ) // -1 means *forced* parens with neg
    {
	// positive currency
	int pmode = getNumericInfo( lcid, LOCALE_ICURRENCY );
	char * negspace = 0;
	// IF the user forced us to use parens and this is a negative value,
	// then we need to check to see if we need the space separator
	// for negative values.  Yes, there really is a LOCALE INFO for this:
	if ( ( neg != 0 )
		    && ( getNumericInfo(lcid,LOCALE_INEGSEPBYSPACE) != 0 ))
	{
		negspace = " ";
	}
	switch( pmode )
	{
	case 0:
	default:
	    // this mode is "$999" -- no spaces in positive
	    p1 = currency;
	    p2 = (neg==0)? 0 : negspace; // might be a space if negative
	    p3 = ascBuff;
	    break;
	case 1:
	    // this mode is "num$" -- no spaces in positive
	    p1 = ascBuff;
	    p2 = (neg==0)? 0 : negspace; // might be a space if negative
	    p3 = currency;
	    break;
	case 2:
	    // this mode is "$ num" -- space after cur sym in positive
	    p1 = currency;
	    p2 = (char *)((neg==0)? " " : negspace); // might NOT be a space if negative
	    p3 = ascBuff;
	    break;
	case 3:
	    // this mode is "num $" -- space before cur sym
	    p1 = ascBuff;
	    p2 = (char *)((neg==0)? " " : negspace); // might NOT be a space if negative
	    p3 = currency;
	    break;
	} // end of switch on pmode

	if ( neg != 0 )
	{
	    // we are here only because of forced parens on negative
	    p0 = "(";
	    p4 = ")";
	}
    }
    else
    {
	// negative currency
	int nmode = getNumericInfo( lcid, LOCALE_INEGCURR );
	switch( nmode )
	{
	case 0:
	    // this mode is "($999)" unless parens are turned off,
	    // in which case it is -$999
	    if ( iUseParens != 0 )
	    {
		p1 = "(";
		p2 = currency;
		p3 = ascBuff;
		p4 = ")";
	    } else {
		p1 = "-";
		p2 = currency;
		p3 = ascBuff;
	    }
	    break;
	case 1:
	default:
	    // "-$999" no spaces
	    p1 = "-";
	    p2 = currency;
	    p3 = ascBuff;
	    break;
	case 2:
	    // "$-999" no spaces
	    p1 = currency;
	    p2 = "-";
	    p3 = ascBuff;
	    break;
	case 3:
	    // "$999-" no spaces
	    p1 = currency;
	    p2 = ascBuff;
	    p3 = "-";
	    break;
	case 4:
	    // this mode is "(999$)" unless parens are turned off,
	    // in which case it is -999$
	    if ( iUseParens != 0 )
	    {
		    p1 = "(";
		    p2 = ascBuff;
		    p3 = currency;
		    p4 = ")";
	    } else {
		    p1 = "-";
		    p2 = ascBuff;
		    p3 = currency;
	    }
	    break;
	case 5:
	    // "-999$" no spaces
	    p1 = "-";
	    p2 = ascBuff;
	    p3 = currency;
	    break;
	case 8:
	    // "-999 $" space before sym
	    p1 = "-";
	    p2 = ascBuff;
	    p3 = " ";
	    p4 = currency;
	    break;
	case 9:
	    // "-$ 999" space after sym
	    p1 = "-";
	    p2 = currency;
	    p3 = " ";
	    p4 = ascBuff;
	    break;
	case 11:
	    // "$ 999-" space after sym
	    p1 = currency;
	    p2 = " ";
	    p3 = ascBuff;
	    p4 = "-";
	    break;
	case 12:
	    // "$ -999" space after sym
	    p1 = currency;
	    p2 = " ";
	    p3 = "-";
	    p4 = ascBuff;
	    break;
	} // end of switch on nmode
    } // end of "else negative"

    // now build the string from the pieces:
    char currBuff[ 300 ];
    currBuff[0] = 0; // start with null string
    if ( p0 != 0 ) strcpy( currBuff, p0 );
    if ( p1 != 0 ) strcat( currBuff, p1 );
    if ( p2 != 0 ) strcat( currBuff, p2 );
    if ( p3 != 0 ) strcat( currBuff, p3 );
    if ( p4 != 0 ) strcat( currBuff, p4 );

    // convert that to widechar:
    BSTR bstr = toBSTR( codepage, currBuff, strlen(currBuff)+1 );
    if ( bstr == NULL ) return E_OUTOFMEMORY;

//### HACK HACK HACK
    // there is a bug in the MS GetLocaleInfoA!  For Japanese, it gives
    // the wrong Yen symbol...or at least not the one MS VBS uses
    //
    // This is an ugly way to do it, but...
    if ( lcid == 0x411 )
    {
	BSTR bptr = bstr;
	while ( *bptr != 0 )
	{
	    if ( *bptr == 92 ) *bptr = 165;
	    ++bptr;
	}
    }
//### HACK HACK HACK

    *pbstrOut = bstr;
    return S_OK;
}

// ***************************************************************************
// *
// * FormatDateTime -- bug for bug with MS
// *
// ***************************************************************************

static const int DAYSPERMONTH[] =
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
// let's be formal about this:
static const int DAYSPERYEAR = 365;
// one year in 4 is a leap year
static const int DAYSPERFOUR = DAYSPERYEAR * 4 + 1;
// xx00 is non leap year, assumed...
static const int DAYSPERCENTURY = 25 * DAYSPERFOUR - 1;
// by every 400 years *is* a leap year
static const int DAYSPERQUAD = 4 * DAYSPERCENTURY + 1;

static char * twodigits( char * pout, int num, int leading )
{
    int tens = num / 10;
    if ( tens != 0 || leading != 0 ) *pout++ = ( '0' + tens );
    *pout++ = ( '0' + ( num % 10 ) );
    return pout;
}

static char * fourdigits( char * pout, int num )
{
    pout = twodigits( pout, num / 100, 1 );
    pout = twodigits( pout, num % 100, 1 );
    return pout;
}

HRESULT
_VAR_FormatDateTime(
	LCID lcid,
	LPVARIANT pvarIn,
	int iNamedFormat,
	ULONG dwFlags,
	BSTR * pbstrOut)
{
    VARIANTARG vtemp;
    LPVARIANT pv;
    int year, month, day;
    int hours, minutes, seconds;
    int sz;
    int leading;

    // for all these routines, we need the codepage first:
    int codepage = getCPfromLCID( lcid );

    // A date is *actually* a double precision floating point number,
    // so we accept either type...
    if ( pvarIn->vt == VT_R8 || pvarIn->vt == VT_DATE )
    {
	pv = pvarIn;
    }
    else
    {
	// or convert whatever we were given to a date...
	pv = &vtemp;
	pv->vt = 0;
	if ( VariantChangeTypeEx( pv, pvarIn, lcid, 0, VT_DATE ) != S_OK )
	{
		return E_INVALIDARG;
	}
    }

    // ??? should use macro instead of .dblVal !!!
    double rawdate = pv->dblVal;

    // now start processing...
    //
    int february = 28; // assume not a leap year

    // separate days since 12/30/1899 from the time...
    int numdays = (int) rawdate;
    double dayfraction = rawdate - numdays;

    // adjust all the way back to year zero:
    numdays += 693959; // magic number...it's right

#ifdef NT_DEBUG_ONLY
    CConvert::infoitem[0] = rawdate;
    CConvert::infoitem[1] = numdays;
    CConvert::infoitem[2] = dayfraction;
#endif

    // we don't handle B.C. dates
    if ( numdays < 0 ) return BADARG(0xC1);

    // find what the weekday is...
    int weekday = ( numdays + 5 ) % 7; // ?????????

    // first, find out how many 400 year chunks there are...
    year = 400 * (numdays / DAYSPERQUAD); // (times 400 of course)
    // and we then only further process what is left over
    numdays %= DAYSPERQUAD;

    if ( numdays <= DAYSPERYEAR ) // 366 days in a leap year, so use <=
    {
	// which is what year 0, 400, 800, etc., are...
	february = 29;
	// and we don't need to go further!
    }
    else
    {
	// not 0, 400, 800, etc.
	//
	// because DAYSPERCENTURY assumes that *all* centuries
	// are non leap year, we have to adjust numdays to be
	// consistent with that assumption
	--numdays; // in other words, we fake it

	// same logic as with 100 year chunks, above
	year += ( 100 * (numdays / DAYSPERCENTURY) );
	numdays %= DAYSPERCENTURY;

	// only 365 days in first year of a century...
	if (numdays < DAYSPERYEAR )  // we use < becuz only 365 days
	{
	    february = 28; // redundant, since it's initialized above
	    // and we go no further...
	}
	else
	{
	    // not a century year
	    //
	    // fix up the "fake" we did before:
	    ++numdays;

	    // same logic as with 100 year chunks, above
	    year += ( 4 * (numdays / DAYSPERFOUR) );
	    numdays %= DAYSPERFOUR;

	    // 366 days in first year of a 4 year set...
	    if ( numdays <= DAYSPERYEAR ) // 366 days in a leap year, so use <=
	    {
		// first year of 4...leap year
		february = 29;
	    }
	    else
	    {
		// not a leap year
		//
		// because DAYSPERYEAR assumes that *all* years
		// are non leap year, we have to adjust numdays to be
		// consistent with that assumption
		-- numdays; // in other words, we fake it

		year += ( numdays / DAYSPERYEAR );
		numdays %= DAYSPERYEAR;
	    }
	}
    }

    // now find which month this is...
    month = 0;
    while( numdays >= ( ( month == 1 ) ? february : DAYSPERMONTH[month] ) )
    {
	numdays -= ( ( month == 1 ) ? february : DAYSPERMONTH[month] );
	++month;
    }
    ++month; // change zero based to human form
    day = numdays + 1; // ditto

    // *****************************************
    // now do time...
    //
    dayfraction += 2.0E-6; // about a tenth of a second...for safety
    dayfraction *= 24.0;
    hours = (int) dayfraction;
    dayfraction = 60.0 * ( dayfraction - hours );
    minutes = (int) dayfraction;
    dayfraction = 60.0 * ( dayfraction - minutes );
    seconds = (int) (dayfraction + 0.5); // round the seconds

    // ***********************************************************************
    // all above did was get the date and time into individually usable pieces
    // ***********************************************************************

    // *NOW* start formatting...

    // buffers...
    char dout[256], dfmt[256];

    char * pout = dout;
    char * pfmt = dfmt;

    if ( iNamedFormat < 3 )
    {
	// if we NEED to get a formatted date...
	int which = LOCALE_SSHORTDATE; // assume short
	if ( iNamedFormat == 1 )
	{
		which = LOCALE_SLONGDATE; // explicitly requested
	}

	// get the format
	if ( GetLocaleInfoA( lcid, which, dfmt, 255 ) < 3 )
	{
		return BADARG(0xC2);
	}

#ifdef NT_DEBUG_ONLY
	strcpy( CConvert::infostring[0], dfmt );
#endif

	while ( *pfmt != 0 )
	{
	    switch( *pfmt )
	    {
	    case '\'': // literal string
		++pfmt; // past apostrophe
		while( *pfmt != '\'' ) *pout++ = *pfmt++;
		++pfmt; // past terminating apostrophe
		break;

	    case 'd': // D should never be found?
		if ( (* ++pfmt) != 'd' )
		{
			// no leading zero, numeral day
			pout = twodigits( pout, day, 0 );
		}
		// "dd" found, but no third 'd'
		else if ( (*++pfmt) != 'd' ) {
			// "dd" found...leading zero, but numeral day
			pout = twodigits( pout, day, 1 );
		}
		// "ddd" found...need one more 'd'
		else if ( (*++pfmt) != 'd' ) {
			return BADARG(0xC3); // naughty naughty
		} else {
			// "dddd" found...use weekday name...
			sz = GetLocaleInfoA(lcid,LOCALE_SDAYNAME1+weekday,pout,64);
			if ( sz == 0 ) return BADARG(0xC4);
			++pfmt; // past the fourth 'd'
			pout += (sz-1); // adjust output pointer
		}
		break;

	    case 'M': // m should never be found?
		if ( (* ++pfmt) != 'M' )
		{
		    // no leading zero, numeral month
		    pout = twodigits( pout, month, 0 );
		}
		// "MM" found, but no third 'M'
		else if ( (*++pfmt) != 'M' )
		{
		    // "MM" found...leading zero, but numeral month
		    pout = twodigits( pout, month, 1 );
		}
		// "MMM" found...need one more 'M'
		else if ( (*++pfmt) != 'M' ) {
		    return BADARG(0xC5); // naughty naughty
		}
		else
		{
		    // "MMMM" found...use month name...
		    sz = GetLocaleInfoA(lcid,LOCALE_SMONTHNAME1+month-1,pout,64);
		    if ( sz == 0 ) return BADARG(0xC6);
		    ++pfmt; // past the fourth 'M'
		    pout += (sz-1); // adjust output pointer
		}
		break;

	    case 'y': // Y should never be found?
		// must have at least "yy"
		if ( (*++pfmt) != 'y' ) return BADARG(0xC7);
		if ( (*++pfmt) != 'y' )
		{
		    // "yy" found...
#ifdef COMPENSATE_FOR_LOCALE_INFO
		    // but we have to special case format 0...  it ignores the
		    // "yy" for years outside the current 1930 to 2029 range...
		    if ( iNamedFormat == 0 && (year < 1930 || year > 2029 ) )
		    {
			// sigh...needs 4 digit year
			pout = fourdigits( pout, year );
		    }
		    else
		    {
			// two digit year needed
			pout = twodigits( pout, year%100, 1 );
		    }
#else /* COMPENSATE_FOR_LOCALE_INFO */
		    // two digit year needed
		    pout = twodigits( pout, year%100, 1 );
#endif
		}
		// "yyy" found...need one more 'y'
		else if ( (*++pfmt) != 'y' )
		{
		    return BADARG(0xC8); // naughty naughty
		} else {
		    // "yyyy" found...use 4 digit year...
		    pout = fourdigits( pout, year );
		    ++pfmt; // past 4th y
		}
		break;

	    default:
		*pout++ = *pfmt++;
		break;
	    } // end the switch

	} // loop on pfmt

    }
    // if general format, we need a space after the date...
    if ( iNamedFormat == 0 && ( hours != 0 || minutes != 0 || seconds != 0 ) )
    {
	*pout++ = ' ';
	// and then the general format becomes long time format
	iNamedFormat = 3;
    }
    if ( iNamedFormat == 3 )
    {
	int hoursTemp = hours;
	char prior = 0;

	// we need long time value...
	if ( GetLocaleInfoA( lcid, LOCALE_STIMEFORMAT, dfmt, 255 ) < 3 )
	{
	    return BADARG(0xC9);
	}

#ifdef COMPENSATE_FOR_LOCALE_INFO
	// GetLocaleInfoA does not have the " tt" specifier given
	// for these locales, yet clearly it should.
	// Except that SouthAfrica (436) only uses " tt" if it
	// is after noon!
	if ( ( lcid == 0x1c09 || lcid == 0x2809 || lcid == 0x2C09 )
		    || ( lcid == 0x0436 && hours >= 12 ))
	{
	    strcat( dfmt, " tt" ); // so append the specifier!
	}
#endif
	// restart pfmt, in case this is format zero!
	// (pout does *not* restart, as it simply appends to dout)
	pfmt = dfmt;

	// processed very similar to the way date string is...
	while ( *pfmt != 0 )
	{
		switch( *pfmt )
		{
		case '\'': // literal string
		    ++pfmt; // past apostrophe
		    while( *pfmt != '\'' ) *pout++ = *pfmt++;
		    ++pfmt; // past terminating apostrophe
		    break;

		case 'h': // 12 hour time
		    if ( hoursTemp > 12 ) hoursTemp -= 12; // 1300 -> 1PM
		    if ( hoursTemp == 0 ) hoursTemp = 12; // 0000 -> 12AM
		case 'H': // 24 hour time
		    prior = *pfmt; // h or H
		    if ( (* ++pfmt) != prior )
		    {
			    // no leading zero, numeral hour
			    leading = 0;
		    }
		    // "HH" or "hh" found
		    else
		    {
			++pfmt; // past the second 'h' or 'H' character
			// need a leading zero
			leading = 1;
#ifdef COMPENSATE_FOR_LOCALE_INFO
			// GetLocaleInfoA says these two locales use 2 digit
			// hour, but VBScript clearly does not agree
			if ( lcid == 0x040D || lcid == 0x081D )
			{
			    leading = 0;
			}
#endif
		    }
		    pout = twodigits( pout, hoursTemp, leading );
		    break;

		case 'm': // M should never be found?
		    if ( (* ++pfmt) != 'm' )
		    {
			    leading = 0; // no leading zero, numeral minutes
		    } // "mm" found
		    else
		    {
			    leading = 1; // need leading zero
			    ++pfmt; // past the second character
		    }
		    pout = twodigits( pout, minutes, leading );
		    break;

		case 's': // S should never be found?
		    if ( (* ++pfmt) != 's' )
		    {
			leading = 0; // no leading zero, numeral seconds
		    } // "ss" found
		    else
		    {
			leading = 1; // need leading zero
			++pfmt; // past the second character
		    }
		    pout = twodigits( pout, seconds, leading );
		    break;

		case 't': // T should never be found?
		    if ( (* ++pfmt) != 't' )
		    {
			    // error! must have "tt"
			    return BADARG(0xCA);
		    } // "tt" found
		    else
		    {
			++pfmt; // past the second 't' character
			int which = ( hours >= 12 ? LOCALE_S2359 : LOCALE_S1159 );
			sz = GetLocaleInfoA( lcid, which, pout, 64 );
			if ( sz == 0 ) return BADARG(0xCB);
			pout += (sz-1); // adjust output pointer
		    }
		    break;

#ifdef COMPENSATE_FOR_LOCALE_INFO
		// GetLocaleInfoA has a period before the AM/PM specifier
		// for Albania, but VBS does not put it there.
		case '.':
		    *pout++ = ( lcid == 0x041C ) ? ' ' : '.'; // space for Albanian!
		    ++pfmt; // ignore the format character
		    break;
#endif
		default:
		    // just copy the character
		    *pout++ = *pfmt++;
		    break;
		} // end the switch

	    } // loop on pfmt
	// done with long date format!
    }
    else if ( iNamedFormat == 4 )
    {
	// short time format.
	//
	// This is simple:  It is *ALWAYS* HH:mm, excepting only
	// that the : delimiter is . in a few locales.
	//
	pout = twodigits( pout, hours, 1 );
	*pout++ = getCharacterInfo( lcid, LOCALE_STIME );
	pout = twodigits( pout, minutes, 1 );
    }
    *pout = 0; // terminate string!

    // convert the final result to widechar:
    BSTR bstr = toBSTR( codepage, dout, strlen(dout)+1);
    if ( bstr == NULL ) return E_OUTOFMEMORY;

    *pbstrOut = bstr;
    return S_OK;
}
