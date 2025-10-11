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
#include <assert.h>

#include "root.h"
#include "dscript.h"
#include "value.h"
#include "property.h"
#include "iterator.h"
#include "dobject.h"

Iterator::Iterator(Dobject *o)
{
#ifdef DEBUG
    foo = ITERATOR_VALUE;
    //PRINTF("Iterator: o = %p, p = %p\n", o, p);
#endif
    ostart = o;
    this->o = o;
    p = o->proptable.start;
    if (p)
	p->attributes |= Locked;
    //PRINTF("Iterator: o = %p, p = %p\n", o, p);
}

#define INTERNAL_PROTOTYPE	1	// 0: use "prototype"
					// 1: use internal [[Prototype]]
Dobject *getPrototype(Dobject *o)
{
    Value *v;

    v = o->Get(TEXT_prototype);
    if (!v || v->isPrimitive())
	return NULL;
    o = v->toObject();
    return o;
}

int Iterator::done()
{
#ifdef DEBUG
    assert(foo == ITERATOR_VALUE);
#endif
    //PRINTF("Iterator::done() p = %p\n", p);

    if (p)
    {	Property *pn;

	p->attributes &= ~Locked;
	if (p->attributes & Deleted)	// if delayed delete
	{
	    pn = p->next;
	    p->free();
	    p = pn;
	}
    }

    for (; ; p = p->next)
    {
	while (!p)
	{
#if INTERNAL_PROTOTYPE
	    o = o->internal_prototype;	// get internal [[Prototype]]
#else
	    o = getPrototype(o);	// get "prototype"
#endif
	    if (!o)
		return TRUE;
	    p = o->proptable.start;
	}
	if (p->attributes & DontEnum)
	    continue;
	else
	{
	    // ECMA 12.6.3
	    // Do not enumerate those properties in prototypes
	    // that are overridden
	    if (o != ostart)
	    {
		PropTable *t;
		unsigned hash;
		Value *key;
		Property *pt;
		Dobject *ot;

		key = &p->key;
		hash = key->getHash();
#if INTERNAL_PROTOTYPE
		for (ot = ostart; ot != o; ot = ot->internal_prototype)
#else
		for (ot = ostart; ot != o; ot = getPrototype(ot))
#endif
		{
		    t = &ot->proptable;

		    // If property p is in t, don't enumerate
		    if (t->roots.dim)
		    {	unsigned i;

			i = hash % t->roots.dim;
			pt = (Property *)t->roots.data[i];
			while (pt)
			{
			    if (hash == pt->hash)
			    {   int c;

				c = key->compareTo(&pt->key);
				if (c == 0)
				    goto Lcontinue;
				else if (c < 0)
				    pt = pt->left;
				else
				    pt = pt->right;
			    }
			    else if (hash < pt->hash)
				pt = pt->left;
			    else
				pt = pt->right;
			}
		    }
		}
	    }
	    return FALSE;

	Lcontinue:
	    ;
	}
    }
}

Value *Iterator::next()
{
    Value *v;

#ifdef DEBUG
    assert(foo == ITERATOR_VALUE);
#endif
    //PRINTF("Iterator::next() p = %p\n", p);
    v = &p->key;
    //WPRINTF(L"v = '%s'\n", d_string_ptr(v->toString()));
    p = p->next;
    if (p)
	p->attributes |= Locked;
    return v;
}
