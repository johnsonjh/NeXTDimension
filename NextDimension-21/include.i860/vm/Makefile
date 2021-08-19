SRCS= vm_page.h vm_prot.h vm_param.h vm_kern.h

DSTDIR=/usr/local/i860/include
SUBDIR=vm
INSTALL=install -c

install:
	-mkdirs $(DSTDIR)/$(SUBDIR)
	for i in $(SRCS); \
	do \
		echo $$i; \
		$(INSTALL) $$i $(DSTDIR)/$(SUBDIR); \
	done
