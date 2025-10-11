# Dscript makefile
# Copyright (C) 1999-2001 by Chromium Communications
# All Rights Reserved
# Written by Walter Bright

# Build with Microsoft VC:
#	nmake -f msvc.mak

!IF !DEFINED(NULL)
!IF DEFINED(OS) && "$(OS)" == "Windows_NT"
NULL=
!ELSE
NULL="NUL"
!ENDIF
!ENDIF

CC=cl
RC=rc.exe
BSC=bscmake.exe /nologo

ROOT=..\root
#GC=..\hbgc2
GC=..\dmgc
DCHAR=-DUNICODE
DEFINES= $(DCHAR)

!IF DEFINED(DISABLE_WINDOWS_STUFF)
DEFINES=$(DEFINES) -DDISABLE_EVENT_HANDLING -DDISABLE_OBJECT_SAFETY
!ENDIF

CPPFLAGS= -I$(ROOT) -I$(GC) /W4 /GR /G6 /Gy /GX-
LFLAGS=/DEBUG /DEBUGTYPE:CV /PDBTYPE:CON

# Debug version
!IF !DEFINED(BUILD_CONFIG) || "$(BUILD_CONFIG)" == "DEBUG" || "$(BUILD_CONFIG)" == "debug"
OUTDIR=Debug
DEFAULTLIBFLAG= /MDd
DEFINES= $(DEFINES) /D_DEBUG /DDEBUG=1
CPPFLAGS= $(CPPFLAGS) /ZI $(DEFAULTLIBFLAG) 

!ELSE

# Release version
DEFAULTLIBFLAG= /MD
CPPFLAGS= $(CPPFLAGS) /Zi $(DEFAULTLIBFLAG) /Ox 
OUTDIR=Release
LFLAGS=$(LFLAGS) /OPT:REF /OPT:ICF,2

!ENDIF

TARGET=ds

!IF "$(GENERATE_COD_FILES)" == "1"
CPPFLAGS=$(CPPFLAGS) /FAcs /Fa$*.cod
!ENDIF

!IF "$(GENERATE_BSC_FILES)" == "1"
CPPFLAGS=$(CPPFLAGS) /FR"$(OUTDIR)/"
!ENDIF


# Makerules:
# (should compile with /W4)
.c{$(OUTDIR)}.obj:
 $(CC) /c $(CPPFLAGS) /Fd$(OUTDIR)\dscript.pdb $(DEFINES) /TP /Fo$@ $<
!IF "$(GENERATE_BSC_FILES)" == "1"
 -$(BSC) /o $(OUTDIR)\dscript.bsc $*.sbr
!ENDIF

.cpp{$(OUTDIR)}.obj:
 $(CC) /c $(CPPFLAGS) /Fd$(OUTDIR)\dscript.pdb $(DEFINES) /TP /Fo$@ $<
!IF "$(GENERATE_BSC_FILES)" == "1"
 -$(BSC) /nologo /o $(OUTDIR)\dscript.bsc $*.sbr
!ENDIF

.rc{$(OUTDIR)}.res:
 $(RC) $(DEFINES) /Fo$@ $<

defaulttarget: $(OUTDIR) $(OUTDIR)\ds.exe $(OUTDIR)\dscript.dll

$(OUTDIR):
 -@IF NOT EXIST "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

################ NT COMMAND LINE RELEASE #########################


################ NT COMMAND LINE DEBUG #########################

debdscript:
	make OPT= "DEBUG=-D -g" LFLAGS= dscript

#########################################

# These .obj's are for the main dscript engine ds.lib
OBJS1= $(OUTDIR)\identifier.obj $(OUTDIR)\lexer.obj $(OUTDIR)\program.obj $(OUTDIR)\response.obj
OBJS2= $(OUTDIR)\parse.obj $(OUTDIR)\expression.obj $(OUTDIR)\statement.obj
OBJS3= $(OUTDIR)\symbol.obj $(OUTDIR)\scope.obj $(OUTDIR)\value.obj $(OUTDIR)\ir.obj
OBJS4= $(OUTDIR)\toir.obj $(OUTDIR)\opcodes.obj $(OUTDIR)\dobject.obj $(OUTDIR)\dfunction.obj
OBJS5= $(OUTDIR)\darray.obj $(OUTDIR)\dboolean.obj $(OUTDIR)\dglobal.obj
OBJS6= $(OUTDIR)\dstring2.obj $(OUTDIR)\dnumber.obj $(OUTDIR)\dmath.obj
OBJS7= $(OUTDIR)\dstring.obj $(OUTDIR)\ddate.obj $(OUTDIR)\property.obj $(OUTDIR)\dnative.obj
OBJS8= $(OUTDIR)\dregexp.obj $(OUTDIR)\function.obj $(OUTDIR)\iterator.obj
OBJS9= $(OUTDIR)\darguments.obj $(OUTDIR)\derror.obj $(OUTDIR)\syntaxerror.obj
OBJS10= $(OUTDIR)\rangeerror.obj $(OUTDIR)\urierror.obj $(OUTDIR)\referenceerror.obj
OBJS11= $(OUTDIR)\typeerror.obj $(OUTDIR)\evalerror.obj $(OUTDIR)\dsobjdispatch.obj
# Disable garbage collection by including in $(ROOT)\$(OUTDIR)\mem.obj
OBJS12= $(OUTDIR)\text.obj $(OUTDIR)\optimize.obj $(OUTDIR)\threadcontext2.obj

