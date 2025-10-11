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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#include "port.h"
#include "root.h"
#include "lstring.h"
#include "mem.h"
#include "date.h"

#include "dscript.h"
#include "value.h"
#include "dobject.h"
#include "text.h"
#include "program.h"

dchar TypeUndefined[] = DTEXT("Undefined");
dchar TypeNull[]      = DTEXT("Null");
dchar TypeBoolean[]   = DTEXT("Boolean");
dchar TypeNumber[]    = DTEXT("Number");
dchar TypeString[]    = DTEXT("String");
dchar TypeObject[]    = DTEXT("Object");

dchar TypeDate[]      = DTEXT("Date");
//dchar TypeVBArray[]   = DTEXT("VBArray");

/*************************** Value ***********************/

void * Value::operator new(size_t m_size)
{
   GC_LOG();
   return mem.malloc(m_size);
}

void * Value::operator new(size_t m_size, Mem *mymem)
{
   GC_LOG();
   return Mem::operator new(m_size, mymem);
}

void * Value::operator new(size_t m_size, GC *mygc)
{
   GC_LOG();
   return Mem::operator new(m_size, mygc);
}

void Value::operator delete(void *p)
{
    mem.free(p);
}


void *Value::toPrimitive(Value *v, dchar *PreferredType)
{
    Value::copy(v, this);
    return NULL;
}

d_boolean Value::toBoolean()
{
    assert(0);
    return FALSE;
}

d_number Value::toNumber()
{
    assert(0);
    return 0;
}

d_number Value::toInteger()
{   d_number number;

    number = toNumber();
    if (Port::isnan(number))
	number = 0;
    else if (number == 0 || Port::isinfinity(number))
	;
    else if (number > 0)
	number = Port::floor(number);
    else
	number = -Port::floor(-number);
    return number;
}

d_int32 Value::toInt32()
{   d_int32 int32;
    d_number number;
    longlong ll;

    number = toNumber();
    if (Port::isnan(number))
	int32 = 0;
    else if (number == 0 || Port::isinfinity(number))
	int32 = 0;
    else
    {	if (number > 0)
	    number = Port::floor(number);
	else
	    number = -Port::floor(-number);

	ll = (longlong) number;
	int32 = (int) ll;
    }
    return int32;
}

d_uint32 Value::toUint32()
{   d_uint32 uint32;
    d_number number;
    longlong ll;

    number = toNumber();
    if (Port::isnan(number))
	uint32 = 0;
    else if (number == 0 || Port::isinfinity(number))
	uint32 = 0;
    else
    {	if (number > 0)
	    number = Port::floor(number);
	else
	    number = -Port::floor(-number);

	ll = (longlong) number;
	uint32 = (unsigned) ll;
    }
    return uint32;
}

d_uint16 Value::toUint16()
{   d_uint16 uint16;
    d_number number;

    number = toNumber();
    if (Port::isnan(number))
	uint16 = 0;
    else if (number == 0 || Port::isinfinity(number))
	uint16 = 0;
    else
    {	if (number > 0)
	    number = Port::floor(number);
	else
	    number = -Port::floor(-number);

#ifdef __GNUC__
	// G++ needs help getting the conversion right for 4294967295 (0xFFFFFFFF)
	uint16 = (unsigned short)(unsigned)number;
#else
	uint16 = (unsigned short)number;
#endif
    }
    return uint16;
}

d_string Value::toString()
{
    assert(0);
    return d_string_null;
}

d_string Value::toLocaleString()
{
    return toString();
}

d_string Value::toSource()
{
    return toString();
}

d_string Value::toString(int radix)
{
    return toString();
}

Dobject *Value::toObject()
{
    assert(0);
    return NULL;
}

void Value::copyTo(Value *to)
{
    // Copy everything, including vptr
    Value::copy(this, to);
}

dchar *Value::getType()
{
    assert(0);
    return NULL;
}

d_string Value::getTypeof()
{
    assert(0);
    return d_string_null;
}

int Value::isPrimitive()
{
    return TRUE;		// true for most cases
}

