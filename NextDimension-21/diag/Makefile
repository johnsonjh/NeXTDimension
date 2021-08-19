INCL=-I..
COPTS=
LIBS= ../libND/libND.a -lsys_s
KLLIBS= ../libND/libND.a -lkernload -lsys_s
CFLAGS=$(COPTS) -g -O $(INCL)
LINKADDR=48000000
DSTROOT=
DSTDIR=/usr/local/bin
INSTALL=install -c -s -m 755

PROGS=	NDprocmem NDcbars NDgamma NDbootscreen NDromcheck

default:	$(PROGS)

all:	$(PROGS)

install:	$(PROGS)
	for i in $(PROGS); do ( $(INSTALL) $$i $(DSTROOT)/$(DSTDIR) ); done

installh:
	@echo "Nothing to installh here..."

clean:
	rm -f *.o $(PROGS) *.860
	
NDbootscreen:	bootscreen.o
	$(CC) $(CFLAGS) -o NDbootscreen bootscreen.o -lsys_s

NDromcheck:	romcheck.o
	$(CC) $(CFLAGS) -o NDromcheck romcheck.o $(KLLIBS)

NDcbars:	cbars.o ndslot.h
	$(CC) $(CFLAGS) -o NDcbars cbars.o $(LIBS)

NDgamma:	gamma.o ndslot.h
	$(CC) $(CFLAGS) -o NDgamma gamma.o $(LIBS)

NDprocmem:	procmem.o qreadtest.860 writetest.860 autotest.860
	$(CC) $(CFLAGS) -o NDprocmem procmem.o $(LIBS) \
	-segcreate __I860 __qreadtest qreadtest.860 \
	-segcreate __I860 __writetest writetest.860 \
	-segcreate __I860 __autotest autotest.860

jjprocmem:	procmem.o qreadtest.860 jjwritetest.860
	$(CC) $(CFLAGS) -o jjprocmem procmem.o $(LIBS) -segcreate __I860 __qreadtest qreadtest.860 -segcreate __I860 __writetest jjwritetest.860

readtest.860:	readtest.s
	cc860 -c readtest.s
	ld860 -o readtest.860 -p -T $(LINKADDR) readtest.o
	
qreadtest.860:	qreadtest.s
	cc860 -c qreadtest.s
	ld860 -o qreadtest.860 -p -T $(LINKADDR) qreadtest.o
	
writetest.860:	writetest.s
	cc860 -c writetest.s
	ld860 -o writetest.860 -p -T $(LINKADDR) writetest.o

autotest.860:	autotest.s
	cc860 -c autotest.s
	ld860 -o autotest.860 -p -T $(LINKADDR) autotest.o

jjwritetest.860:	jjwritetest.s
	cc860 -c jjwritetest.s
	ld860 -o jjwritetest.860 -p -T $(LINKADDR) jjwritetest.o
