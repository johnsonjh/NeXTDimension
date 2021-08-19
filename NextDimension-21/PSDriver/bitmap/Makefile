#
#	Bitmap Package Makefile
#

HFILES = bm34.h bm38.h

CFILES = bm.c bmlocal.c bmpattern.c bm34.c bm34comp.c bm34conv.c \
	 bm38.c bm38comp.c bm38conv.c mp12.c mp12comp.c mp12conv.c \
	 mp12copyline.c mp12funcline.c mp12hlrect.c mp12soverrect.c \
	 mp12copyrect.c

SFILES = bm38copy.860.s bm38conv2to32.860.s bm38hl.860.s bm38sover.860.s \
	 bm38dissolve.860.s

OTHER_SWITCHES = -DwdProcs=fmProcs -DwdMarkProcs=fmMarkProcs

include ../config-cpu

include ../Makefile.common

NAME = bitmap-$(CPUTYPE).o

all: $(NAME)

install: all

install_local:

installh:

clean:
	-rm -rf $(NAME) obj-$(CPUTYPE) make.log depend

$(NAME): $(OFILEDIR) $(OFILES)
	 $(LD) -r -o $(NAME) $(OFILES) 

-include depend