int Value::isUndefined()
{
    return FALSE;
}

int Value::isUndefinedOrNull()
{
    return FALSE;
}

#if !ISTYPE && !VPTR_HACK
int Value::isString()
{
    return FALSE;
}

int Value::isNumber()
{
    return FALSE;
}

int Value::isDate()
{
    return FALSE;
}

//int Value::isVBArray()
//{
//    return FALSE;
//}
#endif

int Value::isArrayIndex(d_uint32 *index)
{
    *index = 0;
    return FALSE;
}

unsigned Value::getHash()
{
    assert(0);
    return 0;
}

int Value::compareTo(Value *v)
{
    assert(0);
    return 0;
}

Value *Value::Put(d_string PropertyName, Value *value)
{
    ErrInfo errinfo;

    return Dobject::RuntimeError(&errinfo, ERR_CANNOT_PUT_TO_PRIMITIVE,
	    d_string_ptr(PropertyName), d_string_ptr(value->toString()),
	    getType());
}

Value *Value::Put(d_uint32 index, Value *value)
{
    ErrInfo errinfo;

    return Dobject::RuntimeError(&errinfo, ERR_CANNOT_PUT_INDEX_TO_PRIMITIVE,
	    index,
	    d_string_ptr(value->toString()), getType());
}

Value *Value::Get(d_string PropertyName, unsigned hash)
{
    RuntimeErrorx(ERR_CANNOT_GET_FROM_PRIMITIVE,
	    d_string_ptr(PropertyName), getType(), d_string_ptr(toString()));
    return &vundefined;
}

Value *Value::Get(d_uint32 index)
{
    RuntimeErrorx(ERR_CANNOT_GET_INDEX_FROM_PRIMITIVE,
	    index,
	    getType(), d_string_ptr(toString()));
    return &vundefined;
}

Value *Value::Get(d_string PropertyName)
{
    RuntimeErrorx(ERR_CANNOT_GET_FROM_PRIMITIVE,
	    d_string_ptr(PropertyName), getType(), d_string_ptr(toString()));
    return &vundefined;
}

void *Value::Construct(CONSTRUCT_ARGS)
{
    ErrInfo errinfo;
    Value::copy(ret, &vundefined);
    errinfo.code = 5100;
    return Dobject::RuntimeError(&errinfo, ERR_PRIMITIVE_NO_CONSTRUCT, getType());
}

void *Value::Call(CALL_ARGS)
{
    ErrInfo errinfo;
    //PRINTF("Call method not implemented for primitive %p (%s)\n", this, d_string_ptr(toString()));
    Value::copy(ret, &vundefined);
    return Dobject::RuntimeError(&errinfo, ERR_PRIMITIVE_NO_CALL, getType());
}

Value *Value::putIterator(Value *v)
{
    ErrInfo errinfo;
    Value::copy(v, &vundefined);
    return Dobject::RuntimeError(&errinfo, ERR_FOR_IN_MUST_BE_OBJECT);
}

void Value::getErrInfo(ErrInfo *perrinfo, int linnum)
{
    ErrInfo errinfo;
    OutBuffer buf;

    if (linnum && errinfo.linnum == 0)
	errinfo.linnum = linnum;
    buf.writedstring("Unhandled exception: ");
    buf.writedstring(d_string_ptr(toString()));
    buf.writedchar(0);
    errinfo.message = (dchar *)buf.data;
    buf.data = NULL;
    if (perrinfo)
	*perrinfo = errinfo;
}

#if VPTR_HACK

// Go through some contortions to get the vtbl[] values for Vnumber
// and Vstring, and store them into Value::vptr_xxxx

void *Value::vptr_Number = NULL;
void *Value::vptr_String = NULL;
void *Value::vptr_Date   = NULL;
//void *Value::vptr_VBArray= NULL;

