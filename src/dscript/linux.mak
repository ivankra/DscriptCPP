##
## linux.mak
##
## Copyright (C) 2001-2002 Chromium Communications Corporation
## All Rights Reserved
##
## Written by Walter Bright
##

#############################################################
#	Change these numbers to rev release values
#
BUILDVERSION := $(shell date +%Y%m%d%H)
#
#############################################################

CC=g++

ROOT_DIR= ../root

GC_DIR= ../dmgc
GC_LIB=../dmgc/dmgc.a

CHILICOM_DIR= ../../chilicom_sdk
CHILICOM_LIB_DIR= $(CHILICOM_DIR)/lib/linux2_debug

FLAGS= -DUNICODE

CHILI_FLAGS= \
	-DBUILD_VERSION=$(BUILDVERSION) \
	-D_XOPEN_SOURCE=500 -D_XOPEN_SOURCE_EXTENDED=1 -D_BSD_SOURCE=1 \
	-D_POSIX_C_SOURCE=199903 -D_POSIX_PTHREAD_SEMANTICS=1 \
	-D_GNU_SOURCE=1 -D_REENTRANT=1 -DTHREAD_SAFE \
	-DMEMCHECK_DISABLED -DSTACKTRACE_DISABLED -DEVENTLOG_ASSERTIONS \
	-D_WIN32

#OPT=-g -g3
OPT=-O2

MINI_CFLAGS= $(OPT) -Wall -D_GNU_SOURCE -I$(ROOT_DIR) -I$(GC_DIR)

CFLAGS= $(FLAGS) $(MINI_CFLAGS) $(CHILI_FLAGS) -fPIC -Wno-non-virtual-dtor \
    -DCSMODULE=dscript -D_THREAD_SAFE \
    -DDISABLE_OBJECT_SAFETY -DDISABLE_EVENT_HANDLING \
    -I$(ROOT_DIR) -I$(CHILICOM_DIR)/include 

LFLAGS= -nodefaultlibs -shared -L$(CHILICOM_LIB_DIR)

EXETARGET=dscript
DLLTARGET=libdscript.so

LIBS= \
    ds.a $(ROOT_DIR)/rutabega.a $(GC_LIB) -lpthread -ldl 

GCCLIBS= -lm -lc -lgcc

SLIBS= -lkernel -ladvapi -lole -loleaut

GENERATED_FILES= text.h text.c syntaxerror.c rangeerror.c urierror.c referenceerror.c typeerror.c evalerror.c

# Disable garbage collection by including in $(ROOT)mem.o
OBJS= \
    identifier.o lexer.o program.o response.o \
    parse.o expression.o statement.o \
    symbol.o scope.o value.o ir.o \
    toir.o opcodes.o dobject.o dfunction.o \
    darray.o dboolean.o dglobal.o \
    dstring2.o dnumber.o dmath.o \
    dstring.o ddate.o property.o \
    dregexp.o function.o iterator.o \
    darguments.o derror.o syntaxerror.o \
    rangeerror.o urierror.o referenceerror.o \
    typeerror.o evalerror.o \
    text.o optimize.o threadcontext.o \
    dsobjdispatch.o

# These .obj's are additional for the dll version ds.so
DLLOBJS= init.o register.o dsfactory.o \
    dcomobject.o denumerator.o dvbarray.o variant.o \
    OLEScript.o NamedItemDisp.o \
    BaseComObject.o BaseDispatch.o \
    ParsedProcedure.o BaseDispatchEx.o \
    ActiveScriptError.o dcomlambda.o \
    varFormats.o comerr.o

TOTALH= $(ROOT_DIR)/root.h dscript.h lexer.h parse.h symbol.h expression.h \
    statement.h scope.h program.h text.h

defaulttarget: $(EXETARGET) $(DLLTARGET)

release:
	make -f linux.mak clean
	make -f linux.mak dscript
	make -f linux.mak clean

$(EXETARGET) : $(OBJS) $(LIBS) dscript.o
	$(CC) $(CFLAGS) -static -Wl,-Map,$(EXETARGET).map -o $@ dscript.o $(LIBS)

$(DLLTARGET) : $(DLLOBJS) $(LIBS)
	$(CC) $(CFLAGS) $(LFLAGS) -Wl,-Map,$(DLLTARGET).map -o $@ $(DLLOBJS) $(LIBS) $(SLIBS) $(GCCLIBS)

ds.a: $(OBJS)
	ar -rc $@ $(OBJS)	

################# Source file dependencies ###############

