
# Copyright (C) 2000-2002 by Digital Mars
# All Rights Reserved
# www.digitalmars.com
# written by Walter Bright

DCHAR=-DUNICODE
#DCHAR=

CC=sc
GC=..\dmgc

#CFLAGS=-Ar -g -mn -cpp $(DCHAR)
CFLAGS=-o -cpp -6 $(DCHAR) -I$(GC)

.c.obj:
	$(CC) -c $(CFLAGS) $*

targets : rutabega.lib testregexp.exe testmem.exe testdate.exe archive.exe

OBJS= date.obj dateparse.obj dchar.obj \
	dmgcmem.obj mutex.obj port.obj \
	root.obj stringtable.obj regexp.obj \
	random.obj thread.obj \
	lstring.obj printf.obj

SRC1= port.h port.c root.h root.c stringtable.h stringtable.c testdate.c
SRC2= mutex.h mutex.c mem.h mem.c hbgcmem.c dmgcmem.c archive.c dchar.c printf.h printf.c
SRC3= regexp.h regexp.c testregexp.c dchar.h random.c dateparse.h dateparse.c
SRC4= date.h date.c testmem.c shell.c thread.h thread.c lstring.h lstring.c
SRC5= linux.mak dmc.mak
SRC6= msvc.mak

rutabega.lib : $(OBJS) dmc.mak
	del rutabega.lib
	lib rutabega /c/noi +date+dateparse+dchar;
	lib rutabega /noi +dmgcmem+mutex+port;
	lib rutabega /noi +root+stringtable+regexp;
	lib rutabega /noi +random+thread;
	lib rutabega /noi +lstring+printf;

date.obj: dchar.h date.h date.c
dateparse.obj: dchar.h dateparse.h dateparse.c
dchar.obj: dchar.h dchar.c
lstring.obj: lstring.h lstring.c
mem.obj: mem.h mem.c
mutex.obj: mutex.h mutex.c
port.obj: port.h port.c
root.obj: root.h root.c
regexp.obj: root.h dchar.h regexp.h regexp.c
stringtable.obj: root.h stringtable.h stringtable.c
istringtable.obj: root.h stringtable.h istringtable.h istringtable.c
testdate.obj : dchar.h date.h testdate.c
testregexp.obj : dchar.h regexp.h testregexp.c
thread.obj : thread.h thread.c

testregexp.exe : dmc.mak rutabega.lib testregexp.obj $(GC)\dmgc.lib
	$(CC) -o testregexp $(CFLAGS) testregexp.obj rutabega.lib $(GC)\dmgc.lib

testmem.exe : dmc.mak rutabega.lib testmem.obj $(GC)\dmgc.lib
	$(CC) -o testmem $(CFLAGS) testmem.obj rutabega.lib $(GC)\dmgc.lib

testdate.exe : dmc.mak rutabega.lib testdate.obj $(GC)\dmgc.lib
	$(CC) -o testdate $(CFLAGS) testdate.obj rutabega.lib $(GC)\dmgc.lib

test : testregexp.exe
	testregexp

archive.exe : dmc.mak rutabega.lib archive.obj
	$(CC) -o archive $(CFLAGS) -L/map archive.obj rutabega.lib $(GC)\dmgc.lib

shell.exe : dmc.mak rutabega.lib shell.obj
	$(CC) -o shell $(CFLAGS) -L/map shell.obj mem.obj rutabega.lib

utoc.exe : dmc.mak rutabega.lib utoc.obj mem.obj
	$(CC) -o utoc $(CFLAGS) -L/map/co utoc.obj mem.obj rutabega.lib

clean:
	del *.obj

zip : $(SRC1) $(SRC2) $(SRC3) $(SRC4) $(SRC5) $(SRC6)
	zip32 -u root $(SRC1)
	zip32 -u root $(SRC2)
	zip32 -u root $(SRC3)
	zip32 -u root $(SRC4)
	zip32 -u root $(SRC5)
	zip32 -u root $(SRC6)

