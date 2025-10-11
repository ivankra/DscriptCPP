/* Digital Mars DMDScript source code.
 * Copyright (c) 2000-2001 by Chromium Communications
 * Copyright (c) 2004-2007 by Digital Mars
 * All Rights Reserved.
 * written by Paul R. Nash
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

#ifndef __DCOMLAMBDA_H__
#define __DCOMLAMBDA_H__

struct IDispatchEx;

struct Dcomlambda : Dobject
{
    int isLambda;	// 1 if Dcomlambda, 0 if Dcomobject

    Dcomlambda(DISPID id, IDispatch* pDisp, IDispatchEx* pDispEx, Dcomobject* pParent);
    ~Dcomlambda();
    void finalizer();

    void *operator new(size_t m_size);

    // COM interfaces on the object that owns this function
    IDispatch*  m_pDisp;
    IDispatchEx*m_pDispEx;  // if there is one, prefer it

    DISPID  m_idMember;

    // Dcomobject that is the parent of this lambda
    Dcomobject* m_pParent;

    Value* GetLambda(d_string PropertyName, unsigned hash)  { return NULL; }
    Value* Get(d_string PropertyName, unsigned hash)        { return NULL; }
    Value* Get(d_string PropertyName)                       { return NULL; }
    Value* Get(d_uint32 index)                              { return NULL; }
    Value* Put(d_string PropertyName, Value *value, unsigned attributes)    { return NULL; }
    Value* Put(d_string PropertyName, Dobject *o, unsigned attributes)      { return NULL; }
    Value* Put(d_string PropertyName, d_number n, unsigned attributes)      { return NULL; }
    Value* Put(d_string PropertyName, d_string s, unsigned attributes)      { return NULL; }
    Value* Put(d_uint32 index, Value *value, unsigned attributes)           { return NULL; }
    Value *put_Value(Value *ret, unsigned argc, Value *arglist);
    int CanPut(d_string PropertyName)       { return false; }
    int HasProperty(d_string PropertyName)  { return false; }
    int Delete(d_string PropertyName)       { return 0; }
    int Delete(d_uint32 index)              { return 0; }
    int implementsDelete()                  { return false; }

    void* Construct(CONSTRUCT_ARGS)         { return NULL; }
    void* Call(CALL_ARGS);

    static void init();
};

#endif // __DCOMLAMBDA_H__

