# Minimal build script for modern Linux systems. Needs i386 multilib gcc.

CC = g++ --std=gnu++98 -fpermissive -w -m32 -static -Isrc/dmgc -Isrc/root -DUNICODE -DVPTR_INDEX=0 -Dlinux -O

DSCRIPT_SRCS = \
  src/dmgc/bits.c \
  src/dmgc/gc.c \
  src/dmgc/linux.c \
  src/dscript/darguments.c \
  src/dscript/darray.c \
  src/dscript/dboolean.c \
  src/dscript/ddate.c \
  src/dscript/derror.c \
  src/dscript/dfunction.c \
  src/dscript/dglobal.c \
  src/dscript/dmath.c \
  src/dscript/dnative.c \
  src/dscript/dnumber.c \
  src/dscript/dobject.c \
  src/dscript/dregexp.c \
  src/dscript/dscript.c \
  src/dscript/dstring.c \
  src/dscript/dstring2.c \
  src/dscript/evalerror.c \
  src/dscript/expression.c \
  src/dscript/function.c \
  src/dscript/id.c \
  src/dscript/identifier.c \
  src/dscript/ir.c \
  src/dscript/iterator.c \
  src/dscript/lexer.c \
  src/dscript/opcodes.c \
  src/dscript/optimize.c \
  src/dscript/parse.c \
  src/dscript/program.c \
  src/dscript/property.c \
  src/dscript/rangeerror.c \
  src/dscript/referenceerror.c \
  src/dscript/response.c \
  src/dscript/scope.c \
  src/dscript/statement.c \
  src/dscript/symbol.c \
  src/dscript/syntaxerror.c \
  src/dscript/text.c \
  src/dscript/threadcontext.c \
  src/dscript/toir.c \
  src/dscript/typeerror.c \
  src/dscript/urierror.c \
  src/dscript/value.c \
  src/root/date.c \
  src/root/dateparse.c \
  src/root/dchar.c \
  src/root/dmgcmem.c \
  src/root/lstring.c \
  src/root/mutex.c \
  src/root/port.c \
  src/root/printf.c \
  src/root/random.c \
  src/root/regexp.c \
  src/root/root.c \
  src/root/stringcommon.c \
  src/root/stringtable.c \
  src/root/thread.c

dscript:
	$(CC) -o dscript $(DSCRIPT_SRCS)

testgc:
	$(CC) -o testgc src/dmgc/testgc.c src/dmgc/gc.c src/dmgc/bits.c src/dmgc/linux.c src/root/printf.c

clean:
	rm -f dscript testgc
