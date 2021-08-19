#
# $Header: /source/M380/gdb-3.0/dist-gdb/RCS/Makefile,v 1.2 89/04/14 22:16:23 kupfer Exp $
#
# On HPUX, you need to add -Ihpux to CFLAGS.
# The headers in the subdir hpux override system headers
# and tell GDB to use BSD executable file format.
# You also need to add -lGNU to CLIBS, and perhaps CC = gcc.

# On USG (System V) machines, you must make sure to setup REGEX &
# REGEX1 to point at regex.o and use the USG version of CLIBS.

# On Sunos 4.0 machines, make sure to compile *without* shared
# libraries if you want to run gdb on itself.  Make sure to compile
# any program on which you want to run gdb without shared libraries.

# If you are compiling with gcc, make sure that either 1) You use the
# -traditional flag, or 2) You have the fixed include files where gcc
# can reach them.  Otherwise the ioctl calls in inflow.c will be
# incorrectly compiled.

#CC=/bin/cc
#CC= cci860
CC= /bin/cc

# Set this up with gcc if you have gnu ld and the loader will print out
# line numbers for undefinded refs.  
CC-LD=${CC}

# -I. for "#include <obstack.h>".  Possibly regex.h also.
CFLAGS =  -I. -Iinclude -g -Di860=1 
#CFLAGS = -g -pg -I. -O
#CFLAGS = -I. -g -DORC -DDEBUG
#CFLAGS = -I. -g -DMACH -DORC -DCOFF_FORMAT -DCS_GENERIC 
# CFLAGS = -I. -g -DMACH -DORC -DCOFF_FORMAT -DCS_GENERIC -DCTHREADS
# although a.out.h indirectly defines AOUTHDR which in gdb defines COFF_FORMAT
# I'm not sure that this works....
LDFLAGS = -g

# define this to be "obstack.o" if you don't have the obstack library installed
# you must at the same time define OBSTACK1 as "obstack.o" 
# so that the dependencies work right.  Similarly with REGEX and "regex.o".
# You must define REGEX and REGEX1 on USG machines.
OBSTACK = obstack.o
OBSTACK1 = obstack.o
REGEX = regex.o
REGEX1 = regex.o

#
# define this to be "malloc.o" if you want to use the gnu malloc routine
# (useful for debugging memory allocation problems in gdb).  Otherwise, leave
# it blank.
GNU_MALLOC =
#GNU_MALLOC = malloc.o

# Flags to be used in compiling malloc.o
# Specify range checking for storage allocation.
MALLOC_FLAGS =
#MALLOC_FLAGS = ${CFLAGS} -Drcheck -Dbotch=fatal -DMSTATS

# for BSD
#CLIBS = $(OBSTACK) $(REGEX) $(GNU_MALLOC) /usr/lib/libmach.a \
	/usr/lib/libthreads.a 
# for USG
CLIBS= $(OBSTACK) $(REGEX) $(GNU_MALLOC)
#CLIBS= $(OBSTACK) $(REGEX) $(GNU_MALLOC) -lPW

SFILES = blockframe.c breakpoint.c coffread.c command.c core.c dbxread.c \
	 environ.c eval.c expprint.c findvar.c infcmd.c inflow.c infrun.c \
	 kdb-start.c main.c printcmd.c \
	 remote.c source.c stack.c standalone.c stuff.c symmisc.c symtab.c \
	 utils.c valarith.c valops.c valprint.c values.c version.c expread.y \
	 xgdb.c threads.c orc-sup.c talloca.c

DEPFILES = convex-dep.c umax-dep.c gould-dep.c default-dep.c sun3-dep.c \
	   sparc-dep.c hp9k320-dep.c news-dep.c i386-dep.c i860-dep.c

PINSNS = gld-pinsn.c i386-pinsn.c sparc-pinsn.c vax-pinsn.c m68k-pinsn.c \
	 ns32k-pinsn.c i860-pinsn.c

