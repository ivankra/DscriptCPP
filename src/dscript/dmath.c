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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>

#if defined __DMC__
#include <fp.h>
#endif

#if _MSC_VER
#include <float.h>
#define	M_LOG2E		1.4426950408889634074
#define	M_LN2		0.6931471805599453094172321
#define	M_PI		3.14159265358979323846
#define	M_E		2.7182818284590452354
#define	M_LOG10E	0.43429448190325182765
#define	M_LN10		2.30258509299404568402
#define	M_PI_2		1.57079632679489661923
#define	M_PI_4		0.78539816339744830962
#define	M_1_PI		0.31830988618379067154
#define	M_2_PI		0.63661977236758134308
#define	M_2_SQRTPI	1.12837916709551257390
#define	M_SQRT2		1.41421356237309504880
#define	M_SQRT1_2	0.70710678118654752440
#endif

#include "port.h"
#include "root.h"
#include "dscript.h"
#include "value.h"
#include "dobject.h"
#include "text.h"

d_number math_helper(unsigned argc, Value *arglist)
{
    Value *v;

    v = argc ? &arglist[0] : &vundefined;
    return v->toNumber();
}

BUILTIN_FUNCTION2(Dmath_, abs, 1)
{
    // ECMA 15.8.2.1
    d_number result;

    result = fabs(math_helper(argc, arglist));
    Vnumber::putValue(ret, result);
    return NULL;
}

BUILTIN_FUNCTION2(Dmath_, acos, 1)
{
    // ECMA 15.8.2.2
    d_number result;

    result = acos(math_helper(argc, arglist));
    Vnumber::putValue(ret, result);
    return NULL;
}

BUILTIN_FUNCTION2(Dmath_, asin, 1)
{
    // ECMA 15.8.2.3
    d_number result;

    result = asin(math_helper(argc, arglist));
    Vnumber::putValue(ret, result);
    return NULL;
}

BUILTIN_FUNCTION2(Dmath_, atan, 1)
{
    // ECMA 15.8.2.4
    d_number result;

    result = atan(math_helper(argc, arglist));
    Vnumber::putValue(ret, result);
    return NULL;
}

BUILTIN_FUNCTION2(Dmath_, atan2, 2)
{
    // ECMA 15.8.2.5
    Value *v1;
    Value *v2;
    d_number result;

    v1 = argc ? &arglist[0] : &vundefined;
    v2 = (argc >= 2) ? &arglist[1] : &vundefined;
    result = Port::atan2(v1->toNumber(), v2->toNumber());
    Vnumber::putValue(ret, result);
    return NULL;
}

BUILTIN_FUNCTION2(Dmath_, ceil, 1)
{
    // ECMA 15.8.2.6
    d_number result;

    result = ceil(math_helper(argc, arglist));
    Vnumber::putValue(ret, result);
    return NULL;
}

BUILTIN_FUNCTION2(Dmath_, cos, 1)
{
    // ECMA 15.8.2.7
    d_number result;

    result = cos(math_helper(argc, arglist));
    Vnumber::putValue(ret, result);
    return NULL;
}

BUILTIN_FUNCTION2(Dmath_, exp, 1)
{
    // ECMA 15.8.2.8
    d_number result;

    result = Port::exp(math_helper(argc, arglist));
    Vnumber::putValue(ret, result);
    return NULL;
}

BUILTIN_FUNCTION2(Dmath_, floor, 1)
{
    // ECMA 15.8.2.9
    d_number result;

    result = floor(math_helper(argc, arglist));
    Vnumber::putValue(ret, result);
    return NULL;
}

BUILTIN_FUNCTION2(Dmath_, log, 1)
{
    // ECMA 15.8.2.10
    d_number result;

    result = log(math_helper(argc, arglist));
    Vnumber::putValue(ret, result);
    return NULL;
}

BUILTIN_FUNCTION2(Dmath_, max, 2)
{
    // ECMA v3 15.8.2.11
    Value *v;
    d_number n;
    d_number result;
    unsigned a;

    result = -Port::infinity;
    for (a = 0; a < argc; a++)
    {
	v = &arglist[a];
	n = v->toNumber();
	if (Port::isnan(n))
	{   result = Port::nan;
	    break;
	}
	if (result == n)
	{
	    // if n is +0 and result is -0, pick n
	    if (n == 0 && !Port::signbit(n))
		result = n;
	}
	else if (n > result)
	    result = n;
    }
    Vnumber::putValue(ret, result);
    return NULL;
}

