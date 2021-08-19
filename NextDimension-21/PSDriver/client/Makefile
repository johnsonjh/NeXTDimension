#
#	Client Makefile
#

CPUTYPE = 040

LIBS = -lND -lkernload

HFILES = nd.h remote.h

CFILES = nd.c remote.c

OTHER_OFILES = psUser.o argstream.o markpack.o

include ../Makefile.common

NAME = reloc

INSTALLDIR = $(DSTROOT)/usr/lib/NextStep/Displays/NextDimension

all:	$(NAME)

o40:
	$(MAKE) $(MAKEOPTS) "CFLAGS=$(CFLAGS) -O -DSERVER_040" all

install: all
	mkdirs $(INSTALLDIR)
	 $(INSTALL) $(NAME) $(INSTALLDIR)

installh:

install_local:

clean:
	rm -rf $(NAME) obj-$(CPUTYPE) depend make.log

psUser.o: $(COMMON)/psUser.c
	$(CC) $(CFLAGS) -c $(COMMON)/psUser.c -o $(OFILEDIR)/$*.o

argstream.o: $(COMMON)/argstream.c
	$(CC) $(CFLAGS) -c $(COMMON)/argstream.c -o $(OFILEDIR)/$*.o

markpack.o: $(COMMON)/markpack.c
	$(CC) $(CFLAGS) -c $(COMMON)/markpack.c -o $(OFILEDIR)/$*.o

$(NAME): $(OFILEDIR) $(OFILES)
	 ld -r -i_Start:_NDStart $(OFILES) $(LIBDIR) $(LIBS) -o $(NAME)

-include depend
