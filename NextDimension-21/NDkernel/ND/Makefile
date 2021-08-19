XXX= -ga -X370 -X425 -X9 -X230 -Z422
DEFS=

COPTS=
INCLUDE=-I../../include.i860 -I../../PSDriver/include -I/usr/local/include/ps
CFLAGS= -O $(COPTS) $(DEFS) $(INCLUDE)
LDFLAGS=-p -T 0xfff00000 -d 0xfff80000
CC=cc860
GCC=ccom860
LD=ld860
AS=as860
MV=mv
MachDriverDIR=../../MachDriver
GENERATED_FILES=ND_KernUser.c ND_Kern.h build_pte lodata.s
OBJDIR=../obj

OBJS=	locore.o flush.o cswitch.o	\
	i860_init.o cursor.o \
	kern_map.o param.o switch.o kalloc.o kern_main.o \
	init_sysent.o syscalls.o trap.o messages.o malloc.o \
	clock.o ports.o \
	_i860rt.o cons.o stubs.o ND_KernUser.o vm_user.o vm_fault.o \
	vm_kern.o pmap.o ipc_oob_data.o vm_pager.o locks.o

default:	$(OBJS)

.s.o:
	$(CC) $(CFLAGS) -DASSEMBLER -c $*.s
	-$(LD) -x -r $@ -o xxx.o
	rm -f $@
	${MV} xxx.o $@
	rm -f $(OBJDIR)/$*.o
	ln $*.o $(OBJDIR)/$*.o


.c.o: io_space.h
	$(CC) $(CFLAGS) -c $*.c
	-$(LD) -x -r $@ -o xxx.o
	rm -f $@
	${MV} xxx.o $@
	rm -f $(OBJDIR)/$*.o
	ln $*.o $(OBJDIR)/$*.o

clean:
	rm -f a.out $(OBJS) $(EOBJS) $(GENERATED_FILES)

lodata.s:	build_pte.c
	cc -o build_pte $(INCLUDE) build_pte.c
	./build_pte > lodata.s
	rm -f build_pte
	
cursor.o: cursor.c
	$(CC) $(CFLAGS) -DISP=isp_mc68020 -DOS=os_mach \
		-DPACKAGE_SPECS=\"package_specs.h\" -c cursor.c
	-$(LD) -x -r $@ -o xxx.o
	rm -f $@
	${MV} xxx.o $@
	rm -f $(OBJDIR)/$*.o
	ln $*.o $(OBJDIR)/$*.o




ND_KernUser.o:	ND_KernUser.c ND_Kern.h

ND_KernUser.c:	$(MachDriverDIR)/ND_KernUser.c
	cp $(MachDriverDIR)/ND_KernUser.c .

ND_Kern.h:	$(MachDriverDIR)/ND_Kern.h
	cp $(MachDriverDIR)/ND_Kern.h .

$(MachDriverDIR)/ND_Kern.h $(MachDriverDIR)/ND_KernUser.c:
	( cd $(MachDriverDIR) ; make ND_KernUser.c )

