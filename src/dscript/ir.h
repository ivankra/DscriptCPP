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


#ifndef IR_H
#define IR_H

// Opcodes for our Intermediate Representation (IR)

enum IROPCODE
{
    IRerror,
    IRnop,			// no operation
    IRend,			// end of function
    IRstring,
    IRthisget,
    IRnumber,
    IRobject,
    IRthis,
    IRnull,
    IRundefined,
    IRboolean,
    IRcall,
    IRcalls = IRcall + 1,
    IRcallscope = IRcalls + 1,
    IRcallv = IRcallscope + 1,
    IRputcall,
    IRputcalls = IRputcall + 1,
    IRputcallscope = IRputcalls + 1,
    IRputcallv = IRputcallscope + 1,
    IRget,
    IRgets = IRget + 1,		// 's' versions must be original + 1
    IRgetscope = IRgets + 1,
    IRput,
    IRputs = IRput + 1,
    IRputscope = IRputs + 1,
    IRdel,
    IRdels = IRdel + 1,
    IRdelscope = IRdels + 1,
    IRnext,
    IRnexts = IRnext + 1,
    IRnextscope = IRnexts + 1,
    IRaddass,
    IRaddasss = IRaddass + 1,
    IRaddassscope = IRaddasss + 1,
    IRputthis,
    IRputdefault,
    IRmov,
    IRret,
    IRretexp,
    IRimpret,
    IRneg,
    IRpos,
    IRcom,
    IRnot,
    IRadd,
    IRsub,
    IRmul,
    IRdiv,
    IRmod,
    IRshl,
    IRshr,
    IRushr,
    IRand,
    IRor,
    IRxor,

    IRpreinc,
    IRpreincs = IRpreinc + 1,
    IRpreincscope = IRpreincs + 1,

    IRpredec,
    IRpredecs = IRpredec + 1,
    IRpredecscope = IRpredecs + 1,

    IRpostinc,
    IRpostincs = IRpostinc + 1,
    IRpostincscope = IRpostincs + 1,

    IRpostdec,
    IRpostdecs = IRpostdec + 1,
    IRpostdecscope = IRpostdecs + 1,

    IRnew,

    IRclt,
    IRcle,
    IRcgt,
    IRcge,
    IRceq,
    IRcne,
    IRcid,
    IRcnid,

    IRjt,
    IRjf,
    IRjtb,
    IRjfb,
    IRjmp,

    IRjlt,		// commonly appears as loop control
    IRjle,		// commonly appears as loop control

    IRjltc,		// commonly appears as loop control
    IRjlec,		// commonly appears as loop control

    IRtypeof,
    IRinstance,

    IRpush,
    IRpop,

    IRiter,
    IRassert,

    IRthrow,
    IRtrycatch,
    IRtryfinally,
    IRfinallyret,

    IRMAX
};

struct Value;
struct Dobject;
struct Statement;

struct IR
{
    union
    {
	struct
	{
	    unsigned char opcode;
	    unsigned char padding;
	    unsigned short linnum;
	} op;
	//enum IROPCODE opcode;
	IR *code;
	Value *value;
	unsigned index;		// index into local variable table
	unsigned hash;		// cached hash value
#if (defined __DMC__) || (defined __GNUC__) || _MSC_VER
	#define INDEX_FACTOR 16
#else
	#define INDEX_FACTOR 1
#endif
	int offset;
	d_string string;
	d_boolean boolean;
	Statement *target;	// used for backpatch fixups
	Dobject *object;
	void *ptr;
    };

    static void *call(CallContext *cc, Dobject *othis, IR *code, Value *ret, Value *locals);
    static void print(unsigned address, IR *code);
    static void printfunc(IR *code);
    static unsigned size(unsigned opcode);
    static unsigned verify(unsigned linnum, IR *code);
};


struct Block;
struct Statement;
struct ScopeStatement;

// The state of the interpreter machine as seen by the code generator, not
// the interpreter.

struct IRstate
{
    Block *current;
    OutBuffer *codebuf;			// accumulate code here
    Statement *breakTarget;		// current statement that 'break' applies to
    Statement *continueTarget;		// current statement that 'continue' applies to
    ScopeStatement *scopeContext;	// current ScopeStatement we're inside
    Array fixups;

    IRstate();
    void next();	// close out current Block, and start a new one

    unsigned locali;
    unsigned nlocals;
    unsigned alloc(unsigned nlocals);
    void release(unsigned local, unsigned n);

    unsigned mark();
    void release(unsigned i);

    void gen0(Loc loc, unsigned opcode);
    void gen1(Loc loc, unsigned opcode, unsigned arg);
    void gen2(Loc loc, unsigned opcode, unsigned arg1, unsigned arg2);
    void gen3(Loc loc, unsigned opcode, unsigned arg1, unsigned arg2, unsigned arg3);
    void gen4(Loc loc, unsigned opcode, unsigned arg1, unsigned arg2, unsigned arg3, unsigned arg4);
    void gen(Loc loc, unsigned opcode, unsigned argc, ...);
    void pops(unsigned npops);
    unsigned getIP();
    void patchJmp(unsigned index, unsigned value);
    void addFixup(unsigned index);
    void doFixups();
    void optimize();
};

#endif
