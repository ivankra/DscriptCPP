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


#ifndef ROOT_H
#define ROOT_H

#define ARRAY_INVARIANT	0	// 1: turn on Array invariant checking

#include <stdlib.h>
#include <stdarg.h>

#if __SC__
#pragma once
#endif

#if defined __GNUC__
#define __cdecl
#endif

#include "dchar.h"
#include "mem.h"
#include "gc.h"

char *unicode2ascii(wchar_t *);
int unicodeIsAscii(wchar_t *);
char *unicode2ascii(wchar_t *, unsigned len);
int unicodeIsAscii(wchar_t *, unsigned len);
wchar_t *ascii2unicode(const char *s);

int bstrcmp(unsigned char *s1, unsigned char *s2);
char *bstr2str(unsigned char *b);
#if UNICODE
void error(const wchar_t *format, ...);
void warning(const wchar_t *format, ...);
void message(const wchar_t *format, ...);
int message2buf(wchar_t *buf, unsigned size, const wchar_t *format, ...);
#else
void error(const char *format, ...);
void warning(const char *format, ...);
void message(const char *format, ...);
int message2buf(char *buf, unsigned size, const char *format, ...);
#endif

#ifndef TYPEDEFS
#define TYPEDEFS

#if _MSC_VER
typedef __int64 longlong;
typedef unsigned __int64 ulonglong;
#else
typedef long long longlong;
typedef unsigned long long ulonglong;
#endif

#endif

// Random number generator
#ifdef NO_RANDOM_STATICS
#define RANDOM_ARGS unsigned long *pdsseed, unsigned long *pdsindex
#define RANDOM_PARAMS pdsseed,pdsindex
#else
#define RANDOM_ARGS
#define RANDOM_PARAMS
void seedrandom(long seed);
#endif

double randomd(RANDOM_ARGS);
longlong randomx(RANDOM_ARGS);
#define RANDOM_INDEX(seed) (seed ^ 0x55555555);

/*
 * Root of our class library.
 */

struct OutBuffer;
struct Array;
struct Mem;
struct GC;

struct Object
{
    Object() { }
    virtual ~Object() { }

    void * operator new(size_t m_size);
    void * operator new(size_t m_size, Mem *mymem);
    void * operator new(size_t m_size, GC *mygc);
    void operator delete(void *p);


    virtual int equals(Object *o);

    /**
     * Returns a hash code, useful for things like building hash tables of Objects.
     */
    virtual unsigned hashCode();

    /**
     * Return <0, ==0, or >0 if this is less than, equal to, or greater than obj.
     * Useful for sorting Objects.
     */
    virtual int compare(Object *obj);

    /**
     * Pretty-print an Object. Useful for debugging the old-fashioned way.
     */
    virtual void print();

    virtual char *toChars();
    virtual dchar *toDchars();
    virtual void toBuffer(OutBuffer *buf);

    /**
     * Marks pointers for garbage collector by calling mem.mark() for all pointers into heap.
     */
    virtual void mark();

    /**
     * Perform integrity check on Object.
     */
    virtual void invariant();
};

struct String : Object
{
    int ref;			// != 0 if this is a reference to someone else's string
    char *str;			// the string itself

    String(char *str, int ref = 1);

    ~String();

    static unsigned calcHash(const char *str, unsigned len);
    static unsigned calcHash(const char *str);
    unsigned hashCode();
    unsigned len();
    int equals(Object *obj);
    int compare(Object *obj);
    char *toChars();
    void print();
    void mark();
};

struct FileName : String
{
    FileName(char *str, int ref);
    FileName(char *path, char *name);
    unsigned hashCode();
    int equals(Object *obj);
    int compare(Object *obj);
    static int absolute(const char *name);
    static char *ext(const char *);
    char *ext();
    static char *name(const char *);
    char *name();
    static char *replaceName(char *path, char *name);

    static Array *splitPath(const char *path);
    static FileName *defaultExt(const char *name, const char *ext);
    static FileName *forceExt(const char *name, const char *ext);
    int equalsExt(const char *ext);

    void CopyTo(FileName *to);
    static char *searchPath(Array *path, char *name, int cwd);
    static int exists(const char *name);
};

struct File : Object
{
    int ref;			// != 0 if this is a reference to someone else's buffer
    unsigned char *buffer;	// data for our file
    unsigned len;		// amount of data in buffer[]
    void *touchtime;		// system time to use for file

    FileName *name;		// name of our file

    File(char *);
    File(FileName *);
    ~File();

    void mark();

    char *toChars();

    /* Read file, return !=0 if error
     */

    int read();

    /* Write file, either succeed or fail
     * with error message & exit.
     */

    void readv();

    /* Write file, return !=0 if error
     */

    int write();

    /* Write file, either succeed or fail
     * with error message & exit.
     */