void Value::init()
{
    Vstring vs((Lstring *)NULL);

    Value::vptr_Number = ((void **)&Vnumber::tmp)[VPTR_INDEX];
    Value::vptr_String = ((void **)&vs)          [VPTR_INDEX];
    Value::vptr_Date   = ((void **)&Vdate::tmp)  [VPTR_INDEX];
//    Value::vptr_VBArray= ((void **)&Vvbarray::tmp)[VPTR_INDEX];

#if 0
    // Use this to figure out the right VPTR_INDEX
    void **p = (void **)&Vnumber::tmp;
    WPRINTF(L"[0] = %x\n", p[0]);
    WPRINTF(L"[1] = %x\n", p[1]);
    WPRINTF(L"[2] = %x\n", p[2]);
    WPRINTF(L"[3] = %x\n", p[3]);
#endif
}

#else
void Value::init()
{
    Vstring vs((Lstring *)NULL);
    Vstring vs2((Lstring *)NULL);
    void **p = (void **)&vs;
    printf("sizeof() = %d\n", sizeof(Value));
    printf("[0] = %x\n", p[0]);
    printf("[1] = %x\n", p[1]);
    p = (void **)&vs2;
    printf("[0] = %x\n", p[0]);
    printf("[1] = %x\n", p[1]);
    printf("boolean  = %d\n", (char *)&vs.boolean - (char *)&vs);
    printf("number   = %d\n", (char *)&vs.number - (char *)&vs);
    printf("x.hash   = %d\n", (char *)&vs.x.hash - (char *)&vs);
    printf("x.string = %d\n", (char *)&vs.x.string - (char *)&vs);
    printf("object   = %d\n", (char *)&vs.object - (char *)&vs);
    printf("uint32   = %d\n", (char *)&vs.uint32 - (char *)&vs);
    printf("uint16   = %d\n", (char *)&vs.uint16 - (char *)&vs);
    printf("ptrvalue = %d\n", (char *)&vs.ptrvalue - (char *)&vs);
}
#endif

void Value::dump()
{   unsigned *v = (unsigned *)this;

    WPRINTF(L"v[%x] = %8x, %8x, %8x, %8x\n", v, v[0], v[1], v[2], v[3]);
}

/*************************** Undefined ***********************/

Vundefined vundefined;

Vundefined::Vundefined()
{
    x.hash = 0;
    x.string = NULL;
}

void Vundefined::invariant()
{
#if VALUE_INVARIANT
    Value *v = this;
    dchar *tx;

    tx = v->getType();
    assert(tx == TypeUndefined);
#endif
}

d_boolean Vundefined::toBoolean()
{
    return FALSE;
}

d_number Vundefined::toNumber()
{
    return Port::nan;
}

d_number Vundefined::toInteger()
{
    return Port::nan;
}

d_int32 Vundefined::toInt32()
{
    return 0;
}

d_uint32 Vundefined::toUint32()
{
    return 0;
}

d_uint16 Vundefined::toUint16()
{
    return 0;
}

d_string Vundefined::toString()
{
    return TEXT_undefined;
}

Dobject *Vundefined::toObject()
{
    //RuntimeErrorx(DTEXT("cannot convert undefined to Object"));
    return NULL;
}

unsigned Vundefined::getHash()
{
    return 0;
}

int Vundefined::isUndefined()
{
    return TRUE;
}

int Vundefined::isUndefinedOrNull()
{
    return TRUE;
}

int Vundefined::compareTo(Value *v)
{
    if (v->getType() == TypeUndefined)
	return 0;
    return -1;
}

dchar *Vundefined::getType()
{
    return TypeUndefined;
}

d_string Vundefined::getTypeof()
{
    return TEXT_undefined;
}

/*************************** Null ***********************/

Vnull vnull;

Vnull::Vnull()
{
}

d_boolean Vnull::toBoolean()
{
    return FALSE;
}

d_number Vnull::toNumber()
{
    return 0;
}

d_number Vnull::toInteger()
{
    return 0;
}

d_int32 Vnull::toInt32()
{
    return 0;
}

d_uint32 Vnull::toUint32()
{
    return 0;
}

d_uint16 Vnull::toUint16()
{
    return 0;
}

d_string Vnull::toString()
{
    return TEXT_null;
}