darguments.o : $(TOTALH) value.h dobject.h darguments.c
darray.o : $(TOTALH) value.h dobject.h darray.c
dboolean.o : $(TOTALH) value.h dobject.h dboolean.c
ddate.o : $(TOTALH) value.h ../root/dateparse.h dobject.h ddate.c
dfunction.o : $(TOTALH) value.h dobject.h dfunction.c
dglobal.o : $(TOTALH) value.h dobject.h dregexp.h dglobal.c
dmath.o : $(TOTALH) value.h dobject.h dmath.c
dnumber.o : $(TOTALH) value.h dobject.h dnumber.c
dobject.o : $(TOTALH) value.h property.h dregexp.h dobject.h dobject.c
dregexp.o : $(TOTALH) value.h property.h dregexp.h dobject.h dregexp.c
dscript.o : $(TOTALH) program.h dscript.h dscript.c
dshell.o : $(TOTALH) program.h dshell.h dshell.cpp
dstring.o : $(TOTALH) value.h dobject.h dstring.c
dstring2.o : $(TOTALH) value.h dobject.h dstring2.c
expression.o : $(TOTALH) expression.h expression.c
function.o : $(TOTALH) ir.h dobject.h identifier.h value.h property.h function.c
identifier.o : $(TOTALH) identifier.h identifier.c
ir.o : $(TOTALH) ir.h ir.c
iterator.o : $(TOTALH) dobject.h iterator.h iterator.c
lexer.o : $(TOTALH) lexer.c
opcodes.o : $(TOTALH) value.h ir.h opcodes.c
optimize.o : $(TOTALH) ir.h optimize.c
parse.o : $(TOTALH) lexer.h parse.h parse.c
program.o : $(TOTALH) program.h program.c
property.o : $(TOTALH) value.h property.h property.c
response.o : $(TOTALH) dscript.h response.c
scope.o : $(TOTALH) scope.h scope.c
statement.o : $(TOTALH) statement.h statement.c
symbol.o : $(TOTALH) identifier.h symbol.h symbol.c
text.o : text.h text.c
toir.o : $(TOTALH) expression.h ir.h toir.c
value.o : $(TOTALH) value.h value.c

referenceerror.o : referenceerror.c
rangeerror.o : rangeerror.c
syntaxerror.o : syntaxerror.c
typeerror.o : typeerror.c
urierror.o : urierror.c

CPPH=OLECommon.h OLEScript.h dsobjdispatch.h NamedItemDisp.h BaseDispatch.h \
	EventHandlers.h BaseComObject.h ActiveScriptError.h ParsedProcedure.h \
	BaseDispatchEx.h

# DLL .o's
comerr.o : $(CPPH) text.h comerr.cpp
dcomobject.o : $(CPPH) dcomobject.cpp dcomobject.h dcomlambda.h
denumerator.o : $(CPPH) denumerator.cpp dcomobject.h
dvbarray.o : $(CPPH) dvbarray.cpp dcomobject.h
variant.o : $(CPPH) varFormats.h variant.cpp
init.o : $(CPPH) text.h init.cpp
register.o : $(CPPH) register.cpp
dsfactory.o : $(CPPH) dsfactory.cpp text.h
varFormats.o : varFormats.h varFormats.cpp

dcomlambda.o : $(CPPH) dcomlambda.cpp dcomlambda.h dcomobject.h
dsobjdispatch.o : $(CPPH) dsobjdispatch.cpp dcomobject.h
NamedItemDisp.o : $(CPPH) NamedItemDisp.cpp
#EventHandlers.o : $(CPPH) EventHandlers.cpp
OLEScript.o : $(CPPH) OLEScript.cpp
BaseComObject.o : $(CPPH) BaseComObject.cpp
BaseDispatch.o : $(CPPH) BaseDispatch.cpp
ActiveScriptError.o : $(CPPH) ActiveScriptError.cpp
ParsedProcedure.o : $(CPPH) ParsedProcedure.cpp

################### Utilities ################

evalerror.c : format protoerror.c
	./format protoerror.c $*.c $* EvalError

rangeerror.c : format protoerror.c
	./format protoerror.c $*.c $* RangeError

referenceerror.c : format protoerror.c
	./format protoerror.c $*.c $* ReferenceError

syntaxerror.c : format protoerror.c
	./format protoerror.c $*.c $* SyntaxError

typeerror.c : format protoerror.c
	./format protoerror.c $*.c $* TypeError

urierror.c : format protoerror.c
	./format protoerror.c $*.c $* URIError

# ---------------------------------------------
#  textgen -- needed to make text.h and text.c
# ---------------------------------------------

text.h text.c : textgen
	./textgen

textgen.o : textgen.c

textgen : textgen.o
	$(CC) -o $@ textgen.o

# ---------------------------------------------
#  format -- needed to make error sources
# ---------------------------------------------

format: format.c
	$(CC) $(MINI_CFLAGS) -o format format.c $(ROOT_DIR)/printf.c $(ROOT_DIR)/root.c $(ROOT_DIR)/mem.c


# ---------------------------------------------

clean:
	-rm -f $(EXETARGET)
	-rm -f $(EXETARGET).map
	-rm -f $(DLLTARGET)
	-rm -f $(DLLTARGET).map
	-rm -f ds.a
	-rm -f *.o
	-rm -f textgen
	-rm -f format
	-rm -f $(GENERATED_FILES)

################### Suffix Rules ################

.c.o :
	$(CC) -c $(CFLAGS) $<

.cpp.o :
	$(CC) -c $(CFLAGS) $<


