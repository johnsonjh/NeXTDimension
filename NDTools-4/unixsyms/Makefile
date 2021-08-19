LOCALDSTDIR=/Net/k9/dps/nd_proj/bin
DSTDIR=/usr/local/bin

PROGS=unixsyms
CFLAGS = -g -I../include

all:	$(PROGS)

install:	$(PROGS)
	-mkdirs $(DSTROOT)$(DSTDIR)
	for i in $(PROGS); do ( install -m 755 $$i $(DSTROOT)$(DSTDIR) ); done

install_local:	$(PROGS)
	-mkdirs $(DSTROOT)$(LOCALDSTDIR)
	for i in $(PROGS); do ( install -m 755 $$i $(DSTROOT)$(LOCALDSTDIR) ); done

clean:
	rm -f $(PROGS) *.o
	
unixsyms:	unixsyms.o
	cc -o unixsyms unixsyms.o
