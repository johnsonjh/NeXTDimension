XXX= -ga -X370 -X425 -X9 -X230 -Z422
DEFS= #-DFLOATING_PT
COPTS= 
INCLUDE=-I../../include.i860
CFLAGS= -O $(COPTS) $(DEFS) $(INCLUDE)
LDFLAGS=-p -T 0xfff00000 -d 0xfff80000
CC=cc860
GCC=ccom860
LD=ld860
AS=as860
MV=mv
OBJDIR=../obj

OBJS=	strcat.o   strcmp.o   strcpy.o   strlen.o   strncat.o  strncmp.o  strncpy.o

default:	$(OBJS)

.s.o:
	$(CC) $(CFLAGS) -DASSEMBLER -c $*.s
	-$(LD) -x -r $@ -o xxx.o
	rm -f $@
	${MV} xxx.o $@
	rm -f $(OBJDIR)/$*.o
	ln $*.o $(OBJDIR)/$*.o


.c.o:
	$(CC) $(CFLAGS) -c $*.c
	-$(LD) -x -r $@ -o xxx.o
	rm -f $@
	${MV} xxx.o $@
	rm -f $(OBJDIR)/$*.o
	ln $*.o $(OBJDIR)/$*.o

clean:
	rm -f a.out $(OBJS) $(EOBJS)