HFILES = command.h defs.h environ.h expression.h frame.h getpagesize.h \
	 inferior.h robotussin.h symseg.h symtab.h \
	 value.h wait.h cthread_internals.h cthread_options.h

OPCODES = m68k-opcode.h pn-opcode.h sparc-opcode.h npl-opcode.h vax-opcode.h \
	  ns32k-opcode.h i860-opcode.h

MFILES = m-hp9k320.h m-i386.h m-isi.h m-merlin.h m-news.h m-npl.h m-pn.h \
	 m-sparc.h m-sun2.h m-sun3.h m-sunos4.h m-umax.h m-vax.h m-i860.h

POSSLIBS = obstack.h obstack.c regex.c regex.h malloc.c

TESTS = testbpt.c testfun.c testrec.c testreg.c testregs.c

OTHERS = Makefile createtags munch config.gdb ChangeLog README TAGS \
	 gdb.texinfo .gdbinit COPYING notes

TAGFILES = ${SFILES} ${DEPFILES} ${PINSNS} ${HFILES} ${OPCODES} ${MFILES} \
	   ${POSSLIBS}
TARFILES = ${TAGFILES} ${OTHERS}

OBS = main.o blockframe.o breakpoint.o findvar.o stack.o source.o \
    values.o eval.o valops.o valarith.o valprint.o printcmd.o \
    symtab.o symmisc.o coffread.o dbxread.o infcmd.o infrun.o remote.o \
    command.o utils.o expread.o expprint.o pinsn.o environ.o version.o \
    threads.o orc-sup.o dummy.o talloca.o

TSOBS = core.o inflow.o dep.o

NTSOBS = standalone.o

TSSTART = /lib/crt0.o

NTSSTART = kdb-start.o

gdb : $(OBS) $(TSOBS) $(OBSTACK1) $(REGEX1)
#	there is currently a bug in Mach/386 with shell scripts!
	${CC-LD} $(LDFLAGS) -o gdb init.c $(OBS) $(TSOBS) $(CLIBS)
#	${CC} -c init.c 
#	ldi860 $(ROOT)/lib/crt0.o.bstep $(LDFLAGS) -o gdb init.o $(OBS) $(TSOBS) $(CLIBS) x.o $(ROOT)/lib/libc.a
#	rm init.c init.o

munch : $(OBS) $(TSOBS) $(OBSTACK1) $(REGEX1)
#	there is currently a bug in Mach/386 with shell scripts!
	sh munch $(OBS) $(TSOBS) > init.c

link : $(OBS) $(TSOBS) $(OBSTACK1) $(REGEX1)
	${CC-LD} $(LDFLAGS) -o gdb init.c $(OBS) $(TSOBS) $(CLIBS)

xgdb : $(OBS) $(TSOBS) xgdb.o $(OBSTACK1) $(REGEX1)
	munch $(OBS) $(TSOBS) xgdb.o > init.c
	$(CC-LD) $(LDFLAGS) -o xgdb init.c $(OBS) $(TSOBS) xgdb.o \
	-lXaw -lXt -lX11 $(CLIBS)

kdb : $(NTSSTART) $(OBS) $(NTSOBS) $(OBSTACK1) $(REGEX1)
	munch $(OBS) $(NTSOBS) > init.c
	$(CC-LD) $(LDFLAGS) -c init.c $(CLIBS)
	ld -o kdb $(NTSSTART) $(OBS) $(NTSOBS) init.o -lc $(CLIBS)

alloca.o : alloca.s
	$(CC) -c alloca.s

threads.o: threads.c defs.h param.h frame.h inferior.h symtab.h
	$(CC) -c $(CFLAGS) -DCTHREADS threads.c

# If it can figure out the appropriate order, createtags will make sure
# that the proper m-*, *-dep, *-pinsn, and *-opcode files come first
# in the tags list.  It will attempt to do the same for dbxread.c and 
# coffread.c.  This makes using M-. on machine dependent routines much 
# easier.
#
#TAGS: ${TAGFILES}
#	createtags ${TAGFILES}
#tags: TAGS
tags:
	etags -t ${TAGFILES}