BUILTIN_FUNCTION2(Dmath_, min, 2)
{
    // ECMA v3 15.8.2.12
    Value *v;
    d_number n;
    d_number result;
    unsigned a;

    result = Port::infinity;
    for (a = 0; a < argc; a++)
    {
	v = &arglist[a];
	n = v->toNumber();
	if (Port::isnan(n))
	{   result = Port::nan;
	    break;
	}
	if (result == n)
	{
	    // if n is -0 and result is +0, pick n
	    if (n == 0 && Port::signbit(n))
		result = n;
	}
	else if (n < result)
	    result = n;
    }
    Vnumber::putValue(ret, result);
    return NULL;
}

BUILTIN_FUNCTION2(Dmath_, pow, 2)
{
    // ECMA 15.8.2.13
    Value *v1;
    Value *v2;
    d_number result;

    v1 = argc ? &arglist[0] : &vundefined;
    v2 = (argc >= 2) ? &arglist[1] : &vundefined;
    result = Port::pow(v1->toNumber(), v2->toNumber());
    Vnumber::putValue(ret, result);
    return NULL;
}

BUILTIN_FUNCTION2(Dmath_, random, 0)
{
    // ECMA 15.8.2.14
    // 0.0 <= result < 1.0
    d_number result;

    result = randomd();

    // Experiments on linux show that this will never be exactly
    // 1.0, so is the assert() worth it?
    assert(result >= 0 && result < 1.0);
    Vnumber::putValue(ret, result);
    return NULL;
}

BUILTIN_FUNCTION2(Dmath_, round, 1)
{
    // ECMA 15.8.2.15
    d_number result;

    result = math_helper(argc, arglist);
    if (!Port::isnan(result))
#if _MSC_VER
	result = _copysign(floor(result + .5), result);
#else
	result = copysign(floor(result + .5), result);
#endif
    Vnumber::putValue(ret, result);
    return NULL;
}

BUILTIN_FUNCTION2(Dmath_, sin, 1)
{
    // ECMA 15.8.2.16
    d_number result;

    result = sin(math_helper(argc, arglist));
    Vnumber::putValue(ret, result);
    return NULL;
}

BUILTIN_FUNCTION2(Dmath_, sqrt, 1)
{
    // ECMA 15.8.2.17
    d_number result;

    result = sqrt(math_helper(argc, arglist));
    Vnumber::putValue(ret, result);
    return NULL;
}

BUILTIN_FUNCTION2(Dmath_, tan, 1)
{
    // ECMA 15.8.2.18
    d_number result;

    result = tan(math_helper(argc, arglist));
    Vnumber::putValue(ret, result);
    return NULL;
}

Dmath::Dmath(ThreadContext *tc)
    : Dobject(tc->Dobject_prototype)
{
    //WPRINTF(L"Dmath::Dmath(%x)\n", this);
    unsigned attributes = DontEnum | DontDelete | ReadOnly;

    static struct MathConst
    {	d_string *name;
	d_number value;
    } table[] =
    {
	{ &TEXT_E,       M_E       },
	{ &TEXT_LN10,    M_LN10    },
	{ &TEXT_LN2,     M_LN2     },
	{ &TEXT_LOG2E,   M_LOG2E   },
	{ &TEXT_LOG10E,  M_LOG10E  },
	{ &TEXT_PI,      M_PI      },
	{ &TEXT_SQRT1_2, M_SQRT1_2 },
	{ &TEXT_SQRT2,   M_SQRT2   },
    };

    for (unsigned u = 0; u < sizeof(table) / sizeof(table[0]); u++)
    {	Value *v;

	v = Put(*table[u].name, table[u].value, attributes);
	//WPRINTF(L"Put(%s,%.5g) = %x\n", d_string_ptr(table[u].name), table[u].value, v);
    }

    classname = TEXT_Math;

    static NativeFunctionData nfd[] =
    {
    #define X(name,len) { &TEXT_##name, #name, Dmath_##name, len }
	X(abs, 1),
	X(acos, 1),
	X(asin, 1),
	X(atan, 1),
	X(atan2, 2),
	X(ceil, 1),
	X(cos, 1),
	X(exp, 1),
	X(floor, 1),
	X(log, 1),
	X(max, 2),
	X(min, 2),
	X(pow, 2),
	X(random, 0),
	X(round, 1),
	X(sin, 1),
	X(sqrt, 1),
	X(tan, 1),
    #undef X
    };

    DnativeFunction::init(this, nfd, sizeof(nfd) / sizeof(nfd[0]), attributes);
}

void Dmath::init(ThreadContext *tc)
{
    srand(time(NULL));		// set random number generator

    GC_LOG();
    tc->Dmath_object = new(&tc->mem) Dmath(tc);
}
