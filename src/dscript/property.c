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
#include <stddef.h>
#include <string.h>
#include <assert.h>

#if _MSC_VER
#include <malloc.h>
#endif

#include "gc.h"
#include "root.h"
#include "mem.h"
#include "value.h"
#include "property.h"
#include "text.h"
#include "dobject.h"

#define FREELIST	0	// don't use for multithreaded versions

/*********************************** PropTable *********************/

PropTable::PropTable()
{
    start = NULL;
    end = NULL;
    previous = NULL;
}

PropTable::~PropTable()
{
    unsigned i;

    for (i = 0; i < roots.dim; i++)
    {	Property *p;

	p = (Property *)roots.data[i];
	if (p)
	{
	    p->free();			// add to Property::freelist
	}
    }
    roots.zero();			// aid GC by zeroing pointers
    start = NULL;
    end = NULL;
    previous = NULL;
}

/*******************************
 * Look up name and get its corresponding Property.
 * Return NULL if not found.
 */

Property *PropTable::getProperty(d_string name)
{
    Value *v;
    Property *p;

    v = get(name, Vstring::calcHash(name));
    if (!v)
	return NULL;

    // Work backwards from &p->value to p
    p = (Property *)((char *)v - offsetof(Property, value));

    return p;
}

Value *PropTable::get(Value *key, unsigned hash)
{
    unsigned i;
    Property *p;
    PropTable *t;

    //PRINTF("get: hash = %d\n", hash);
    //assert(key->getHash() == hash);
    t = this;
    do
    {
	//PRINTF("PropTable::get(%p, dim = %d)\n", this, t->roots.dim);
	if (t->roots.dim)
	{
	    i = hash % t->roots.dim;
	    p = (Property *)t->roots.data[i];
	    while (p)
	    {	int c;

		c = hash - p->hash;
		if (c == 0)
		{
		    c = key->compareTo(&p->key);
		    if (c == 0)
			return &p->value;
		}
		if (c < 0)
		    p = p->left;
		else
		    p = p->right;
	    }
	}
	t = t->previous;
    } while (t);
    return NULL;			// not found
}

Value *PropTable::get(d_uint32 index)
{
    //PRINTF("PropTable::get(%p, %d)\n", this, index);
#if 1
    // Inline for speed
    unsigned i;
    Property *p;
    PropTable *t;
    unsigned hash = HASH(index);

    //PRINTF("get: hash = %d\n", hash);
    //assert(key->getHash() == hash);
    t = this;
    do
    {
	//PRINTF("PropTable::get(%p, dim = %d)\n", this, t->roots.dim);
	if (t->roots.dim)
	{
	    i = hash % t->roots.dim;
	    p = (Property *)t->roots.data[i];
	    while (p)
	    {	int c;

		c = hash - p->hash;
		if (c == 0)
		{
		    // Emulate Vnumber::compareTo
		    // c = key->compareTo(&p->key);
		    {
			Value *v = &p->key;
			if (v->isNumber())
			{
			    if (index == v->number)
				return &p->value;
			    else if (index > v->number)
				c = 1;
			}
			else if (v->isString())
			{   Vnumber key(index);
			    c = Lstring::cmp(key.toString(), v->x.string);
			}
			else
			    c = -1;
		    }
		    if (c == 0)
			return &p->value;
		}
		if (c < 0)
		    p = p->left;
		else
		    p = p->right;
	    }
	}
	t = t->previous;
    } while (t);
    return NULL;			// not found
#else
    Vnumber key(index);
    return get(&key, HASH(index));
#endif
}

