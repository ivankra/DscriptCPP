# Dscript makefile
# Copyright (C) 1999-2002 by Digital Mars
# All Rights Reserved
# www.digitalmars.com
# Written by Walter Bright

# Build with Digital Mars C++:
#	make -f dmc.mak

CC=sc


ROOT=..\root
GC=..\dmgc
OUTDIR=.
DCHAR=-DUNICODE
#DCHAR=

# Debug build
CFLAGS=-I$(ROOT);$(GC) -D -g $(DCHAR)
LFLAGS=/ma/co

# Release build
#CFLAGS=-I$(ROOT);$(GC) -o $(DCHAR)
#LFLAGS=/ma

# Makerules:
.c.obj :
	 $(CC) -c $(CFLAGS) -cpp -Aa -wx -6 $*.c

.cpp.obj :
	 $(CC) -c $(CFLAGS) -I\dm\include;\dscript\vcinc -Aa -Ae -w6 -6 $*.cpp

defaulttarget: ds.exe dscript.dll

################ RELEASES #########################

release:
	make clean
	make dscript
	make clean

################ NT COMMAND LINE RELEASE #########################


################ NT COMMAND LINE DEBUG #########################

debdscript:
	make OPT= "DEBUG=-D -g" LFLAGS= dscript

#########################################

OBJS= identifier.obj lexer.obj program.obj response.obj parse.obj expression.obj \
	statement.obj symbol.obj scope.obj value.obj ir.obj toir.obj opcodes.obj \
	dobject.obj dfunction.obj darray.obj dboolean.obj dglobal.obj dstring2.obj \
	dnumber.obj dmath.obj dstring.obj ddate.obj property.obj dregexp.obj \
	function.obj iterator.obj darguments.obj derror.obj syntaxerror.obj \
	rangeerror.obj urierror.obj referenceerror.obj \
	typeerror.obj evalerror.obj dsobjdispatch.obj \
	text.obj optimize.obj threadcontext2.obj dnative.obj

# These .obj's are additional for the dll version ds.dll
DLLOBJS= $(OUTDIR)\init.obj $(OUTDIR)\register.obj $(OUTDIR)\dsfactory.obj \
    $(OUTDIR)\dcomobject.obj $(OUTDIR)\EventHandlers.obj       \
    $(OUTDIR)\OLEScript.obj $(OUTDIR)\NamedItemDisp.obj        \
    $(OUTDIR)\BaseComObject.obj $(OUTDIR)\BaseDispatch.obj     \
    $(OUTDIR)\ParsedProcedure.obj $(OUTDIR)\BaseDispatchEx.obj \
    $(OUTDIR)\ActiveScriptError.obj $(OUTDIR)\dcomlambda.obj   \
    $(OUTDIR)\denumerator.obj $(OUTDIR)\dvbarray.obj $(OUTDIR)\variant.obj \
    $(OUTDIR)\dsobjdispatch.obj $(OUTDIR)\initguid.obj $(OUTDIR)\comerr.obj

LIBS= ds.lib rutabega.lib $(GC)\dmgc.lib

SRC1= dscript.c trace.c rutabega.c iterator.h dnative.c
SRC2= symbol.h format.c scope.h builtin.c identifier.c
SRC3= identifier.h symbol.c darguments.c ir.c expression.h dregexp.h
SRC4= protoerror.c derror.c ir.h parse.h toir.c program.h lexer.h dmath.c
SRC5= dobject.h dboolean.c dcomobject.h function.h dstring2.c scope.c value.h
SRC6= function.c dglobal.c statement.h iterator.c lexer.c dregexp.c property.h
SRC7= ddate.c text.h text.c dfunction.c dnumber.c dstring.c dobject.c
SRC8= opcodes.c optimize.c parse.c expression.c program.c statement.c
SRC9= value.c darray.c dscript.h property.c threadcontext2.c dmc_rpcndr.h

# Sources just for .DLL version
SRCD1= dsactiveparse.h dsactiveparseprocedure.h dsactivescript.h dsdispatch.h
SRCD2= dsfactory.h dsobjectsafety.h register.h dswrapper.h dsruntime.h dsevents.h
SRCD3= register.cpp dcomobject.cpp dsactivescript.cpp dsdispatch.cpp dsobjdispatch.h
SRCD4= dsactiveparseprocedure.cpp dsactiveparse.cpp dsevents.cpp dsobjectsafety.cpp
SRCD5= dsruntime.cpp dswrapper.cpp init.cpp dsfactory.cpp initguid.cpp dsobjdispatch.cpp
SRCD6= printf.cpp denumerator.cpp comerr.cpp


