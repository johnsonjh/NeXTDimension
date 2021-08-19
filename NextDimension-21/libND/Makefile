INCDIR=..
SERVERDIR=../MachDriver

DESTINCDIR=/usr/local/include/ND
DESTLIBDIR=/usr/local/lib
LOCALDESTINCDIR=/Net/k9/dps/nd_proj/include/ND
LOCALDESTLIBDIR=/Net/k9/dps/nd_proj/lib
COPTS=

INCL=-I$(INCDIR)
CFLAGS=$(COPTS) -g -O $(INCL)
NAME=libND.a
OBJS=	init.o NDUser.o msg_send.o code.o boot.o server.o dfltserver.o \
	ND_kernserver.o ND_driver.o

INCLUDE_FILES= NDreg.h NDmsg.h NDlib.h ND_conio.h ND_thread_status.h \
		ND_machine.h NDGlobals.h

INSTALLED_INCLUDES = ../ND/NDreg.h ../ND/NDmsg.h ../ND/NDlib.h \
		../ND/ND_conio.h ../ND/ND_thread_status.h \
		../ND/ND_machine.h ../ND/NDGlobals.h

DUPS= ND.h NDUser.c ND_types.h
INSTALL=install -c -m 644
INSTALLH=install -c -m 644 -d

default:	all

all:	installh $(NAME)

# Since the project is self-contained, there is no need to install
# these files at this time.
install:
	@echo "Nothing to install here..."

#install:	installh $(NAME)
#	-mkdirs $(DESTINCDIR)
#	-mkdirs $(DESTLIBDIR)
#	for i in $(INCLUDE_FILES); do ( $(INSTALLH) $$i $(DESTINCDIR) ); done
#	cp $(NAME) $(DESTLIBDIR)
#	ranlib $(DESTLIBDIR)/$(NAME)
	
installh:	$(INSTALLED_INCLUDES)
	
install_local:	$(NAME)
	-mkdirs $(LOCALDESTINCDIR)
	-mkdirs $(LOCALDESTLIBDIR)
	for i in $(INCLUDE_FILES); do ( $(INSTALLH) $$i $(LOCALDESTINCDIR) ); done
	cp $(NAME) $(LOCALDESTLIBDIR)
	ranlib $(LOCALDESTLIBDIR)/$(NAME)

$(NAME):	$(OBJS)
	ar cr $(NAME) $(OBJS)
	ranlib $(NAME)
	

clean:
	rm -f *.o $(NAME) core $(DUPS)

NDUser.c:	ND.h	$(SERVERDIR)/NDUser.c
	cp $(SERVERDIR)/NDUser.c .

ND.h:	ND_types.h $(SERVERDIR)/ND.h
	cp $(SERVERDIR)/ND.h .

ND_types.h:	$(SERVERDIR)/ND_types.h
	cp $(SERVERDIR)/ND_types.h .

$(SERVERDIR)/ND.h $(SERVERDIR)/NDUser.c:
	( cd $(SERVERDIR) ; make )

$(OBJS):	$(INSTALLED_INCLUDES)

../ND/NDreg.h:		NDreg.h
	$(INSTALLH) NDreg.h $(INCDIR)/ND

../ND/NDmsg.h:		NDmsg.h
	$(INSTALLH) NDmsg.h $(INCDIR)/ND

../ND/NDGlobals.h:		NDGlobals.h
	$(INSTALLH) NDGlobals.h $(INCDIR)/ND

../ND/NDlib.h:		NDlib.h
	$(INSTALLH) NDlib.h $(INCDIR)/ND

../ND/ND_conio.h:	ND_conio.h
	$(INSTALLH) ND_conio.h $(INCDIR)/ND

../ND/ND_thread_status.h:	ND_thread_status.h
	$(INSTALLH) ND_thread_status.h $(INCDIR)/ND

../ND/ND_machine.h:	ND_machine.h
	$(INSTALLH) ND_machine.h $(INCDIR)/ND




