XXX= -ga -X370 -X425 -X9 -X230 -Z422
DEFS=
COPTS=
INCLUDE=-I../include.i860
CFLAGS= -O $(COPTS) $(DEFS)  $(INCLUDE)
LDFLAGS=-p -T f8000000
DSTROOT=
DESTDIR=/usr/local/i860/lib
LOCALDESTDIR=/Net/k9/dps/nd_proj/i860/lib
CC=cc860
LD=ld860
AS=as860
RANLIB=ranlib860
MAKEOPTS=  $(MFLAGS) "DSTROOT=$(DSTROOT)" "COPTS=$(COPTS)"
INSTALL=install -c

SUBDIRS=kdb ND libc
OBJDIR=obj

# CAUTION: Keep modules with page or other large align requirements FIRST in ld list
CRTOBJS=$(OBJDIR)/locore.o \
	$(OBJDIR)/flush.o

OBJS= $(OBJDIR)/_i860rt.o $(OBJDIR)/cons.o \
	$(OBJDIR)/cswitch.o $(OBJDIR)/trap.o $(OBJDIR)/stubs.o	\
	$(OBJDIR)/kalloc.o $(OBJDIR)/kern_map.o \
	$(OBJDIR)/param.o $(OBJDIR)/switch.o $(OBJDIR)/kern_main.o \
	$(OBJDIR)/init_sysent.o $(OBJDIR)/malloc.o \
	$(OBJDIR)/syscalls.o $(OBJDIR)/messages.o $(OBJDIR)/clock.o \
	$(OBJDIR)/ports.o $(OBJDIR)/i860_init.o  $(OBJDIR)/ND_KernUser.o \
	$(OBJDIR)/vm_user.o $(OBJDIR)/vm_fault.o $(OBJDIR)/vm_kern.o \
	$(OBJDIR)/pmap.o $(OBJDIR)/ipc_oob_data.o $(OBJDIR)/vm_pager.o \
	$(OBJDIR)/cursor.o $(OBJDIR)/locks.o

LIBOBJS= $(OBJDIR)/strcat.o $(OBJDIR)/strcpy.o $(OBJDIR)/strncat.o \
	$(OBJDIR)/strncpy.o $(OBJDIR)/strcmp.o $(OBJDIR)/strlen.o \
	$(OBJDIR)/strncmp.o

KDBOBJS= $(OBJDIR)/kdb.o $(OBJDIR)/access.o $(OBJDIR)/bits.o \
	$(OBJDIR)/command.o $(OBJDIR)/expr.o \
	$(OBJDIR)/format.o $(OBJDIR)/input.o $(OBJDIR)/kdb_main.o \
	$(OBJDIR)/message.o $(OBJDIR)/output.o \
	$(OBJDIR)/pcs.o $(OBJDIR)/print.o $(OBJDIR)/runpcs.o \
	$(OBJDIR)/setup.o $(OBJDIR)/stack.o $(OBJDIR)/sym.o \
	$(OBJDIR)/tbls.o $(OBJDIR)/utls.o

EOBJS= example.o

.s.o:
	$(CC) -c $*.s

.c.o:
	$(CC) $(CFLAGS) -c $*.c

all:	crt0.o libsa.a kern.860

installh:
	@echo Nothing to $@ here

install: crt0.o libsa.a
	-mkdirs $(DSTROOT)/$(DESTDIR)
	$(INSTALL) -m 644 crt0.o $(DSTROOT)/$(DESTDIR)
	$(INSTALL) libsa.a $(DSTROOT)/$(DESTDIR)
	$(RANLIB) $(DSTROOT)/$(DESTDIR)/libsa.a
	chmod 644 $(DSTROOT)/$(DESTDIR)/libsa.a

install_local: crt0.o libsa.a
	-mkdirs $(DSTROOT)/$(LOCALDESTDIR)
	$(INSTALL) -m 644 crt0.o $(DSTROOT)/$(LOCALDESTDIR)
	$(INSTALL) libsa.a $(DSTROOT)/$(LOCALDESTDIR)
	$(RANLIB) $(DSTROOT)/$(LOCALDESTDIR)/libsa.a
	chmod 644 $(DSTROOT)/$(DESTDIR)/libsa.a

$(SUBDIRS): obj
	for i in $(SUBDIRS); do ( cd $$i && $(MAKE) $(MAKEOPTS) ); done

obj::
	-mkdirs obj

crt0.o:	$(SUBDIRS) $(CRTOBJS)
	$(LD) -r -o $@ $(CRTOBJS)
	
libsa.a: $(SUBDIRS) $(OBJS) $(LIBOBJS) $(KDBOBJS)
	rm -f $@
	ar rc $@ $(OBJS) $(LIBOBJS) $(KDBOBJS)
	$(RANLIB) $@
	

kern.860: crt0.o libsa.a $(EOBJS)
	$(LD) $(LDFLAGS) -o kern.860 -e pstart crt0.o $(EOBJS) libsa.a
	unixsyms kern.860


FORCE:

clean:
	rm -f a.out $(OBJS) $(KDBOBJS) $(EOBJS) crt0.o libsa.a kern.860 tags
	for i in $(SUBDIRS); do ( cd $$i && $(MAKE) $(MAKEOPTS)  clean ); done
	

