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


#ifndef PROPERTY_H
#define PROPERTY_H

struct Property;
struct Value;

#include "value.h"

struct PropTable
{
    Array roots;	// Array of Property's
    Property *start;
    Property *end;
    PropTable *previous;

    PropTable();
    ~PropTable();

    Property *getProperty(d_string name);

    Value *get(Value *key, unsigned hash);
    Value *get(d_uint32 index);
    Value *get(d_string name, unsigned hash);

    Value *put(Value *key, unsigned hash, Value *value, unsigned attributes);
    Value *put(d_string name, Value *value, unsigned attributes);
    Value *put(d_uint32 index, Value *value, unsigned attributes);
    Value *put(d_uint32 index, d_string string, unsigned attributes);

    int canput(Value *key, unsigned hash);
    int canput(d_string name);

    int hasownproperty(Value *key, int enumerable);
    int hasproperty(Value *key);
    int hasproperty(d_string name);

    int del(Value *key);
    int del(d_string name);
    int del(d_uint32 index);

    void balance(unsigned i);
    void rehash(unsigned length);
};

struct Property
{
    Property *left;
    Property *right;

    Property *next;
    Property *prev;

    unsigned attributes;
    #define ReadOnly       0x001
    #define DontEnum       0x002
    #define DontDelete     0x004
    #define Internal       0x008
    #define Deleted        0x010
    #define Locked         0x020
    #define DontOverride   0x040
    #define KeyWord        0x080
    #define DebugFree      0x100        // for debugging help
    #define Instantiate    0x200        // For COM named item namespace support

    unsigned hash;
    Value key;
    Value value;

    // Storage allocation
    static void *operator new(size_t sz, Mem *mem);
    void free();

    // Rebalance
    unsigned count();
    void fill(Property ***p);
};

#endif