Dobject *Vnull::toObject()
{
    //RuntimeErrorx(DTEXT("cannot convert null to Object"));
    return NULL;
}

unsigned Vnull::getHash()
{
    return 0;
}

int Vnull::isUndefinedOrNull()
{
    return TRUE;
}

int Vnull::compareTo(Value *v)
{
    if(v->getType() == TypeNull)
	return 0;
    return -1;
}

dchar *Vnull::getType()
{
    return TypeNull;
}

d_string Vnull::getTypeof()
{
    return TEXT_object;
}

/*************************** Boolean ***********************/

Vboolean Vboolean::tmp(FALSE);

d_boolean Vboolean::toBoolean()
{
    return boolean;
}

d_number Vboolean::toNumber()
{
    return boolean ? 1 : 0;
}

d_number Vboolean::toInteger()
{
    return boolean ? 1 : 0;
}

d_int32 Vboolean::toInt32()
{
    return boolean ? 1 : 0;
}

d_uint32 Vboolean::toUint32()
{
    return boolean ? 1 : 0;
}

d_uint16 Vboolean::toUint16()
{
    return (d_uint16) (boolean ? 1 : 0);
}

d_string Vboolean::toString()
{
    return boolean ? TEXT_true : TEXT_false;
}

Dobject *Vboolean::toObject()
{
    GC_LOG();
    return new Dboolean(boolean);
}

unsigned Vboolean::getHash()
{
    return boolean ? 1 : 0;
}

int Vboolean::compareTo(Value *v)
{
#if ISTYPE
    if (v->__istype(Vboolean))
#else
    if (v->getType() == TypeBoolean)
#endif
	return v->boolean - boolean;
    return -1;
}

dchar *Vboolean::getType()
{
    return TypeBoolean;
}

d_string Vboolean::getTypeof()
{
    return TEXT_boolean;
}

/*************************** Number ***********************/

Vnumber Vnumber::tmp(0);

#if 0
void Vnumber::putValue(Value *v, d_number n)
{
    Vnumber tmp(n);

    Value::copy(v, &tmp);
}
#endif

#if 0 // inline'd
Vnumber::Vnumber(d_number number)
{
    this->number = number;
}
#endif

d_boolean Vnumber::toBoolean()
{
    return !(number == 0.0 || Port::isnan(number));
}

d_number Vnumber::toNumber()
{
    return number;
}

d_string Vnumber::toString()
{   d_string string;
    static d_string *strings[10] =
	{ &TEXT_0,&TEXT_1,&TEXT_2,&TEXT_3,&TEXT_4,
	  &TEXT_5,&TEXT_6,&TEXT_7,&TEXT_8,&TEXT_9 };

    //PRINTF("Vnumber::toString(%g)\n", number);
    if (Port::isnan(number))
	string = TEXT_NaN;
    else if (number >= 0 && number <= 9 && number == (int) number)
	string = *strings[(int) number];
    else if (Port::isinfinity(number))
    {
	if (number < 0)
	    string = TEXT_negInfinity;
	else
	    string = TEXT_Infinity;
    }
    else
    {
	dchar buffer[100];	// should shrink this to max size,
				// but doesn't really matter
	dchar *p;

	// ECMA 262 requires %.21g (21 digits) of precision. But, the
	// C runtime library doesn't handle that. Until the C runtime
	// library is upgraded to ANSI C 99 conformance, use
	// 16 digits, which is all the GCC library will round correctly.
#if defined UNICODE
	SWPRINTF(buffer, sizeof(buffer)/sizeof(buffer[0]), L"%.16g", number);
#else
	sprintf(buffer, "%.16g", number);
	assert(strlen(buffer) < sizeof(buffer));
#endif
	// Trim leading spaces
	for (p = buffer; *p == ' '; p++)
	    ;

	{   // Trim any 0's following exponent 'e'
	    dchar *q;
	    dchar *t;

	    for (q = p; *q; q++)
	    {
		if (*q == 'e')
		{
		    q++;
		    if (*q == '+' || *q == '-')
			q++;
		    t = q;
		    while (*q == '0')
			q++;
		    if (t != q)
		    {
			for (;;)
			{
			    *t = *q;
			    if (*t == 0)
				break;
			    t++;
			    q++;
			}
		    }
		    break;
		}
	    }
	}
	GC_LOG();
	Mem mem;
	string = Dstring::dup(&mem, p);
    }
    //WPRINTF(L"string = '%ls'\n", d_string_ptr(string));
    return string;
}

