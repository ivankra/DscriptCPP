!IF !DEFINED(NULL)
!IF DEFINED(OS) && "$(OS)" == "Windows_NT"
NULL=
!ELSE
NULL="NUL"
!ENDIF
!ENDIF

CC=cl
#GC=..\hbgc2
GC=..\dmgc
GCLIB=dmgcvc.lib

DCHAR=-DUNICODE
#DCHAR=

CPPFLAGS= $(CPPFLAGS) $(DCHAR) /W4 /GR /Gy /TP /Fd$(OUTDIR)\rutvc.pdb /I$(GC)
LFLAGS=/DEBUG /DEBUGTYPE:CV /PDBTYPE:CON
LIBFLAGS=

# Debug settings
!IF !DEFINED(BUILD_CONFIG) || "$(BUILD_CONFIG)" == "DEBUG" || "$(BUILD_CONFIG)" == "debug"
OUTDIR=Debug
DEFAULTLIBFLAG= /MDd
CPPFLAGS= $(CPPFLAGS) $(DEFAULTLIBFLAG) /D_DEBUG /DDEBUG /ZI

!ELSE

# Release settings
DEFAULTLIBFLAG= /MD
CPPFLAGS= $(CPPFLAGS) $(DEFAULTLIBFLAG) /Zi /Ox
OUTDIR=Release
LFLAGS=$(LFLAGS) /OPT:REF /OPT:ICF,2

!ENDIF

!IF "$(GENERATE_COD_FILES)" == "1"
CPPFLAGS=$(CPPFLAGS) /FAcs /Fa$*.cod
!ENDIF

.c{$(OUTDIR)}.obj:
	$(CC) -c $(CPPFLAGS) /Fo$@ $<

targets : $(OUTDIR) \
    $(OUTDIR)\rutvc.lib \
    $(OUTDIR)\mem.obj \
    $(OUTDIR)\testregexp.exe \
    $(OUTDIR)\testmem.exe \
    $(OUTDIR)\testdate.exe \
    $(OUTDIR)\archive.exe
    

$(OUTDIR):
 -@IF NOT EXIST "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

OBJS1= $(OUTDIR)\date.obj $(OUTDIR)\dateparse.obj $(OUTDIR)\dchar.obj
OBJS2= $(OUTDIR)\dmgcmem.obj $(OUTDIR)\mutex.obj $(OUTDIR)\port.obj
OBJS3= $(OUTDIR)\root.obj $(OUTDIR)\stringtable.obj $(OUTDIR)\regexp.obj
OBJS4= $(OUTDIR)\random.obj $(OUTDIR)\thread.obj
OBJS5= $(OUTDIR)\lstring.obj $(OUTDIR)\printf.obj

OBJS= $(OBJS1) $(OBJS2) $(OBJS3) $(OBJS4) $(OBJS5)

SRC1= port.h port.c root.h root.c stringtable.h stringtable.c testdate.c
SRC2= mutex.h mutex.c mem.h mem.c hbgcmem.c dmgcmem.c archive.c dchar.c printf.h printf.c
SRC3= regexp.h regexp.c testregexp.c dchar.h random.c dateparse.h dateparse.c
SRC4= date.h date.c testmem.c shell.c thread.h thread.c lstring.h lstring.c
SRC5= dmc.mak
SRC6= msvc.mak

$(OUTDIR)\rutvc.lib : $(OBJS) msvc.mak $(OUTDIR)\_libcmd.rsp
	lib /out:$@ @$(OUTDIR)\_libcmd.rsp
	-del $(OUTDIR)\_libcmd.rsp

$(OUTDIR)\_libcmd.rsp : msvc.mak
	echo $(OBJS1) >  $(OUTDIR)\_libcmd.rsp
	echo $(OBJS2) >> $(OUTDIR)\_libcmd.rsp
	echo $(OBJS3) >> $(OUTDIR)\_libcmd.rsp
	echo $(OBJS4) >> $(OUTDIR)\_libcmd.rsp
	echo $(OBJS5) >> $(OUTDIR)\_libcmd.rsp