gdb.tar: ${TARFILES}
	rm -f gdb.tar
	mkdir dist-gdb
	cd dist-gdb ; for i in ${TARFILES} ; do ln -s ../$$i . ; done
	tar chf gdb.tar dist-gdb
	rm -rf dist-gdb

gdb.tar.Z: gdb.tar
	compress gdb.tar

clean:
	-rm ${OBS}
	-rm gdb

squeakyclean: clean
	-rm expread.tab.c

xgdb.o : xgdb.c defs.h param.h symtab.h frame.h
	$(CC) -c $(CFLAGS) xgdb.c -o $@

expread.tab.c : expread.y
	@echo 'Expect 101 shift/reduce conflicts and 1 reduce/reduce conflict.'
	yacc expread.y
	mv y.tab.c expread.tab.c

expread.o : expread.tab.c defs.h param.h symtab.h frame.h expression.h
	$(CC) -c ${CFLAGS} expread.tab.c
	mv expread.tab.o expread.o

#
# Only useful if you are using the gnu malloc routines.
#
malloc.o : malloc.c
	${CC} -c ${MALLOC_FLAGS} malloc.c

#
# dep.o depends on ALL the dep files since we don't know which one
# is really being used.
#
dep.o : ${DEPFILES} defs.h param.h frame.h inferior.h obstack.h

# pinsn.o depends on ALL the opcode printers
# since we don't know which one is really being used.
pinsn.o : ${PINSNS} defs.h param.h symtab.h obstack.h symseg.h frame.h \
	  ${OPCODES}

#
# The rest of this is a standard dependencies list (hand edited output of
# cpp -M).  It does not include dependencies of .o files on .c files.
#
blockframe.o : defs.h param.h symtab.h obstack.h symseg.h frame.h 
breakpoint.o : defs.h param.h symtab.h obstack.h symseg.h frame.h
coffread.o : defs.h param.h 
command.o : command.h defs.h
core.o : defs.h  param.h
dbxread.o : param.h obstack.h defs.h symtab.h obstack.h symseg.h 
environ.o : environ.h 
eval.o : defs.h  param.h symtab.h obstack.h symseg.h value.h expression.h 
expprint.o : defs.h symtab.h obstack.h symseg.h param.h expression.h
findvar.o : defs.h param.h symtab.h obstack.h symseg.h frame.h value.h 
infcmd.o : defs.h  param.h symtab.h obstack.h symseg.h frame.h inferior.h \
	   environ.h value.h
inflow.o : defs.h  param.h frame.h inferior.h
infrun.o : defs.h  param.h symtab.h obstack.h symseg.h frame.h inferior.h \
	   wait.h
kdb-start.o : defs.h param.h 
main.o : defs.h  command.h param.h
malloc.o :  getpagesize.h
obstack.o : obstack.h 
printcmd.o :  defs.h param.h frame.h symtab.h obstack.h symseg.h value.h \
	      expression.h 
regex.o : regex.h 
remote.o : defs.h  param.h frame.h inferior.h wait.h
source.o : defs.h  symtab.h obstack.h symseg.h param.h
stack.o :  defs.h param.h symtab.h obstack.h symseg.h frame.h 
standalone.o : defs.h param.h symtab.h obstack.h symseg.h frame.h \
	       inferior.h wait.h 
symmisc.o : defs.h symtab.h obstack.h symseg.h obstack.h 
symtab.o : defs.h  symtab.h obstack.h symseg.h param.h  obstack.h
utils.o : defs.h  param.h 
valarith.o : defs.h param.h symtab.h obstack.h symseg.h value.h expression.h 
valops.o :  defs.h param.h symtab.h obstack.h symseg.h value.h frame.h \
	    inferior.h
valprint.o :  defs.h param.h symtab.h obstack.h symseg.h value.h 
values.o :  defs.h param.h symtab.h obstack.h symseg.h value.h 

robotussin.h : getpagesize.h   
symtab.h : obstack.h symseg.h 