d_string Vnumber::toString(int radix)
{
    dchar string[32 + 1];

    assert(2 <= radix && radix <= 36);
#if defined(UNICODE)
    // Investigate using _i64tow() instead
    Port::itow((int) number, string, radix);
#else
    // Investigate using _i64toa() instead
    Port::itoa((int) number, string, radix);
#endif
    assert(((unsigned)Dchar::len(string)) < sizeof(string) / sizeof(string[0]));
    GC_LOG();
    Mem mem;
    return Dstring::dup(&mem, string);
}

Dobject *Vnumber::toObject()
{
    GC_LOG();
    return new Dnumber(number);
}

int Vnumber::isArrayIndex(d_uint32 *index)
{
    *index = toUint32();
    return TRUE;
}

unsigned Vnumber::getHash()
{
    return HASH((unsigned) number);
}

int Vnumber::compareTo(Value *v)
{
    if (v->isNumber())
    {
	if (number == v->number)
	    return 0;
	else if (number > v->number)
	    return 1;
    }
    else if (v->isString())
    {
	return Lstring::cmp(toString(), v->x.string);
    }
    return -1;
}

#if !ISTYPE && !VPTR_HACK
int Vnumber::isNumber()
{
    return TRUE;
}
#endif

dchar *Vnumber::getType()
{
    return TypeNumber;
}

d_string Vnumber::getTypeof()
{
    return TEXT_number;
}

/*************************** String ***********************/


void Vstring::putValue(Value *v, dchar *s)
{
    Vstring tmp(s);

    //assert(s);
    Value::copy(v, &tmp);
}

#if 0
Vstring::Vstring(d_string s)
{
    x.hash = 0;
    x.string = s;
}

Vstring::Vstring(d_string s, unsigned hash)
{
    x.hash = hash;
    x.string = s;
}
#endif

d_boolean Vstring::toBoolean()
{
    return x.string->len() ? TRUE : FALSE;
}

d_number Vstring::toNumber()
{
    dchar *endptr;
    d_string s;
    d_number n;
    unsigned len;

    len = d_string_len(x.string);
    s = x.string;
    n = Dstring::toNumber(s, &endptr);

    // Trailing whitespace
    while (Dstring::isStrWhiteSpaceChar(*endptr))
	endptr++;
    if (endptr - d_string_ptr(s) != (int)len)
	n = Port::nan;

    return n;
}

d_string Vstring::toString()
{
    return x.string;
}

d_string Vstring::toSource()
{
    OutBuffer buf;
    d_string s;

    buf.reserve(2 + x.string->len() + 1);
    buf.writedchar('"');
    buf.writedstring(x.string->toDchars());
    buf.writedchar('"');
    buf.writedchar(0);
    s = d_string_ctor((dchar *)buf.data);
    buf.data = NULL;
    return s;
}

Dobject *Vstring::toObject()
{
    GC_LOG();
    return new Dstring(x.string);
}

int Vstring::isArrayIndex(d_uint32 *index)
{
    return StringToIndex(x.string, index);
}

unsigned Vstring::calcHash(d_string s)
{
    unsigned hash;
    unsigned len;
    dchar *p;

    // If it looks like an array index, hash it to the
    // same value as if it was an array index.
    hash = 0;
    for (p = d_string_ptr(s); ; p++)
    {
	if (*p == 0)
	    return HASH(hash);
	switch (*p)
	{
	    case '0':	hash *= 10;		break;
	    case '1':	hash = hash * 10 + 1;	break;

	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
		hash = hash * 10 + (*p - '0');
		break;

	    default:
		len = d_string_len(s);
		return Dchar::calcHash(d_string_ptr(s), len);
	}
    }
}

