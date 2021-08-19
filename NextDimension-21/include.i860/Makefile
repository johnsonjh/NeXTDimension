SUBDIRS=sys machine vm kern
SRCS= cpus.h setjmp.h cs_kdb.h mach_load.h stat_time.h simple_clock.h
COPTS=
LOCALDSTDIR=$(DSTROOT)/Net/k9/dps/nd_proj/i860/include
DSTDIR=$(DSTROOT)/usr/local/i860/include
LINKPATH=../../include/ND
INSTALL=install -c

default all clean installh:	ND i860

ND:
	-ln -s ../ND ND

i860:
	-ln -s machine i860

$(DSTDIR)/ND:
	-mkdirs $(DSTDIR)
	-( cd $(DSTDIR) ; ln -s $(LINKPATH) ND )

$(LOCALDSTDIR)/ND:
	-mkdirs $(LOCALDSTDIR)
	-( cd $(LOCALDSTDIR) ; ln -s $(LINKPATH) ND )

$(DSTDIR)/i860:
	-mkdirs $(DSTDIR)/machine
	-( cd $(DSTDIR) ; ln -s machine ND )

$(LOCALDSTDIR)/i860:
	-mkdirs $(LOCALDSTDIR)/machine
	-( cd $(LOCALDSTDIR) ; ln -s machine ND )

install:	$(DSTDIR)/ND $(DSTDIR)/i860
	for i in $(SUBDIRS); \
	do \
		( cd $$i ; make install "DSTDIR=$(DSTDIR)" ); \
	done
	for i in $(SRCS); \
	do \
		echo $$i; \
		$(INSTALL) $$i $(DSTDIR); \
	done
	
install_local:	$(LOCALDSTDIR)/ND $(LOCALDSTDIR)/i860
	for i in $(SUBDIRS); \
	do \
		( cd $$i ; make install "DSTDIR=$(LOCALDSTDIR)" ); \
	done
	for i in $(SRCS); \
	do \
		echo $$i; \
		$(INSTALL) $$i $(LOCALDSTDIR); \
	done
