/* Digital Mars DMDScript source code.
 * Copyright (c) 2001-2007 by Digital Mars
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

#include <string.h>

#if __DMC__
// Inline bit operations
#include <bitops.h>
#endif

#ifdef linux
#include "gccbitops.h"
#endif

#if _MSC_VER
    // Disable useless warnings about unreferenced functions
    #pragma warning (disable : 4514)
#endif // _MSC_VER


#define BITS_PER_WORD	32
#define BITS_SHIFT	5
#define BITS_MASK	31

struct Mem;

struct GCBits
{
    unsigned *data;
    unsigned nwords;	// allocated words in data[] excluding sentinals
    unsigned nbits;	// number of bits in data[] excluding sentinals

    GCBits();
    ~GCBits();
    void invariant();

    void alloc(unsigned nbits);

#if __DMC__
    unsigned test(unsigned i) { return _inline_bt(data + 1, i); }
    void set(unsigned i)      { _inline_bts(data + 1, i); }
    void clear(unsigned i)    { _inline_btr(data + 1, i); }
    unsigned testClear(unsigned i) { return _inline_btr(data + 1, i); }
    unsigned testSet(unsigned i) { return _inline_bts(data + 1, i); }
#elif 0 //defined linux
    // for unknown reasons, GCC does badly with this
    unsigned test(unsigned i) { return _inline_bt(data + 1, i); }
    void set(unsigned i)      { _inline_bts(data + 1, i); }
    void clear(unsigned i)    { _inline_btr(data + 1, i); }
    unsigned testClear(unsigned i) { return _inline_btr(data + 1, i); }
    unsigned testSet(unsigned i) { return _inline_bts(data + 1, i); }
#else
    unsigned test(unsigned i) { return data[1 + (i >> BITS_SHIFT)] & (1 << (i & BITS_MASK)); }
    void set(unsigned i)      { data[1 + (i >> BITS_SHIFT)] |= (1 << (i & BITS_MASK)); }
    void clear(unsigned i)    { data[1 + (i >> BITS_SHIFT)] &= ~(1 << (i & BITS_MASK)); }
    unsigned testClear(unsigned i)
    {
	unsigned *p = &data[1 + (i >> BITS_SHIFT)];
	unsigned mask = (1 << (i & BITS_MASK));
	unsigned result = *p & mask;
	*p &= ~mask;
	return result;
    }
    unsigned testSet(unsigned i)
    {
	unsigned *p = &data[1 + (i >> BITS_SHIFT)];
	unsigned mask = (1 << (i & BITS_MASK));
	unsigned result = *p & mask;
	*p |= mask;
	return result;
    }
#endif

    void zero() { memset(data + 1, 0, nwords * sizeof(unsigned)); }
    void copy(GCBits *f) { memcpy(data + 1, f->data + 1, nwords * sizeof(unsigned)); }

    unsigned *base() { return data + 1; }
};
