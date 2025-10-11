/* Digital Mars DMDScript source code.
 * Copyright (c) 2000-2001 by Chromium Communications
 * Copyright (c) 2004-2007 by Digital Mars
 * All Rights Reserved.
 * written by Walter Bright and Paul R. Nash
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

#ifndef __DCOMOBJECT_H__
#define __DCOMOBJECT_H__

struct IDispatchEx;

struct Dcomobject : Dobject
{
    int isLambda;	// 1 if Dcomlambda, 0 if Dcomobject

    Dcomobject(d_string name, IDispatch *pDisp, Dcomobject *pParent);
    ~Dcomobject();
    void finalizer();
#if INVARIANT
    void invariant();
#endif

    void *operator new(size_t m_size);

    // com interface
    IDispatch*  m_pDisp;
    IDispatchEx*m_pDispEx;  // if there is one, prefer it

    // object interface data
    d_string objectname; // item name

    // If this Dcomobject is a property of a parent Dcomobject, this
    // is the parent Dcomobject
    Dcomobject *m_pParent;

    Value *GetLambda(d_string PropertyName, unsigned hash);
    Value *Get(d_string PropertyName, unsigned hash);
    Value *Get(d_string PropertyName);
    Value *Get(d_uint32 index);
    Value *Put(d_string PropertyName, Value *value, unsigned attributes);
    Value *Put(d_string PropertyName, Dobject *o, unsigned attributes);
    Value *Put(d_string PropertyName, d_number n, unsigned attributes);
    Value *Put(d_string PropertyName, d_string s, unsigned attributes);
    Value *Put(d_uint32 index, Value *value, unsigned attributes);
    Value *PutDefault(Value *value);
    Value *put_Value(Value *ret, unsigned argc, Value *arglist);
    int CanPut(d_string PropertyName);
    int HasProperty(d_string PropertyName);
    int Delete(d_string PropertyName);
    int Delete(d_uint32 index);
    int implementsDelete();

    HRESULT _GetNameAndInvoke(LCID lcid, dchar* pwszName, DWORD grfdex,
                              WORD wInvokeFlags, DISPPARAMS* pDP,
                              VARIANT* pvarResult, EXCEPINFO* pEI);

    HRESULT _GetDispID(LCID lcid, dchar* pwszName, DWORD grfdex, DISPID* pID);

    void *DefaultValue(Value *ret, dchar *Hint);
    void *Construct(CONSTRUCT_ARGS);
    void *Call(CALL_ARGS);
    void *Call2(d_string s, CALL_ARGS);

    virtual int isDcomobject();
    virtual IDispatch* toComObject();

    static void init(ThreadContext *tc);
};

struct Denumerator : Dobject
{
    int isLambda;	// 1 if Dcomlambda, 0 if Dcomobject, 2 if Denumerator

    Denumerator(Value *v);
    Denumerator(Dobject *prototype);
    void finalizer();
    void *operator new(size_t m_size);
    void getNextItem();

    Value *vc;			// collection object
    IEnumVARIANT *mEnum;	// and vc's enumerator interface
    Value currentItem;
    int isItemCurrent;

    static Dfunction *getConstructor();
    static Dobject *getPrototype();
};

struct Dvariant : Dobject
{
    int isLambda;	// 1 if Dcomlambda, 0 if Dcomobject, 2 if Denumerator,
			// 3 if Dvariant

    Dvariant(Dobject *prototype, VARIANT *var);
    void finalizer();
    void *operator new(size_t m_size);
    d_string getTypeof();

    VARIANT variant;
};


struct Dvbarray : Dvariant
{
    Dvbarray(Value *v);
    Dvbarray(VARIANT *var);
    Dvbarray(Dobject *prototype);
    d_string getTypeof();

    static Dfunction *getConstructor();
    static Dobject *getPrototype();
};


void DoFinalize(void* pObj, void* pClientData);
void ValueToVariant(VARIANTARG *variant, Value *value);
HRESULT VariantToValue(Value *ret, VARIANTARG *variant, d_string PropertyName, Dcomobject *pParent);
int hresultToMsgnum(HRESULT hResult, int msgnum);

#endif // __DCOMOBJECT_H__