OBJS= $(OBJS1) $(OBJS2) $(OBJS3) $(OBJS4) $(OBJS5) $(OBJS6) $(OBJS7) $(OBJS8) \
    $(OBJS9) $(OBJS10) $(OBJS11) $(OBJS12)

# These .obj's are additional for the dll version ds.dll
DLLOBJS= $(OUTDIR)\init.obj $(OUTDIR)\register.obj $(OUTDIR)\dsfactory.obj          \
    $(OUTDIR)\dcomobject.obj $(OUTDIR)\EventHandlers.obj                            \
    $(OUTDIR)\OLEScript.obj $(OUTDIR)\NamedItemDisp.obj        \
    $(OUTDIR)\BaseComObject.obj $(OUTDIR)\BaseDispatch.obj                          \
    $(OUTDIR)\ParsedProcedure.obj $(OUTDIR)\BaseDispatchEx.obj                      \
    $(OUTDIR)\ActiveScriptError.obj $(OUTDIR)\dcomlambda.obj                        \
    $(OUTDIR)\denumerator.obj $(OUTDIR)\dvbarray.obj $(OUTDIR)\variant.obj \
    $(OUTDIR)\comerr.obj

!IF "$(OVERRIDE_GC)" == "1" && "$(OUTDIR)" == "Debug"

DLLOBJS= $(DLLOBJS) $(ROOT)\$(OUTDIR)\mem.obj

!ENDIF

LIBS= $(OUTDIR)\ds.lib $(ROOT)\$(OUTDIR)\rutvc.lib $(GC)\$(OUTDIR)\dmgcvc.lib

############### Link Command Line ##########################

$(OUTDIR)\ds.exe : $(OBJS) $(LIBS) $(OUTDIR)\dscript.obj msvc.mak
	$(CC) $(DEFAULTLIBFLAG) /Feds.exe $(OUTDIR)\dscript.obj -link /pdb:$*.pdb /out:$@ /map:$*.map $(LIBS) oleaut32.lib user32.lib $(LFLAGS)

############### Link DLL ##########################

$(OUTDIR)\dscript.dll : $(LIBS) $(DLLOBJS) $(OUTDIR)\dscript.res dscript.def msvc.mak
	link $(LFLAGS) /dll /def:dscript.def /map:$*.map /PDB:$*.pdb /out:$@ $(LIBS) $(DLLOBJS) $(OUTDIR)\dscript.res \
		kernel32.lib user32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib

############### Lib ##########################

$(OUTDIR)\ds.lib : $(OBJS) msvc.mak _libcmd.rsp
	lib /DEBUGTYPE:cv /out:$@ @_libcmd.rsp
	del _libcmd.rsp

_libcmd.rsp : msvc.mak $(OBJS)
	echo $(OBJS1) >   _libcmd.rsp
	echo $(OBJS2) >>  _libcmd.rsp
	echo $(OBJS3) >>  _libcmd.rsp
	echo $(OBJS4) >>  _libcmd.rsp
	echo $(OBJS5) >>  _libcmd.rsp
	echo $(OBJS6) >>  _libcmd.rsp
	echo $(OBJS7) >>  _libcmd.rsp
	echo $(OBJS8) >>  _libcmd.rsp
	echo $(OBJS9) >>  _libcmd.rsp
	echo $(OBJS10) >>  _libcmd.rsp
	echo $(OBJS11) >>  _libcmd.rsp
	echo $(OBJS12) >>  _libcmd.rsp

##################### INCLUDE MACROS #####################

TOTALH= $(ROOT)/root.h dscript.h lexer.h parse.h symbol.h expression.h \
	statement.h scope.h program.h text.h

##################### SPECIAL BUILDS #####################

################# Source file dependencies ###############