unsigned Vstring::getHash()
{
    // Since strings are immutable, if we've already
    // computed the hash, use previous value
    if (!x.hash)
	x.hash = calcHash(x.string);
    return x.hash;
}

int Vstring::compareTo(Value *v)
{
    if (v->isString())
    {
	//WPRINTF(L"'%ls'->compareTo('%ls')\n", x.string->toDchars(), v->x.string->toDchars());
	if (x.string == v->x.string)
	{   //PRINTF("\tp\n");
	    return 0;
	}
	return Lstring::cmp(x.string, v->x.string);
    }
    else if (v->getType() == TypeNumber)
    {
	//WPRINTF(L"'%ls'->compareTo(%g)\n", x.string->toDchars(), v->number);
	return d_string_cmp(x.string, v->toString());
    }
    return -1;
}

#if !ISTYPE && !VPTR_HACK
int Vstring::isString()
{
    return TRUE;
}
#endif

dchar *Vstring::getType()
{
    return TypeString;
}

d_string Vstring::getTypeof()
{
    return TEXT_string;
}

/*************************** Object ***********************/

void Vobject::putValue(Value *v, Dobject *o)
{
    Vobject tmp(o);

    Value::copy(v, &tmp);
}

#if 0 // inline'd
Vobject::Vobject(Dobject *o)
{
    this->object = o;
}
#endif

void *Vobject::toPrimitive(Value *v, dchar *PreferredType)
{
    /*	ECMA 9.1
	Return a default value for the Object.
	The default value of an object is retrieved by
	calling the internal [[DefaultValue]] method
	of the object, passing the optional hint
	PreferredType. The behavior of the [[DefaultValue]]
	method is defined by this specification for all
	native ECMAScript objects (see section 8.6.2.6).
	If the return value is of type Object or Reference,
	a runtime error is generated.
     */
    void *a;

    assert(object);
    a = object->DefaultValue(v, PreferredType);
    if (a)
	return a;
    if (!v->isPrimitive())
    {
	ErrInfo errinfo;

	Value::copy(v, &vundefined);
	return Dobject::RuntimeError(&errinfo, ERR_OBJECT_CANNOT_BE_PRIMITIVE);
    }
    return NULL;
}

d_boolean Vobject::toBoolean()
{
    return TRUE;
}

d_number Vobject::toNumber()
{   Value val;
    Value *v;

    //PRINTF("Vobject::toNumber()\n");
    v = &val;
    toPrimitive(v, TypeNumber);
    if (v->isPrimitive())
	return v->toNumber();
    else
	return Port::nan;
}

d_string Vobject::toString()
{   Value val;
    Value *v = &val;
    void *a;

    //WPRINTF(L"Vobject::toString()\n");
    a = toPrimitive(v, TypeString);
    //assert(!a);
    if (v->isPrimitive())
	return v->toString();
    else
	return v->toObject()->classname;
}

d_string Vobject::toSource()
{   Value *v;

    //PRINTF("Vobject::toSource()\n");
    v = Get(TEXT_toSource, d_string_hash(TEXT_toSource));
    if (!v)
	v = &vundefined;
    if (v->isPrimitive())
	return v->toSource();
    else	// it's an Object
    {   void *a;
	CallContext *cc;
	Dobject *o;
	Value *ret;
	Value val;

	o = v->object;
	cc = Program::getProgram()->callcontext;
	ret = &val;
	a = o->Call(cc, this->object, ret, 0, NULL);
	if (a)			// if exception was thrown
	{
	    /*return a*/;
	    WPRINTF(L"Vobject::toSource() failed with %x\n", a);
	}
	else if (ret->isPrimitive())
	    return ret->toString();
    }
    return TEXT_undefined;
}

Dobject *Vobject::toObject()
{
    return object;
}

int Vobject::isPrimitive()
{
    return FALSE;
}