Value *PropTable::get(d_string name, unsigned hash)
{
    //WPRINTF(L"PropTable::get(%p, '%ls', hash = x%x)\n", this, d_string_ptr(name), hash);
    unsigned i;
    Property *p;
    PropTable *t;

    t = this;
    do
    {
	//PRINTF("PropTable::get(%p, dim = %d)\n", t, t->roots.dim);
	if (t->roots.dim)
	{
	    i = hash % t->roots.dim;
	    p = (Property *)t->roots.data[i];
	    while (p)
	    {	int c;

		c = hash - p->hash;
		if (c == 0)
		{
		    // Match logic of Vstring::compareTo(&p->key);
#if defined __DMC__
		    if (p->key.isString())
		    {
			if (name == p->key.x.string)
			    return &p->value;
			c = d_string_cmp(name, p->key.x.string);
		    }
		    else if (p->key.isNumber())
		    {
			c = d_string_cmp(name, p->key.toString());
		    }
		    else
			c = -1;
#else
		    // Force GCC to do virtual call
		    Value *v = &p->key;
		    if (v->isString())
		    {
			if (name == p->key.x.string)
			    return &p->value;
			c = d_string_cmp(name, p->key.x.string);
		    }
		    else if (v->isNumber())
		    {
			c = d_string_cmp(name, (&p->key)->toString());
		    }
		    else
			c = -1;
#endif

		    if (c == 0)
			return &p->value;
		}
		if (c < 0)
		    p = p->left;
		else
		    p = p->right;
	    }
	}
	t = t->previous;
    } while (t);
    return NULL;			// not found
}

/*******************************
 * Determine if property exists for this object.
 * The enumerable flag means the DontEnum attribute cannot be set.
 */

int PropTable::hasownproperty(Value *key, int enumerable)
{
    unsigned i;
    Property *p;
    unsigned hash = key->getHash();

    //PRINTF("hasownproperty: hash = %d\n", hash);
    //assert(key->getHash() == hash);
    //PRINTF("PropTable::hasownproperty(%p, dim = %d)\n", this, roots.dim);
    if (roots.dim)
    {
	i = hash % roots.dim;
	p = (Property *)roots.data[i];
	while (p)
	{   int c;

	    c = hash - p->hash;
	    if (c == 0)
	    {
		c = key->compareTo(&p->key);
		if (c == 0)
		{
		    return (!enumerable || !(p->attributes & DontEnum));
		}
	    }
	    if (c < 0)
		p = p->left;
	    else
		p = p->right;
	}
    }
    return FALSE;			// not found
}

int PropTable::hasproperty(Value *key)
{
    Value *v;

    v = get(key, key->getHash());
    return (v != NULL);
}

int PropTable::hasproperty(d_string name)
{
    return get(name, Vstring::calcHash(name)) != NULL;
}

Value *PropTable::put(Value *key, unsigned hash, Value *value, unsigned attributes)
{
    unsigned i;
    Property *p;
    Property **pp;

//    WPRINTF(L"PropTable::put(this=%x, %x, %x)\n", this, key, value);
//    fflush(stdout);
//    WPRINTF(L"PropTable::put(key = '%ls', value = '%ls', attributes = x%x)\n",
//	d_string_ptr(key->toString()), d_string_ptr(value->toString()), attributes);
//    WPRINTF(L"PropTable::put(key = '%ls', attributes = x%x)\n",
//	d_string_ptr(key->toString()), attributes);
//    PRINTF("put: hash = %d\n", hash);
    if (!roots.dim)
    {
	//WPRINTF(L"\tsetting %x to 16\n", this);
	GC_LOG();
	roots.setDim(64/4);
	roots.zero();
    }
    //PRINTF("PropTable::put(%p, dim = %d)\n", this, roots.dim);
    //assert(key->getHash() == hash);
    i = hash % roots.dim;
    pp = (Property **)&roots.data[i];
    while (*pp)
    {	int c;

	p = *pp;
	c = hash - p->hash;
	if (c == 0)
	{
	    c = key->compareTo(&p->key);
	    if (c == 0)
	    {
		if (attributes & DontOverride ||
		    p->attributes & ReadOnly)
		{
		    if (p->attributes & KeyWord)
			return NULL;
		    return &vundefined;
		}

		if (previous && previous->canput(key, hash) == FALSE)
		{
		    p->attributes |= ReadOnly;
		    return &vundefined;
		}

		// Overwrite property with new value
		Value::copy(&p->value, value);
		p->attributes = (attributes & ~DontOverride) | (p->attributes & (DontDelete | DontEnum));
		return NULL;
	    }
	}
	if (c < 0)
	    pp = &p->left;
	else
	    pp = &p->right;
    }

    // Not in table; create new entry

    // This ugly hack relies on the fact that the only place we use
    // PropTable is as the first entry of Dobject. Dobject inherits from
    // Mem, which is what we want.
    // We want Mem because in it is cached the thread specific storage
    // allocator, and we feel the need for speed.
#ifdef __GCC__
    Mem *mymem = (Mem *)((char *)this - offsetof(Dobject, proptable));
#else
    Mem *mymem = (Mem *)((char *)this - offsetof(Dobject, proptable) + offsetof(Dobject, gc));
#endif

    //printf("mymem = %p, mymem->gc = %p, %x, %x\n", mymem, mymem->gc, offsetof(Dobject, gc), offsetof(Dobject, proptable));

    GC_LOG();
    p = new(mymem) Property();
    memset(p, 0, sizeof(Property));
    p->hash = hash;
    p->attributes = attributes & ~DontOverride;
    Value::copy(&p->key,   key);
    Value::copy(&p->value, value);

    // Link into threaded list of all properties
#if 1
    if (end)
    {
	end->next = p;
	p->prev = end;
	end = p;
    }
    else
    {	start = p;
	end = p;
    }
#else
    p->next = start;
    p->prev = NULL;
    if (start)
    {	start->prev = p;
    }
    start = p;
#endif

    *pp = p;
    return NULL;			// success
}

