DSTROOT=
LOCDESTDIR=/Net/k9/dps/nd_proj/bin
LOCDESTLIBDIR=/Net/k9/dps/nd_proj/i860/lib
DESTDIR=/usr/local/bin
DESTLIBDIR=/usr/local/i860/lib
COPTS=
INCL=-I..
CFLAGS=$(COPTS) -g -O $(INCL)
LIB=../libND/libND.a
LIBDIR=../libND
PROGS=run860 debug860
AUXPROGS=bind860.o
INSTALL=install -c -m 755 -s
INSTALLOBJ=install -c -m 644
INSTALLSH=install -c -m 755

default:	$(PROGS) $(AUXPROGS)

all:	default

install:	$(PROGS) $(AUXPROGS)
	-mkdirs $(DSTROOT)/$(DESTDIR)
	-mkdirs $(DSTROOT)/$(DESTLIBDIR)
	for i in $(PROGS); do ( $(INSTALL) $$i $(DSTROOT)/$(DESTDIR) ); done
	$(INSTALLOBJ) $(AUXPROGS) $(DSTROOT)/$(DESTLIBDIR)
	$(INSTALLSH) -d bind860.sh $(DSTROOT)/$(DESTDIR)/bind860

installh:
	@echo cmds does not install any headers.

install_local:	$(PROGS) $(AUXPROGS)
	-mkdirs $(DSTROOT)/$(LOCDESTDIR)
	-mkdirs $(DSTROOT)/$(LOCDESTLIBDIR)
	for i in $(PROGS); do ( $(INSTALL) $$i $(DSTROOT)/$(LOCDESTDIR) ); done
	$(INSTALL) $(AUXPROGS) $(DSTROOT)/$(LOCDESTLIBDIR)
	$(INSTALL) -d bind860.sh $(DSTROOT)/$(LOCDESTDIR)/bind860
	
clean:
	rm -f *.o $(PROGS) $(AUXPROGS)

run860:	run860.o $(LIB)
	cc -g -o run860 run860.o $(LIB)

debug860:	debug860.o $(LIB)
	cc -g -o debug860 debug860.o $(LIB)

bind.o:	run860.c
	cc -g -c -o bind.o -DBIND860 $(CFLAGS) run860.c
	
run860.o:	run860.c
	cc -g -c -DRUN860 $(CFLAGS) run860.c

debug860.o:	run860.c
	cc -g -c -o debug860.o -DDEBUG860 $(CFLAGS) run860.c

bind860.o:	bind.o $(LIB)
	$(LD) -r -o bind860.o bind.o $(LIB)

$(LIB):
	cd $(LIBDIR) ; make