unsigned Vobject::getHash()
{
    return (unsigned) object;	// this returns the address of the object.
				// since the object never moves, it will work
				// as its hash.
}

Value *Vobject::Put(d_string PropertyName, Value *value)
{
    return object->Put(PropertyName, value, 0);
}

Value *Vobject::Put(d_uint32 index, Value *value)
{
    return object->Put(index, value, 0);
}

Value *Vobject::Get(d_string PropertyName, unsigned hash)
{
    return object->Get(PropertyName, hash);
}

Value *Vobject::Get(d_string PropertyName)
{
    return object->Get(PropertyName);
}

Value *Vobject::Get(d_uint32 index)
{
    return object->Get(index);
}

int Vobject::compareTo(Value *v)
{
#if ISTYPE
    if (v->__istype(Vobject))
#else
    if(v->getType() == TypeObject)
#endif
    {
	if (v->object == object)
	    return 0;
    }
    return -1;
}

dchar *Vobject::getType()
{
    return TypeObject;
}

d_string Vobject::getTypeof()
{
    return object->getTypeof();
}

void *Vobject::Construct(CONSTRUCT_ARGS)
{
    return object->Construct(cc, ret, argc, arglist);
}

void *Vobject::Call(CALL_ARGS)
{
    void *a;

    a = object->Call(cc, othis, ret, argc, arglist);
    //if (a) WPRINTF(L"Vobject::Call() returned %x\n", a);
    return a;
}

Value *Vobject::putIterator(Value *v)
{
    return object->putIterator(v);
}

void Vobject::getErrInfo(ErrInfo *perrinfo, int linnum)
{
    object->getErrInfo(perrinfo, linnum);
}


/*************************** Date ***********************/

Vdate Vdate::tmp(0);

#if 0
void Vdate::putValue(Value *v, d_number n)
{
    Vdate tmp(n);

    Value::copy(v, &tmp);
}
#endif

#if 0 // inline'd
Vdate::Vdate(d_number number)
{
    this->number = number;
}
#endif

d_boolean Vdate::toBoolean()
{
    return !(number == 0.0 || Port::isnan(number));
}

d_number Vdate::toNumber()
{
    // Convert from VT_DATE to Date format
    return Date::VarDateToTime(number);
}

d_string Vdate::toString()
{
    Dobject *d;

    d = toObject();
    Vobject vo(d);

    return vo.toString();
}

Dobject *Vdate::toObject()
{
    GC_LOG();
    return new Ddate(toNumber());
}

unsigned Vdate::getHash()
{
    return HASH((unsigned) number);
}

int Vdate::compareTo(Value *v)
{
    if (v->isDate())
    {
	if (number == v->number)
	    return 0;
	else if (number > v->number)
	    return 1;
    }
    return -1;
}

#if !ISTYPE && !VPTR_HACK
int Vdate::isDate()
{
    return TRUE;
}
#endif

dchar *Vdate::getType()
{
    return TypeDate;
}

d_string Vdate::getTypeof()
{
    return TEXT_date;
}


/*************************** VBArray ***********************/

#if 0
Vvbarray Vvbarray::tmp(0);

d_boolean Vvbarray::toBoolean()
{
    return !(safearray == NULL);
}

d_number Vvbarray::toNumber()
{
    return Port::nan;
}

d_string Vvbarray::toString()
{
    Dobject *d;

    d = toObject();
    Vobject vo(d);

    return vo.toString();
}

Dobject *Vvbarray::toObject()
{
    GC_LOG();
    return Dvbarray_ctor(this);
}

unsigned Vvbarray::getHash()
{
    return HASH((unsigned) safearray);
}

int Vvbarray::compareTo(Value *v)
{
    if (v->isVBArray())
    {
	if (safearray == v->safearray)
	    return 0;
    }
    return -1;
}

#if !ISTYPE && !VPTR_HACK
int Vvbarray::isVBArray()
{
    return TRUE;
}
#endif

dchar *Vvbarray::getType()
{
    return TypeVBArray;
}

d_string Vvbarray::getTypeof()
{
    return TEXT_unknown;
}

#endif
