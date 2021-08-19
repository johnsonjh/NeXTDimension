DSTROOT=
LOCDESTDIR=$(DSTROOT)/Net/k9/dps/nd_proj/bin
DESTDIR=$(DSTROOT)/usr/local/bin
CFILES=romimage.c
OBJS=romimage.o
COPTS=
CFLAGS= $(COPTS) -O -I..
LIBS=	../libND/libND.a -lkernload -lsys_s

860DIR=/usr/local/bin
CC860=$(860DIR)/cc860
LD860=$(860DIR)/ld860
INSTALL=install -c -m 755 -s

all:	NDromload

mon.prom: NDromload
	NDromload -3 -S > mon.prom

ND_pcode.out:	ND_pcode.def pas/pas
	pas/pas -o ND_pcode.out ND_pcode.def

NDromload: romload.o romcode/rom ND_pcode.out
	$(CC) -o NDromload romload.o \
		-sectcreate __i860 __bootcode romcode/rom \
		-sectcreate __i860 __pcode ND_pcode.out \
		$(LIBS)

romcode/rom:
	( cd romcode ; make )

pas/pas:
	(cd pas; make)

clean:
	rm -f ${OBJS} mon.prom .depend_done Makefile.bak \
		ND_pcode.out
	(cd pas; make clean)
	(cd romcode; make clean)

install:	NDromload
	$(INSTALL) NDromload $(DESTDIR)

install_local:	NDromload
	-$(INSTALL) NDromload $(LOCDESTDIR)


installh:
	@echo "Nothing to installh here..."

depend .depend_done:
	grep '^#include' ${CFILES} | grep -v '<' | \
	sed -e 's/:[^"]*"\([^"]*\)".*/: \1/' \
	    -e 's/\.c/.o/' \
	    -e 's,../[a-zA-Z]*/,,' | \
	awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		       else rec = rec " " $$2 } } \
	      END { print rec } ' > makedep
	echo '$$r makedep' >>eddep
	echo '/^# DO NOT DELETE THIS LINE/+1,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep
	touch .depend_done

# DO NOT DELETE THIS LINE -- make depend uses it