Value *PropTable::put(d_string name, Value *value, unsigned attributes)
{
    Vstring key(name);

    //WPRINTF(L"PropTable::put(%p, '%ls', hash = x%x)\n", this, d_string_ptr(name), key.getHash());
    return put(&key, key.getHash(), value, attributes);
}

Value *PropTable::put(d_uint32 index, Value *value, unsigned attributes)
{
    Vnumber key(index);

    //PRINTF("PropTable::put(%d)\n", index);
#if 1
    unsigned i;
    Property *p;
    Property **pp;
    unsigned hash;
    if (!roots.dim)
    {
	GC_LOG();
	roots.setDim(64/4);
	roots.zero();
    }
    hash = HASH(index);
    i = hash % roots.dim;
    //printf("roots.dim = %d, hash = %d, i = %d\n", roots.dim, hash, i);
    pp = (Property **)&roots.data[i];
    while (*pp)
    {	int c;

	p = *pp;
	c = hash - p->hash;
	if (c == 0)
	{
#if 1
	    // Emulate Vnumber::compareTo()
	    Value *v = &p->key;
	    if (v->isNumber())
	    {
		if (index == v->number)
		{
		    Value::copy(&p->value, value);
		    return NULL;
		}
		else if (index > v->number)
		    c = 1;
	    }
	    else if (v->isString())
	    {
		c = Lstring::cmp(key.toString(), v->x.string);
	    }
	    else
		c = -1;
#else
	    c = key.compareTo(&p->key);
#endif
	    if (c == 0)
	    {
		// Overwrite property with new value
		Value::copy(&p->value, value);
		return NULL;
	    }
	}
	if (c < 0)
	    pp = &p->left;
	else
	    pp = &p->right;
    }

    // Not in table; create new entry

    // This ugly hack relies on the fact that the only place we use
    // PropTable is as the first entry of Dobject. Dobject inherits from
    // Mem, which is what we want.
    // We want Mem because in it is cached the thread specific storage
    // allocator, and we feel the need for speed.
#ifdef __GCC__
    Mem *mymem = (Mem *)((char *)this - offsetof(Dobject, proptable));
#else
    Mem *mymem = (Mem *)((char *)this - offsetof(Dobject, proptable) + offsetof(Dobject, gc));
#endif

    //printf("mymem = %p, mymem->gc = %p, %x, %x\n", mymem, mymem->gc, offsetof(Dobject, gc), offsetof(Dobject, proptable));

    GC_LOG();
    p = new(mymem) Property();

    memset(p, 0, sizeof(Property));
    //p->left = NULL;
    //p->right = NULL;
    //p->next = NULL;
    //p->prev = NULL;

    p->attributes = attributes & ~DontOverride;
    p->hash = hash;
    Value::copy(&p->key,   &key);
    Value::copy(&p->value, value);

    // Link into threaded list of all properties
    if (end)
    {
	end->next = p;
	p->prev = end;
	end = p;
    }
    else
    {	start = p;
	end = p;
    }

    *pp = p;
    return NULL;			// success
#else
    return put(&key, HASH(index), value, attributes);
#endif
}

