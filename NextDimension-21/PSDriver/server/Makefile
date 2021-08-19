#
#	Server Makefile
#

INSTALLDIR = $(DSTROOT)/usr/lib/NextStep/Displays/NextDimension

CFILES = bounds.c imstretch.c imident.c process.c server.c \
	 support.c pool.c video.c dmsd.c

OTHER_OFILES = psServer.o argstream.o markpack.o

SFILES = copy38.860.s

include ../config-cpu
include ../Makefile.common

NAME = server-$(CPUTYPE)

all: $(NAME)

install: all
	mkdirs $(INSTALLDIR)
	$(INSTALL) server $(INSTALLDIR)
	$(INSTALL) dmsd.ps $(INSTALLDIR)
	

installh:

clean:
	-rm -rf server $(NAME) obj-$(CPUTYPE) make.log depend

psServer.o: $(COMMON)/psServer.c
	$(CC) $(CFLAGS) -c $(COMMON)/psServer.c -o $(OFILEDIR)/$*.o

argstream.o: $(COMMON)/argstream.c
	$(CC) $(CFLAGS) -c $(COMMON)/argstream.c -o $(OFILEDIR)/$*.o

markpack.o: $(COMMON)/markpack.c
	$(CC) $(CFLAGS) -c $(COMMON)/markpack.c -o $(OFILEDIR)/$*.o

$(NAME): $(OFILEDIR) $(OFILES) ../bitmap/bitmap-$(CPUTYPE).o \
	../device/device-$(CPUTYPE).a ../devpattern/devpattern-$(CPUTYPE).a
	$(LD) $(LD_FINAL_FLAGS) $(CRT0) \
	 -i_os_bzero:_bzero -i_os_bcopy:_bcopy -i_os_calloc:_calloc \
	 -i_os_malloc:_malloc -i_os_abort:_abort -i_os_printf:_printf \
	 -i_os_realloc:_realloc -i_os_free:_free -i_os_sureCalloc:_calloc \
	 -i_os_sureMalloc:_malloc -i_os_strcat:_strcat -i_os_strcmp:_strcmp \
	 -i_os_strcpy:_strcpy -i_os_strncpy:_strncpy -i_os_strlen:_strlen \
	  $(OFILES) \
	../bitmap/bitmap-$(CPUTYPE).o ../device/device-$(CPUTYPE).a \
	../devpattern/devpattern-$(CPUTYPE).a $(LIBS) -o $(NAME)
	$(POST_LD_COMMAND)

-include depend
