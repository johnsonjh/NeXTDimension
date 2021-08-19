SRCS= exception.h	pcb.h	psl.h	scb.h	vm_param.h	\
	cpu.h	param.h	proc.h 	reg.h 	trap.h	vm_types.h

DSTDIR=/usr/local/i860/include
SUBDIR=i860
DSTLINK=machine
INSTALL=install -c

install:	$(DSTDIR)/$(SUBDIR) $(DSTDIR)/$(DSTLINK)
	for i in $(SRCS); \
	do \
		echo $$i; \
		$(INSTALL) $$i $(DSTDIR)/$(SUBDIR); \
	done

$(DSTDIR)/$(DSTLINK): $(DSTDIR)/$(SUBDIR)
	-ln -s $(DSTDIR)/$(SUBDIR) $(DSTDIR)/$(DSTLINK)

$(DSTDIR)/$(SUBDIR):
	-mkdirs $(DSTDIR)/$(SUBDIR)