Value *PropTable::put(d_uint32 index, d_string string, unsigned attributes)
{
    Vnumber key(index);
    Vstring value(string);

    return put(&key, HASH(index), &value, attributes);
}

int PropTable::canput(Value *key, unsigned hash)
{
    unsigned i;
    Property *p;
    Property **pp;
    PropTable *t;

    t = this;
    do
    {
	if (t->roots.dim)
	{
	    i = hash % t->roots.dim;
	    pp = (Property **)&t->roots.data[i];
	    while (*pp)
	    {	int c;

		p = *pp;
		c = hash - p->hash;
		if (c == 0)
		{
		    c = key->compareTo(&p->key);
		    if (c == 0)
		    {
			return (p->attributes & ReadOnly)
			    ? FALSE : TRUE;
		    }
		}
		if (c < 0)
		    pp = &p->left;
		else
		    pp = &p->right;
	    }
	}
	t = t->previous;
    } while (t);
    return TRUE;			// success
}

int PropTable::canput(d_string name)
{
    Vstring v(name);

    return canput(&v, v.getHash());
}

int PropTable::del(Value *key)
{
    unsigned hash;
    unsigned i;
    Property *p;
    Property **pp;

    //WPRINTF(L"PropTable::del('%ls')\n", d_string_ptr(key->toString()));
    if (roots.dim)
    {
	hash = key->getHash();
	i = hash % roots.dim;
	pp = (Property **)&roots.data[i];
	while (*pp)
	{   int c;

	    p = *pp;
	    c = hash - p->hash;
	    if (c == 0)
	    {
		c = key->compareTo(&p->key);
		if (c == 0)
		{
		    if (p->attributes & DontDelete)
			return FALSE;

		    // Remove p from linked list of all members
		    if (p->prev)
			p->prev->next = p->next;
		    else
			start = p->next;
		    if (p->next)
			p->next->prev = p->prev;
		    if (p == end)
			end = p->prev;

		    // Look for simple cases
		    if (p->left == NULL)
		    {
			if (p->right == NULL)
			{
			    p->free();
			    *pp = NULL;
			    return TRUE;
			}
			*pp = p->right;
			p->right = NULL;
			p->free();
			return TRUE;
		    }
		    else if (p->right == NULL)
		    {
			*pp = p->left;
			p->left = NULL;
			p->free();
			return TRUE;
		    }
		    else
		    {
			// Rebalance tree to effect the delete
			p->attributes |= Deleted;
			balance(i);
			p->left = NULL;
			p->right = NULL;
			p->free();
			return TRUE;
		    }
		}
	    }
	    if (c < 0)
		pp = &p->left;
	    else
		pp = &p->right;
	}
    }
    return TRUE;			// not found
}

int PropTable::del(d_string name)
{
    Vstring v(name);

    //WPRINTF(L"PropTable::del('%ls')\n", d_string_ptr(name));
    return del(&v);
}

int PropTable::del(d_uint32 index)
{
    Vnumber v(index);

    //PRINTF("PropTable::del(%d)\n", index);
    return del(&v);
}

