#
#	Device Package Makefile
#

include ../config-cpu

HFILES = devcommon.h devmark.h devprim.h framedev.h framemaskdev.h \
	 genericdev.h hostdict.h maskcache.h nextprebuilt.h nulldev.h \
	 prebuiltformat.h 

CFILES = blackmask.c blackrun.c blacktrap.c clippedmask.c constantmask.c\
	 constantrun.c constanttrap.c devbounds.c devcommon.c devinit.c\
	 devprim.c framedev.c framemaskdev.c  genericdev.c graymask.c grayrun.c\
	 graytrap.c halftone.c mark.c markimage.c nulldev.c qintersect.c\
	 whitemask.c whiterun.c whitetrap.c

OTHER_SOURCES = hostdict.c maskcache.c nextprebuilt.c tfrfcn.c trapprecision.c

SFILES = trap32.860.s run32.860.s mask32.860.s

include ../Makefile.common

NAME = device-$(CPUTYPE).a

OTHER_SWITCHES = -DSAMPLEDEVICE=1 -DMASKCOMPRESSION=0 -DNEXTDIMENSION

all:	$(NAME)

install: all

installh:

clean:
	-rm -rf obj-$(CPUTYPE) make.log depend $(NAME)

$(NAME): $(OFILEDIR) $(OFILES)
	ar r $(NAME) $(OFILES)
	$(RANLIB) $(NAME)

-include depend