############### Link Command Line ##########################

ds.exe : dscript.obj $(LIBS) dmc.mak
	sc -ods.exe dscript.obj -cpp -L$(LFLAGS) $(LIBS)

############### Link DLL ##########################

dscript.dll : $(DLLOBJS) $(LIBS) dmc.def dmc.mak
	sc -odscript.dll $(DLLOBJS) -cpp -L$(LFLAGS) dmc.def $(LIBS) \
		kernel32.lib user32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib

############### Lib ##########################

ds.lib : $(OBJS) dmc.mak
	del ds.lib
	lib ds /c/noi/pagesize:512 +identifier+lexer+program+response+parse+expression;
	lib ds /noi +statement+symbol+scope+value+ir+toir+opcodes;
	lib ds /noi +dobject+dfunction+darray+dboolean+dglobal+dstring2;
	lib ds /noi +dnumber+dmath+dstring+ddate+property+dregexp;
	lib ds /noi +function+iterator+darguments+derror+syntaxerror;
	lib ds /noi +rangeerror+urierror+referenceerror+typeerror+evalerror;
	lib ds /noi +text+optimize+threadcontext2+dnative;


##################### INCLUDE MACROS #####################

TOTALH= $(ROOT)\root.h dscript.h lexer.h parse.h symbol.h expression.h \
	statement.h scope.h program.h text.h

##################### SPECIAL BUILDS #####################

################# Source file dependencies ###############

darguments.obj : $(TOTALH) value.h dobject.h darguments.c
darray.obj : $(TOTALH) value.h dobject.h darray.c
dboolean.obj : $(TOTALH) value.h dobject.h dboolean.c
ddate.obj : $(TOTALH) value.h ../root/dateparse.h dobject.h ddate.c
dfunction.obj : $(TOTALH) value.h dobject.h dfunction.c
dglobal.obj : $(TOTALH) value.h dobject.h dregexp.h dglobal.c
dmath.obj : $(TOTALH) value.h dobject.h dmath.c
dnative.obj : $(TOTALH) value.h dobject.h dnative.c
dnumber.obj : $(TOTALH) value.h dobject.h dnumber.c
dobject.obj : $(TOTALH) value.h property.h dregexp.h dobject.h dobject.c
dregexp.obj : $(TOTALH) value.h property.h dregexp.h dobject.h dregexp.c
dscript.obj : $(TOTALH) program.h dscript.h dscript.c
dstring.obj : $(TOTALH) value.h dobject.h dstring.c
dstring2.obj : $(TOTALH) value.h dobject.h dstring2.c
expression.obj : $(TOTALH) expression.h expression.c
format.obj : format.c
function.obj : $(TOTALH) ir.h dobject.h identifier.h value.h property.h function.c
identifier.obj : $(TOTALH) identifier.h identifier.c
ir.obj : $(TOTALH) ir.h ir.c
iterator.obj : $(TOTALH) dobject.h iterator.h iterator.c
lexer.obj : $(TOTALH) lexer.c
opcodes.obj : $(TOTALH) value.h ir.h opcodes.c
optimize.obj : $(TOTALH) ir.h optimize.c
parse.obj : $(TOTALH) lexer.h parse.h parse.c
program.obj : $(TOTALH) program.h program.c
property.obj : $(TOTALH) value.h property.h property.c
response.obj : $(ROOT)/root.h dscript.h response.c
rutabega.obj : $(ROOT)/root.h $(ROOT)/root.c $(ROOT)/stringtable.h $(ROOT)/stringtable.c
scope.obj : $(TOTALH) scope.h scope.c
statement.obj : $(TOTALH) statement.h statement.c
symbol.obj : $(TOTALH) identifier.h symbol.h symbol.c
text.obj : text.h text.c
toir.obj : $(TOTALH) expression.h ir.h toir.c
value.obj : $(TOTALH) value.h value.c

$(OUTDIR)\referenceerror.obj : referenceerror.c
$(OUTDIR)\rangeerror.obj : rangeerror.c
$(OUTDIR)\syntaxerror.obj : syntaxerror.c
$(OUTDIR)\typeerror.obj : typeerror.c
$(OUTDIR)\urierror.obj : urierror.c

