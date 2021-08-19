SRCS= boolean.h kern_msg.h lock.h map.h message.h port.h queue.h sched.h systm.h \
	task.h thread.h thread_modes.h timer.h types.h exception.h


DSTDIR=/usr/local/i860/include
SUBDIR=sys
INSTALL=install -c

install:
	-mkdirs $(DSTDIR)/$(SUBDIR)
	for i in $(SRCS); \
	do \
		echo $$i; \
		$(INSTALL) $$i $(DSTDIR)/$(SUBDIR); \
	done
