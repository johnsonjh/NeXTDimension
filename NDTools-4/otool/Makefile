#
# Generic top-level makefile for simple commands
# (c) NeXT, Inc.  1987
#
# Note: otool.c will not build with -O as of 0.8 release
LOCALDSTDIR=/Net/k9/dps/nd_proj/bin
DSTDIR=/usr/local/bin

PROGS=ot860 ranlib860

CFLAGS = -g -I../include -DI860
PAGESIZE = 8192
IFLAGS = -s
BINDIR = /bin
LOCBINDIR = /usr/local/bin
CFILES = otool.c ranlib.c
HFILES =
OTHER_SRCS =
GARBAGE=vers.c

all:	.depend_done $(PROGS)

install:	$(PROGS)
	-mkdirs $(DSTROOT)$(DSTDIR)
	for i in $(PROGS); do ( install -m 755 $$i $(DSTROOT)$(DSTDIR) ); done

install_local:	$(PROGS)
	-mkdirs $(DSTROOT)$(LOCALDSTDIR)
	for i in $(PROGS); do ( install -m 755 $$i $(DSTROOT)$(LOCALDSTDIR) ); done

clean:
	rm -f $(PROGS) $(GARBAGE) *.o

ot860: otool.o vers.o
	$(CC) $(CFLAGS) -o ot860 otool.o vers.o

ranlib860: ranlib.o vers.o
	$(CC) $(CFLAGS) -o ranlib860 ranlib.o vers.o



vers.c: ${DOVERS} ${OBJS}
	@rm -f $@
	vers_string -c ccmisc > $@

DSTROOT LCLROOT SRCROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

${DSTROOT}${BINDIR} ${DSTROOT}${LOCBINDIR} ${LCLROOT}${BINDIR} ${LCLROOT}${LOCBINDIR}:
	mkdirs $@

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
