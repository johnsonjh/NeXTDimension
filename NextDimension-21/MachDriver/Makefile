NAME=NextDimension
SCRIPTFILE=NextDimension.kern_server
COPTS=
DRIVERDIR=$(DSTROOT)/usr/lib/kern_loader/NextDimension
CFILES= ND_intr.c ND_server.c NDServer.c ND_reply.c  \
	ND_map_hw.c ND_init_hw.c ND_msg_io.c ND_locks.c ND_cursor.c \
	ND_threads.c ND_services.c ND_KernServer.c ND_Kern_services.c \
	ND_pmap.c ND_rom.c
SFILES=
OFILES= ND_intr.o ND_server.o NDServer.o ND_reply.o  \
	ND_map_hw.o ND_init_hw.o ND_msg_io.o ND_locks.o ND_cursor.o \
	ND_threads.o ND_services.o ND_KernServer.o ND_Kern_services.o \
	ND_pmap.o ND_rom.o

DESTINCDIR=$(DSTROOT)/usr/local/include/ND
LOCALDESTINCDIR=$(DSTROOT)/Net/k9/dps/nd_proj/include/ND

INCLUDE_FILES= ND_id.h  ND_types.h ND.h ND_Kern.h
MIGINCLUDES= NDServer.c ND.h ND_KernServer.c ND_Kern.h
MIG_GENERATED= $(MIGINCLUDES) NDUser.c ND_KernUser.c

#
#  Things to be done just before exiting
#
MD_SUFFIX=d
MD=	md

EXIT_RULE=( touch M.${MD_SUFFIX} ; ${MD} -f -o Makedep -d `ls *.${MD_SUFFIX}` ; rm -f M.${MD_SUFFIX} )

PROJECT_INCLUDES=-I..
SOURCEFILES= Makefile $(CFILES) $(SFILES) $(SCRIPTFILE) $(HFILES)
INSTALL=install -c -m 644
INSTALLH=install -c -d -m 644
MAKEOPTS= $(MFLAGS) "DSTROOT=$(DSTROOT)"

MIGFLAGS=$(COPTS) $(PROJECT_INCLUDES)
CFLAGS= $(COPTS) -O -MD -DKERNEL -DKERNEL_FEATURES -DMACH \
	-DCURSOR_i860\
	$(PROJECT_INCLUDES)

all:	$(NAME)_reloc

debug:
	$(MAKE) $(MAKEOPTS) "CFLAGS=-g $(CFLAGS)" debug_version

install:	all
	-mkdirs $(DRIVERDIR)
	$(INSTALL) $(NAME)_reloc $(DRIVERDIR)
#	-mkdirs $(DESTINCDIR)
#	for i in $(INCLUDE_FILES); do ( $(INSTALLH) $$i $(DESTINCDIR) ); done
	

installh:	../ND/ND_types.h ../ND/ND_id.h ../ND/ND.h ../ND/ND_Kern.h

install_local:  all
	-mkdirs $(DRIVERDIR)
	$(INSTALL) $(NAME)_reloc $(DRIVERDIR)
	-mkdirs $(LOCALDESTINCDIR)
	for i in $(INCLUDE_FILES); do ( $(INSTALLH) $$i $(LOCALDESTINCDIR) ); done

installsrc: $(SRCDIR)
	tar cf - $(SOURCEFILES) | (cd $(SRCDIR); tar xpf -)

$(SRCDIR):
	mkdirs $@

clean:
	-rm -rf $(NAME)_reloc $(SOFILES) $(UOFILES) \
		$(MIGCFILES) $(MIG_GENERATED) *.d *.o \
		Makedep ND_gamma.h gamma $(NAME)_loadable core test .places

Makefile: *.${MD_SUFFIX}
	@rm -f Makefile.cmp; cp Makefile Makefile.cmp
	${EXIT_RULE}

.EXIT:
	${EXIT_RULE}

depend: 
	@echo you don\'t make depend any more.