CPPH=OLECommon.h OLEScript.h dsobjdispatch.h NamedItemDisp.h BaseDispatch.h \
	EventHandlers.h BaseComObject.h ActiveScriptError.h ParsedProcedure.h \
	BaseDispatchEx.h

# DLL .obj's
$(OUTDIR)\comerr.obj : $(CPPH) comerr.cpp
$(OUTDIR)\dcomobject.obj : $(CPPH) dcomobject.cpp dcomobject.h dcomlambda.h
$(OUTDIR)\denumerator.obj : $(CPPH) denumerator.cpp dcomobject.h
$(OUTDIR)\dvbarray.obj : $(CPPH) dvbarray.cpp dcomobject.h
$(OUTDIR)\variant.obj : $(CPPH) variant.cpp
$(OUTDIR)\init.obj : $(CPPH) init.cpp
$(OUTDIR)\register.obj : $(CPPH) register.cpp
$(OUTDIR)\dsfactory.obj : $(CPPH) dsfactory.cpp

$(OUTDIR)\dcomlambda.obj : $(CPPH) dcomlambda.cpp dcomlambda.h dcomobject.h
$(OUTDIR)\dsobjdispatch.obj : $(CPPH) dsobjdispatch.cpp dcomobject.h
$(OUTDIR)\NamedItemDisp.obj : $(CPPH) NamedItemDisp.cpp
$(OUTDIR)\EventHandlers.obj : $(CPPH) EventHandlers.cpp
$(OUTDIR)\OLEScript.obj : $(CPPH) OLEScript.cpp
$(OUTDIR)\BaseComObject.obj : $(CPPH) BaseComObject.cpp
$(OUTDIR)\BaseDispatch.obj : $(CPPH) BaseDispatch.cpp
$(OUTDIR)\ActiveScriptError.obj : $(CPPH) ActiveScriptError.cpp
$(OUTDIR)\ParsedProcedure.obj : $(CPPH) ParsedProcedure.cpp
$(OUTDIR)\initguid.obj : initguid.cpp

$(OUTDIR)\dscript.res : dscript.rc


################### Utilities ################

evalerror.c : format.exe protoerror.c
	format protoerror.c $*.c $* EvalError

referenceerror.c : format.exe protoerror.c
	format protoerror.c $*.c $* ReferenceError

rangeerror.c : format.exe protoerror.c
	format protoerror.c $*.c $* RangeError

syntaxerror.c : format.exe protoerror.c
	format protoerror.c $*.c $* SyntaxError

typeerror.c : format.exe protoerror.c
	format protoerror.c $*.c $* TypeError

urierror.c : format.exe protoerror.c
	format protoerror.c $*.c $* URIError

format.exe : format.obj
	sc format.obj $(ROOT)\rutabega.lib $(GC)\dmgc.lib


####

text.h text.c : $(OUTDIR)\textgen.exe
	$(OUTDIR)\textgen

$(OUTDIR)\textgen.obj : textgen.c

$(OUTDIR)\textgen.exe : $(OUTDIR)\textgen.obj
	sc $(OUTDIR)\textgen.obj -o$(OUTDIR)\textgen.exe

################### Utilities ################

clean:
	del *.obj

makefile : dmc.mak
	copy dmc.mak makefile

zip : dmc.mak msvc.mak linux.mak
	zip32 -u dscript dmc.mak msvc.mak linux.mak *.ds
	zip32 -u dscript $(SRC1)
	zip32 -u dscript $(SRC2)
	zip32 -u dscript $(SRC3)
	zip32 -u dscript $(SRC4)
	zip32 -u dscript $(SRC5)
	zip32 -u dscript $(SRC6)
	zip32 -u dscript $(SRC7)
	zip32 -u dscript $(SRC8)
	zip32 -u dscript $(SRC9)
	zip32 -u dscript $(SRCD1)
	zip32 -u dscript $(SRCD2)
	zip32 -u dscript $(SRCD3)
	zip32 -u dscript $(SRCD4)
	zip32 -u dscript $(SRCD5)
	zip32 -u dscript $(SRCD6)

ds.zip : dmc.mak
	zip32 -u ds dmc.mak
	zip32 -u ds $(SRC1)
	zip32 -u ds $(SRC2)
	zip32 -u ds $(SRC3)
	zip32 -u ds $(SRC4)
	zip32 -u ds $(SRC5)
	zip32 -u ds $(SRC6)
	zip32 -u ds $(SRC7)
	zip32 -u ds $(SRC8)
	zip32 -u ds $(SRC9)

###################################
