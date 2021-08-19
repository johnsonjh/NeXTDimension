#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
LOCALDSTDIR=/Net/k9/dps/nd_proj/bin
DSTDIR=/usr/local/bin

PROGS=ld860

CFLAGS = -bsd -g -O -I../include
IFLAGS = -s

I860FLAGS = -DCROSS -DI860 -DPAGESIZE=4096

PAGESIZE = 4096

PRODUCT = $(PROGS)

HFILES =
YFILES =
CFILES = ${PRODUCT}.c
SFILES =

SRCS = ${HFILES} ${YFILES} ${CFILES} ${SFILES}
OTHER_SRCS = notes
OBJS = ${CFILES:.c=.o} ${YFILES:.y=.o} ${SFILES:.s=.o}

FILES = ${SRCS} Makefile Makedep
INTERMEDIATES =
DERIVED =
GARBAGE = TAGS tags vers.c

all:	${PRODUCT}

#
# Building PRODUCT
#

${PRODUCT}:	${OBJS} vers.o
	$(CC) $(CFLAGS) -o $@ ${OBJS} vers.o

ld860.o: .depend_done ld.c
	$(CC) $(CFLAGS) $(I860FLAGS) -c ld.c -o $@

vers.c: ${DOVERS} ${OBJS}
	@rm -f $@
	vers_string -c ${PRODUCT} > $@


#
# Standard targets
#
install:	$(PROGS)
	-mkdirs $(DSTROOT)$(DSTDIR)
	for i in $(PROGS); do ( install -m 755 $$i $(DSTROOT)$(DSTDIR) ); done

install_local:	$(PROGS)
	-mkdirs $(DSTROOT)$(LOCALDSTDIR)
	for i in $(PROGS); do ( install -m 755 $$i $(DSTROOT)$(LOCALDSTDIR) ); done

clean:
	rm -f $(PROGS) $(GARBAGE) *.o

depend .depend_done: ${DERIVED}
	rm -f Makedep
	${CC} ${CFLAGS} -M ${CFILES} ${SFILES} ${DERIVED} | \
	    awk '{ if ($$1 != prev) { if (rec != "") print rec; \
		rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec }' >Makedep
	-touch .depend_done


tags:	ALWAYS
	etags ${SRCS}
	ctags ${SRCS}

ALWAYS:

include Makedep
