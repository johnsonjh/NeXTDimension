#
#
#XXX=-ga -X425 -Z422 -X370 -X9 -X230
DEFS=-DKERNEL -UMACH -DCMU
COPTS=
INCLUDE=-I. -I../../include.i860
CFLAGS= -O $(COPTS) $(DEFS) $(INCLUDE)
CC=cc860
LD=ld860
AS=as860
MV=mv
OBJDIR=../obj

KDBOBJS= kdb.o access.o bits.o command.o expr.o \
	format.o input.o kdb_main.o message.o output.o \
	pcs.o print.o runpcs.o setup.o stack.o sym.o \
	tbls.o utls.o

default: $(KDBOBJS)

a.out: $(OBJS) $(KDBOBJS) $(EOBJS)
	$(LD) $(LDFLAGS) -e pstart $(OBJS) $(KDBOBJS) $(EOBJS)
	nusyms a.out

.s.o:
	$(CC) -c $*.s
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
	rm -f a.out $(OBJS) $(EOBJS) $(KDBOBJS)