void PropTable::rehash(unsigned length)
{
    Property *pr;
    Property *p;
    Property **pp;
    unsigned i;

    // Resize roots array
    if (length < 2)
	length = 2;
    roots.setDim(length);
    roots.fixDim();
    roots.zero();

    // Walk threaded list, inserting each property back into the tree
    for (pr = start; pr; pr = pr->next)
    {	unsigned hash;

	hash = pr->hash;
	i = hash % roots.dim;
	pp = (Property **)&roots.data[i];
	while (*pp)
	{   int c;

	    p = *pp;
	    c = hash - p->hash;
	    if (c == 0)
	    {
		c = pr->key.compareTo(&p->key);
		if (c == 0)
		    assert(0);		// should be unique
	    }
	    if (c < 0)
		pp = &p->left;
	    else
		pp = &p->right;
	}
	*pp = pr;
	pr->left = NULL;
	pr->right = NULL;
    }
}

/*********************************** Property *********************/

// If DEBUG is on, items in the freelist are marked with DebugFree attribute.
// Properties must never be in the tree and on the freelist at the same time.

//Property *Property::freelist = NULL;

//int propertycount;

void *Property::operator new(size_t size, Mem *mem)
{
    //WPRINTF(L"\tnew property %d\n", ++propertycount);
    return mem->malloc(size);
}

void Property::free()
{
#ifdef DEBUG
    assert(!(attributes & DebugFree));
#endif
    if (attributes & Locked)
	attributes |= Deleted;
    else
    {
	//WPRINTF(L"\tfree property %d\n", --propertycount);
	if (left)
	{   left->free();
	    left = NULL;
	}
	if (right)
	{   right->free();
	    right = NULL;
	}
	memset(this, 0, sizeof(Property));
#ifdef DEBUG
	attributes |= DebugFree;
#endif
    }
}

/**********************************
 * Count up number of entries in tree.
 */

unsigned Property::count()
{
    unsigned i;
    Property *p;

    //PRINTF("Property::count(): %p->left = %p, right = %p\n", this, left, right);
    //fflush(stdout);
#ifdef DEBUG
    assert(!(attributes & DebugFree));
    assert(this != left && this != right && (!left || left != right));
#endif
    i = 0;
    p = this;
    for (;;)			// do tail recursion
    {
	if (!(p->attributes & Deleted))
	    i++;
	if (p->left)
	    i += p->left->count();
	if (p->right)
	    p = p->right;
	else
	    break;
    }
    return i;
}

/*************************************
 * Fill array with sorted Property's.
 */

void Property::fill(Property ***pp)
{   Property *p;

    p = this;
    for (;;)
    {
	if (p->left)
	{   p->left->fill(pp);
	    p->left = NULL;
	}
	if (!(p->attributes & Deleted))
	    *(*pp)++ = p;
	if (p->right)
	{   Property *q = p;

	    p = p->right;
	    q->right = NULL;
	}
	else
	    break;
    }
}

/*************************************
 * Rebalance tree index i.
 */

    static void insert(Property **pp, Property **arr, unsigned dim)
    {   unsigned pivot;

	switch (dim)
	{
	    case 0:
		return;

	    case 1:
		assert(*pp == NULL);
		*pp = arr[0];
		break;

	    default:
		pivot = dim / 2;
		*pp = arr[pivot];
		insert(&(*pp)->left, arr, pivot);
		insert(&(*pp)->right, arr + pivot + 1, dim - pivot - 1);
		break;
	}
    }

void PropTable::balance(unsigned i)
{
    unsigned cnt;
    Property **pp;
    Property *p;

    //PRINTF("PropTable::balance(%d)\n", i);
    pp = (Property **)&roots.data[i];
    p = *pp;
    if (p)
    {	Property **arr;
	Property **t;

	// Build sorted array
	cnt = p->count();
	//PRINTF("cnt = %d\n", cnt);
	void *p1 = NULL;
	if (cnt >= 128 || (arr = (Property **)alloca(cnt * sizeof(Property *))) == NULL)
	{
	    p1 = mem.malloc(cnt * sizeof(Property *));
	    arr = (Property **)p1;
	}
	t = arr;
	p->fill(&t);

	// Rebuild the tree from the sorted array	
	insert(pp,arr,cnt);

	if (p1)
	    mem.free(p1);
    }
}

