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

extern "C"
{

typedef struct _GUID {          // size is 16
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[8];
} IID;

IID IID_IObjectSafety =
    { 0xcb5bdc81, 0x93c1, 0x11cf, { 0x8f, 0x20, 0x00, 0x80, 0x5f, 0x2c, 0xd0, 0x64}};

IID IID_IConnectionPointContainer =
    { 0xB196B284, 0xBAB4, 0x101A, { 0xB6, 0x9C, 0x00, 0xAA, 0x00, 0x34, 0x1D, 0x07}};

IID IID_IProvideClassInfo =
    { 0xB196B283, 0xBAB4, 0x101A, { 0xB6, 0x9C, 0x00, 0xAA, 0x00, 0x34, 0x1D, 0x07}};

IID IID_IDispatchEx =
    { 0xa6ef9860, 0xc720, 0x11d0, { 0x93, 0x37, 0x00, 0xa0, 0xc9, 0x0d, 0xca, 0xa9}};

IID IID_IProvideMultipleClassInfo =
    { 0xa7aba9c1, 0x8983, 0x11cf, { 0x8f, 0x20, 0x00, 0x80, 0x5f, 0x2c, 0xd0, 0x64}};

//IID IID_IActiveScriptParseProcedure = 
//    { 0xaa5b6a80, 0xb834, 0x11d0, { 0x93, 0x2f, 0x00, 0xa0, 0xc9, 0x0d, 0xca, 0xa9}};

IID IID_IHostInfoUpdate =
    { 0x1d044690, 0x8923, 0x11d0, { 0xab, 0xd2, 0x00, 0xa0, 0xc9, 0x11, 0xe8, 0xb2}};

IID IID_IHostInfoProvider =
    { 0xf8418ae0, 0x9a5d, 0x11d0, { 0xab, 0xd4, 0x00, 0xa0, 0xc9, 0x11, 0xe8, 0xb2}};

}