$(NAME)_reloc:	$(OFILES) Load_Commands.seg Unload_Commands.seg
	kl_ld -n $(NAME) -i instance -l Load_Commands.seg \
		-u Unload_Commands.seg \
		-o $(NAME)_reloc -x $(OFILES)


debug_version: $(OFILES) Load_Commands.seg Unload_Commands.seg $(NAME)_loadable
	kl_ld -n $(NAME) -i instance -l Load_Commands.seg -u Unload_Commands.seg \
		-d $(NAME)_loadable -o $(NAME)_reloc $(OFILES)
#
# kl_ld insists on this file existing before it will add the sect directing the
# kern_loader to create this file.  Why, I don't know....
$(NAME)_loadable:
	touch $(NAME)_loadable

$(NAME)_reloc.oldway : $(OFILES) Server_Name.seg Loadable_Name.seg \
		Version_String.seg Executable_Name.seg Instance_Var.seg \
		Load_Commands.seg Unload_Commands.seg
	ld -r $(OFILES) -o $@ $(SOFILES) \
	-sectalign  "Loaded Server" "Server Name" 1 \
	-sectcreate "Loaded Server" "Server Name" Server_Name.seg \
	-sectalign  "Loaded Server" "Load Commands" 1 \
	-sectcreate "Loaded Server" "Load Commands" Load_Commands.seg \
	-sectalign  "Loaded Server" "Unload Commands" 1 \
	-sectcreate "Loaded Server" "Unload Commands" Unload_Commands.seg \
	-sectalign  "Loaded Server" "Loadable Name" 1 \
	-sectcreate "Loaded Server" "Loadable Name" Loadable_Name.seg \
	-sectalign  "Loaded Server" "Executable Name" 1 \
	-sectcreate "Loaded Server" "Executable Name" Executable_Name.seg \
	-sectalign  "Loaded Server" "Instance Var" 1 \
	-sectcreate "Loaded Server" "Instance Var" Instance_Var.seg

Server_Name.seg:
	echo -n $(NAME) > $@

Loadable_Name.seg:
	echo -n $(NAME)_loadable > $@

Executable_Name.seg:
	echo -n /mach > $@

Instance_Var.seg:
	echo -n instance > $@

Version_String.seg:
	echo Unknown Version > $@


ND_tt1.o:	ND_tt1.s
	cc -E -DASSEMBLER ND_tt1.s > ND_tt1.i
	as -o ND_tt1.o ND_tt1.i
	rm -f ND_tt1.i

ND_handler.o: NDServer.c

ND.defs: ../ND/ND_id.h ../ND/ND_types.h

NDServer.c ND.h: ND.defs
	mig ${MIGFLAGS} ND.defs

ND_Kern.defs: ../ND/ND_types.h

ND_KernServer.c ND_KernUser.c ND_Kern.h: ND_Kern.defs
	mig ${MIGFLAGS} ND_Kern.defs

test:	test.o ../libND/libND.a
	cc -o test test.o ../libND/libND.a -lNeXT_s -lsys_s
	
test.o:	test.c ND.h
	cc -g -c -O -MD $(PROJECT_INCLUDES) test.c
	
../libND/libND.a:	NDUser.c ND.h
	( cd ../libND ; make )
	
ND_gamma.h:	gamma
	./gamma 2.0 > ND_gamma.h

gamma:	gamma.c
	cc -o gamma gamma.c -lsys_s

ND_init_hw.o:	ND_gamma.h

../ND/ND_id.h:	ND_id.h	
	-mkdirs ../ND
	$(INSTALLH) ND_id.h ../ND

../ND/ND_types.h:	ND_types.h
	-mkdirs ../ND
	$(INSTALLH) ND_types.h ../ND

../ND/ND.h:	ND.h
	-mkdirs ../ND
	$(INSTALLH) ND.h ../ND

../ND/ND_Kern.h:	ND_Kern.h
	-mkdirs ../ND
	$(INSTALLH) ND_Kern.h ../ND

-include Makedep


