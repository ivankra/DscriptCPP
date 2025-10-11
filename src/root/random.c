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

/*
* --------------------------------------------------------------------
*  random.c
*
*  Segments of the code in this file Copyright (C) 1997 by Rick Booth
*  From "Inner Loops" by Rick Booth, Addison-Wesley
* --------------------------------------------------------------------
*/

#include "root.h"
#if defined linux
#  include <sys/time.h>
#endif

static longlong seedran();

#ifdef NO_RANDOM_STATICS
// Use passed arguments instead of statics for dsseed and dsindex
// RANDOM_ARGS and RANDOM_PARAMS are pointers args to dsseed and dsindex
#define dsseed *pdsseed
#define dsindex *pdsindex

#else
// Use local statics for dsseed and dsindex
// RANDOM_ARGS and RANDOM_PARAMS are empty
static unsigned long dsseed;
static unsigned long dsindex;

// Seed random number generator

void seedrandom(long seed)
{
     dsseed = seed;
     dsindex = RANDOM_INDEX(seed);
}
#endif

/* ===================== Random ========================= */

static long randomx_long(RANDOM_ARGS)
{
    static long xormix1[] = {
                0xbaa96887, 0x1e17d32c, 0x03bcdc3c, 0x0f33d1b2,
                0x76a6491d, 0xc570d85d, 0xe382b1e3, 0x78db4362,
                0x7439a9d4, 0x9cea8ac5, 0x89537c5c, 0x2588f55d,
                0x415b5e1d, 0x216e3d95, 0x85c662e7, 0x5e8ab368,
                0x3ea5cc8c, 0xd26a0f74, 0xf3a9222b, 0x48aad7e4};

    static long xormix2[] = {
                0x4b0f3b58, 0xe874f0c3, 0x6955c5a6, 0x55a7ca46,
                0x4d9a9d86, 0xfe28a195, 0xb1ca7865, 0x6b235751,
                0x9a997a61, 0xaa6e95c8, 0xaaa98ee1, 0x5af9154c,
                0xfc8e2263, 0x390f5e8c, 0x58ffd802, 0xac0a5eba,
                0xac4874f6, 0xa9df0913, 0x86be4c74, 0xed2c123b};

    unsigned long hiword, loword, hihold, temp, itmpl, itmph, i;

    if (!dsseed)
    {	longlong s;

	s = seedran();
	dsseed = (unsigned long) s;
	dsindex = (unsigned long) (s >> 32);
    }

    loword = dsseed;
    hiword = dsindex++;
    for (i = 0; i < 4; i++) {
        hihold  = hiword;                           // save hiword for later
        temp    = hihold ^  xormix1[i];             // mix up bits of hiword
        itmpl   = temp   &  0xffff;                 // decompose to hi & lo
        itmph   = temp   >> 16;                     // 16-bit words
        temp    = itmpl * itmpl + ~(itmph * itmph); // do a multiplicative mix
        temp    = (temp >> 16) | (temp << 16);      // swap hi and lo halves
        hiword  = loword ^ ((temp ^ xormix2[i]) + itmpl * itmph); //loword mix
        loword  = hihold;                           // old hiword is loword
    }
    return hiword;
}

longlong randomx(RANDOM_ARGS)
{
#ifdef NO_RANDOM_STATICS
    return ((longlong)randomx_long(pdsseed,pdsindex) << 32) +
                      randomx_long(pdsseed,pdsindex);
#else
    return ((longlong)randomx_long() << 32) + randomx_long();
#endif
}

// Return random number as a double >= 0.0 and < 1.0

double randomd(RANDOM_ARGS)
{
    // 0.0 <= result < 1.0
    double result;
#if _MSC_VER
    __int64 x;

    // Only want 53 bits of precision
    x = randomx(RANDOM_PARAMS) & 0xFFFFFFFFFFFFF800I64;
    result = x  * (1 / (0x10000000 * 16.0 * 0x10000000 * 16.0))
		+ (1 / (0x20000000 * 16.0 * 0x10000000 * 16.0));

    // VC doesn't convert unsigned long long to double. So
    // the value we get back is -.5 <= result < .5
    // Shift result.
    if (result < 0)
	result += 1.0;
#else
    unsigned long long x;

    // Only want 53 bits of precision
    x = randomx(RANDOM_PARAMS);
    //PRINTF("x = x%016llx\n",x);
    x &= 0xFFFFFFFFFFFFF800LL;
    result = x  * (1 / (0x100000000LL * (double)0x100000000LL))
		+ (1 / (0x200000000LL * (double)0x100000000LL));
#endif
    // Experiments on linux show that this will never be exactly
    // 1.0, so is the assert() worth it?
    //assert(result >= 0 && result < 1.0);
    return result;
}

#ifdef linux

#include <time.h>
static longlong seedran()
{
    struct timeval tv;

    if (gettimeofday(&tv, NULL))
    {	// Some error happened - try time() instead
	return ::time(NULL);
    }

    return tv.tv_sec * 0x100000000LL + tv.tv_usec;
}

#else

#include <time.h>
static longlong seedran()
{
    return time(NULL);
}

#endif // linux