    void writev();

    /* Return !=0 if file exists.
     *	0:	file doesn't exist
     *	1:	normal file
     *	2:	directory
     */

    /* Append to file, return !=0 if error
     */

    int append();

    /* Append to file, either succeed or fail
     * with error message & exit.
     */

    void appendv();

    /* Return !=0 if file exists.
     *	0:	file doesn't exist
     *	1:	normal file
     *	2:	directory
     */

    int exists();

    /* Given wildcard filespec, return an array of
     * matching File's.
     */

    static Array *match(char *);
    static Array *match(FileName *);

    // Compare file times.
    // Return	<0	this < f
    //		=0	this == f
    //		>0	this > f
    int File::compareTime(File *f);

    // Read system file statistics
    void File::stat();

    /* Set buffer
     */

    void setbuffer(void *buffer, unsigned len)
    {
	this->buffer = (unsigned char *)buffer;
	this->len = len;
    }

    void checkoffset(size_t offset, size_t nbytes);

    // Convert buffer to Unicode if it isn't already
    void toUnicode();
};

struct OutBuffer : Object
{
    unsigned char *data;
    unsigned offset;
    unsigned size;
    Mem mem;

    OutBuffer();
    ~OutBuffer();
    void *extractData();
    void mark();

    void reserve(unsigned nbytes)
    {
	//PRINTF("OutBuffer::reserve: size = %d, offset = %d, nbytes = %d\n", size, offset, nbytes);
	if (size - offset < nbytes)
	{
	    size = (offset + nbytes) * 2;
	    GC_LOG();
	    data = (unsigned char *)mem.realloc(data, size);
	}
    }

    void setsize(unsigned size);
    void reset();
    void write(const void *data, unsigned nbytes);
    void writebstring(unsigned char *string);
    void writestring(const char *string);
    void writedstring(const char *string);
    void writedstring(const wchar_t *string);
    void prependstring(char *string);
    void writenl();			// write newline
    void tab(unsigned level,unsigned sp=4);	// tab over sp*level
    void writeByte(unsigned b);
    void writebyte(unsigned b) { writeByte(b); }
    void writedchar(unsigned b);
    void prependbyte(unsigned b);
    void writeword(unsigned w);
    void write4(unsigned w);

    void write4n(unsigned w)		// no reserve check
    {	*(unsigned *)(this->data + offset) = w;
	offset += 4;			// assume unsigned is 4 bytes
    }

    void write(OutBuffer *buf);
    void write(Object *obj);
    void fill0(unsigned nbytes);
    void align(unsigned size);
    void vprintf(const char *format, va_list args);
    void printf(const char *format, ...);
#if defined(UNICODE)
    void vprintf(const wchar_t *format, va_list args);
    void printf(const wchar_t *format, ...);
#endif
    void bracket(char left, char right);
    void spread(unsigned offset, unsigned nbytes);
    char *toChars();
    char *extractString();
};

struct Array : Object
{
#if ARRAY_INVARIANT
    // Used to detect corruption
    unsigned signature;
    #define ARRAY_SIGNATURE 0xAAAAFFFF
    void invariant();
#else
    void invariant() { }
#endif
    unsigned dim;
    unsigned allocdim;
    void **data;

    inline Array();
    inline ~Array();
    char *toChars();

    // Returning 0 means success, !=0 means failure (out of memory)
    int reserve(unsigned nentries);
    int setDim(unsigned newdim);
    int fixDim();
    inline int push(const void *ptr);
    int shift(void *ptr);
    int insert(unsigned index, void *ptr);
    int append(Array *a);

    inline void *pop();
    void remove(unsigned i);
    inline void zero();
    void *tos();
    void sort();
};

Array::Array()
{
#if ARRAY_INVARIANT
    signature = ARRAY_SIGNATURE;
#endif
    data = NULL;
    dim = 0;
    allocdim = 0;
}

Array::~Array()
{
#if ARRAY_INVARIANT
    invariant();
    signature = 0;
#endif
    mem.free(data);
    data = NULL;
}

int Array::push(const void *ptr)
{
    invariant();
    if (reserve(1))
    {
	return 1;
    }
    data[dim++] = (void *)ptr;
    return 0;
}

void *Array::pop()
{
    invariant();
    return data[--dim];
}

void Array::zero()
{
    invariant();
    memset(data,0,dim * sizeof(data[0]));
}

struct Bits : Object
{
    unsigned bitdim;
    unsigned allocdim;
    unsigned *data;

    Bits();
    ~Bits();
    void mark();

    void resize(unsigned bitdim);

    void set(unsigned bitnum);
    void clear(unsigned bitnum);
    int test(unsigned bitnum);

    void set();
    void clear();
    void copy(Bits *from);
    Bits *clone();

    void sub(Bits *b);
};

#endif
