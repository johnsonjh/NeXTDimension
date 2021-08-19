DSTROOT=
LOCDESTDIR=$(DSTROOT)/Net/k9/dps/nd_proj/bin
DESTDIR=$(DSTROOT)/usr/local/bin
CFILES=romimage.c
OBJS=romimage.o
COPTS=
CFLAGS= $(COPTS) -O -I../../include.i860
LIBS=	

860DIR=/usr/local/bin
CC860=$(860DIR)/cc860
LD860=$(860DIR)/ld860
INSTALL=install -c -m 755 -s

OBJS=	locore.o flush.o i860_init.o main.o
SRCS=	locore.s flush.s i860_init.c main.c

all:	rom

.s.o:
	$(CC860) $(CFLAGS) -DASSEMBLER -c $*.s

.c.o:
	$(CC860) $(CFLAGS) -c $*.c

rom:	$(OBJS)
	$(LD860) -o rom -x -p -T FFFE0000 -e Reset $(OBJS)
	size rom


clean:
	rm -f ${OBJS} rom

install:
	@echo "Nothing to install here..."

install_local:
	@echo "Nothing to install here..."

installh:
	@echo "Nothing to installh here..."

