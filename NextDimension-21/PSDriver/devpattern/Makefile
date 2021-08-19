#
#	Devpattern Package Makefile
#

HFILES = imagepriv.h patternpriv.h

CFILES = colorpattern.c constclrpat.c constgraypat.c graypattern.c\
	 grypat4of4.c im110.c  ims11d11.c ims11d12.c ims11d1x.c\
	 ims12d12notfr.c ims1xd11.c ims1xd12.c imsupport.c imsxxd11.c\
	 imsxxd12.c monopattern.c patternimpl.c
  
# NOTE: The *main.c files here are included by some CFILES above
OTHER_SOURCES = im106.c im116.c im111.c im112.c im113.c im114.c ims11d14.c\
		ims11d18.c ims1xd14.c ims1xd18.c ims1xd1x.c imsxxd1x.c\
		imsxxd14.c imsxxd18.c imsxxd3x.c ims11d1xmain.c\
		ims1xd1xmain.c imsxxd1xmain.c maskpattern.c directory

include ../config-cpu
include ../Makefile.common

NAME = devpattern-$(CPUTYPE).a

all:	$(NAME)

install: all

installh:

clean:
	-rm -rf obj-$(CPUTYPE) make.log depend $(NAME)

$(NAME): $(OFILEDIR) $(OFILES)
	 ar r $(NAME) $(OFILES)
	 $(RANLIB) $(NAME)
	
-include depend