$(OUTDIR)\darguments.obj : $(TOTALH) value.h dobject.h darguments.c
$(OUTDIR)\darray.obj : $(TOTALH) value.h dobject.h darray.c
$(OUTDIR)\dboolean.obj : $(TOTALH) value.h dobject.h dboolean.c
$(OUTDIR)\ddate.obj : $(TOTALH) value.h ../root/dateparse.h dobject.h ddate.c
$(OUTDIR)\dfunction.obj : $(TOTALH) value.h dobject.h dfunction.c
$(OUTDIR)\dglobal.obj : $(TOTALH) value.h dobject.h dregexp.h dglobal.c
$(OUTDIR)\dmath.obj : $(TOTALH) value.h dobject.h dmath.c
$(OUTDIR)\dnative.obj : $(TOTALH) value.h dobject.h dnative.c
$(OUTDIR)\dnumber.obj : $(TOTALH) value.h dobject.h dnumber.c
$(OUTDIR)\dobject.obj : $(TOTALH) value.h property.h dregexp.h dobject.h dobject.c
$(OUTDIR)\dregexp.obj : $(TOTALH) value.h property.h dregexp.h dobject.h dregexp.c
$(OUTDIR)\dscript.obj : $(TOTALH) program.h dscript.h dscript.c
$(OUTDIR)\dstring.obj : $(TOTALH) value.h dobject.h dstring.c
$(OUTDIR)\dstring2.obj : $(TOTALH) value.h dobject.h dstring2.c
$(OUTDIR)\expression.obj : $(TOTALH) expression.h expression.c
$(OUTDIR)\function.obj : $(TOTALH) ir.h dobject.h identifier.h value.h property.h function.c
$(OUTDIR)\identifier.obj : $(TOTALH) identifier.h identifier.c
$(OUTDIR)\ir.obj : $(TOTALH) ir.h ir.c
$(OUTDIR)\iterator.obj : $(TOTALH) dobject.h iterator.h iterator.c
$(OUTDIR)\lexer.obj : $(TOTALH) lexer.c
$(OUTDIR)\opcodes.obj : $(TOTALH) value.h ir.h opcodes.c
$(OUTDIR)\optimize.obj : $(TOTALH) ir.h optimize.c
$(OUTDIR)\parse.obj : $(TOTALH) lexer.h parse.h parse.c
$(OUTDIR)\program.obj : $(TOTALH) program.h program.c
$(OUTDIR)\property.obj : $(TOTALH) value.h property.h property.c
$(OUTDIR)\response.obj : $(ROOT)/root.h dscript.h response.c
$(OUTDIR)\rutabega.obj : $(ROOT)/root.h $(ROOT)/root.c $(ROOT)/stringtable.h $(ROOT)/stringtable.c
$(OUTDIR)\scope.obj : $(TOTALH) scope.h scope.c
$(OUTDIR)\statement.obj : $(TOTALH) statement.h statement.c
$(OUTDIR)\symbol.obj : $(TOTALH) identifier.h symbol.h symbol.c
$(OUTDIR)\text.obj : text.h text.c
$(OUTDIR)\toir.obj : $(TOTALH) expression.h ir.h toir.c
$(OUTDIR)\value.obj : $(TOTALH) value.h value.c

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

$(OUTDIR)\dscript.res : dscript.rc


################### Utilities ################

evalerror.c : $(OUTDIR)\format.exe protoerror.c
	$(OUTDIR)\format protoerror.c $@ evalerror EvalError

referenceerror.c : $(OUTDIR)\format.exe protoerror.c
	$(OUTDIR)\format protoerror.c $@ referenceerror ReferenceError

rangeerror.c : $(OUTDIR)\format.exe protoerror.c
	$(OUTDIR)\format protoerror.c $@ rangeerror RangeError

syntaxerror.c : $(OUTDIR)\format.exe protoerror.c
	$(OUTDIR)\format protoerror.c $@ syntaxerror SyntaxError

typeerror.c : $(OUTDIR)\format.exe protoerror.c
	$(OUTDIR)\format protoerror.c $@ typeerror TypeError

urierror.c : $(OUTDIR)\format.exe protoerror.c
	$(OUTDIR)\format protoerror.c $@ urierror URIError

$(OUTDIR)\format.obj : format.c $(ROOT)/root.h

$(OUTDIR)\format.exe : $(OUTDIR)\format.obj
    echo $*
	$(CC) $(DEFAULTLIBFLAG) $(OUTDIR)\format.obj /link /out:$@ $(ROOT)\$(OUTDIR)\rutvc.lib $(GC)\$(OUTDIR)\dmgcvc.lib
#	copy $(GC)\$(OUTDIR)\gc.dll $(OUTDIR)
#	copy $(GC)\$(OUTDIR)\gc.pdb $(OUTDIR)



####

text.h text.c : $(OUTDIR)\textgen.exe
	$(OUTDIR)\textgen

$(OUTDIR)\textgen.obj : textgen.c

$(OUTDIR)\textgen.exe : $(OUTDIR)\textgen.obj
	cl $(OUTDIR)\textgen.obj /link /out:$@

################### Utilities ################

clean:
 -@echo Cleaning $(BUILD_CONFIG)
 -@IF EXIST "$(OUTDIR)/$(NULL)" delnode /q "$(OUTDIR)"
 -@IF EXIST evalerror.c del evalerror.c
 -@IF EXIST referenceerror.c del referenceerror.c
 -@IF EXIST rangeerror.c del rangeerror.c
 -@IF EXIST syntaxerror.c del syntaxerror.c
 -@IF EXIST typeerror.c del typeerror.c
 -@IF EXIST urierror.c del urierror.c
 -@IF EXIST text.c del text.c
 -@IF EXIST text.h del text.h

zip : dmc.mak msvc.mak
	zip32 -u dscript dmc.mak msvc.mak *.h *.c *.cpp *.ds dscript.def

###################################