$(OUTDIR)\date.obj: dchar.h date.h date.c
$(OUTDIR)\dateparse.obj: dchar.h dateparse.h dateparse.c
$(OUTDIR)\dchar.obj: dchar.h dchar.c
$(OUTDIR)\dmgcmem.obj: mem.h dmgcmem.c
$(OUTDIR)\lstring.obj: root.h dchar.h lstring.h lstring.c
$(OUTDIR)\mem.obj: mem.h mem.c
$(OUTDIR)\mutex.obj: mutex.h mutex.c
$(OUTDIR)\port.obj: port.h port.c
$(OUTDIR)\printf.obj: printf.h printf.c
$(OUTDIR)\root.obj: root.h root.c
$(OUTDIR)\regexp.obj: root.h dchar.h regexp.h regexp.c
$(OUTDIR)\stringtable.obj: root.h lstring.h stringtable.h stringtable.c
$(OUTDIR)\testdate.obj : dchar.h date.h testdate.c
$(OUTDIR)\testregexp.obj : dchar.h regexp.h testregexp.c
$(OUTDIR)\thread.obj : thread.h thread.c

$(OUTDIR)\testregexp.exe : msvc.mak $(OUTDIR)\rutvc.lib $(OUTDIR)\testregexp.obj $(GC)\$(OUTDIR)\$(GCLIB)
	$(CC) $(DEFAULTLIBFLAG) $(OUTDIR)\testregexp.obj /link $(OUTDIR)\rutvc.lib $(GC)\$(OUTDIR)\$(GCLIB) /out:$@

$(OUTDIR)\testmem.exe : msvc.mak $(OUTDIR)\rutvc.lib $(OUTDIR)\testmem.obj $(GC)\$(OUTDIR)\$(GCLIB)
	$(CC) $(DEFAULTLIBFLAG) $(OUTDIR)\testmem.obj /link $(OUTDIR)\rutvc.lib $(GC)\$(OUTDIR)\$(GCLIB) /out:$@

$(OUTDIR)\testdate.exe : msvc.mak $(OUTDIR)\rutvc.lib $(OUTDIR)\testdate.obj $(OUTDIR)\mem.obj
	$(CC) $(DEFAULTLIBFLAG) $(OUTDIR)\testdate.obj $(OUTDIR)\mem.obj /link $(OUTDIR)\rutvc.lib /out:$@

test : $(OUTDIR)\testregexp.exe
	testregexp

$(OUTDIR)\archive.exe : msvc.mak $(OUTDIR)\rutvc.lib $(OUTDIR)\archive.obj $(GC)\$(OUTDIR)\$(GCLIB)
	$(CC) $(DEFAULTLIBFLAG) $(OUTDIR)\archive.obj /link $(OUTDIR)\rutvc.lib $(GC)\$(OUTDIR)\$(GCLIB) /out:$@

$(OUTDIR)\shell.exe : msvc.mak $(OUTDIR)\rutvc.lib $(OUTDIR)\shell.obj
	$(CC) $(DEFAULTLIBFLAG) $(OUTDIR)\shell.obj $(OUTDIR)\mem.obj /link $(OUTDIR)\rutvc.lib /out:$@
	
clean:
    @echo Cleaning $(BUILD_CONFIG)
    -@IF EXIST "$(OUTDIR)/$(NULL)" delnode /q "$(OUTDIR)"

zip : $(SRC1) $(SRC2) $(SRC3) $(SRC4) $(SRC5) $(SRC6)
	zip32 -u root $(SRC1)
	zip32 -u root $(SRC2)
	zip32 -u root $(SRC3)
	zip32 -u root $(SRC4)
	zip32 -u root $(SRC5)
	zip32 -u root $(SRC6)

