SRCS= parallel.h

DSTDIR=/usr/local/i860/include
SUBDIR=kern
INSTALL=install -c

install:
	-mkdirs $(DSTDIR)/$(SUBDIR)
	for i in $(SRCS); \
	do \
		echo $$i; \
		$(INSTALL) $$i $(DSTDIR)/$(SUBDIR); \
	done
