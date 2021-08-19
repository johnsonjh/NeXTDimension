#
# Root makefile for a multi-directory project
#
# Specify SUBDIRS in the order that they should be built.
SUBDIRS= MachDriver libND cmds NDkernel PSDriver diag ROM
SOURCEFILES=Makefile $(SUBDIRS) include.i860 ND RelNotes
SRCPROTECTION=777
MAKEOPTS= $(MFLAGS) "DSTROOT=$(DSTROOT)" "COPTS=$(COPTS)"
DSTROOT=
RELNOTES_DIR=$(DSTROOT)/usr/local/RelNotes
RELNOTES=RelNotes/NextDimensionNotes.wn
#
# Everything we want in an internal update for ND project developers.
#
UPDATE_DIR= /Net/k9/dps/nd_proj/update
UPDATE_STUFF=	/usr/lib/kern_loader/NextDimension/NextDimension_reloc \
		/usr/lib/NextStep/Displays/NextDimension/reloc \
		/usr/lib/NextStep/Displays/NextDimension/movei860

default:	all

all:	installh
	for i in $(SUBDIRS); do ( cd $$i && $(MAKE) $(MAKEOPTS) $@ ); done
	
install:	installh
	-mkdirs $(RELNOTES_DIR)
	for i in $(SUBDIRS); do ( cd $$i && $(MAKE) $(MAKEOPTS) $@ ); done
	cp $(RELNOTES) $(RELNOTES_DIR)

installh: ND
	for i in $(SUBDIRS); do ( cd $$i && $(MAKE) $(MAKEOPTS) installh ); \
	done

install_local:	installh
	for i in $(SUBDIRS); do ( cd $$i && $(MAKE) $(MAKEOPTS) $@ ); done
#
# Build an nd_proj update tar archive!
#
update:	install $(UPDATE_DIR)
	-/bin/rm -f $(UPDATE_DIR)/NextDimension-*
	tar cf $(UPDATE_DIR)/`vers_string -f NextDimension` $(UPDATE_STUFF)
	
$(UPDATE_DIR)::
	-mkdirs $(UPDATE_DIR)

clean::	ND
	for i in $(SUBDIRS); do ( cd $$i && $(MAKE) $(MAKEOPTS) clean ); done
	cd ND ; /bin/rm -f *.h

installsrc: SRCROOT
	-/bin/rm -rf $(SRCROOT)
	mkdirs $(SRCROOT)
	chmod 755 $(SRCROOT)
	tar cf - $(SOURCEFILES) | (cd $(SRCROOT); tar xf -)
	chmod $(SRCPROTECTION) $(SRCROOT)/*

help::
	@echo '  all (the default)'
	@echo '  clean'
	@echo '  installsrc SRCROOT=somepath'
	@echo '  install [DSTROOT=somepath]'
	@echo '  clean'
	@echo '  depend'

#
#
ND::
	-mkdirs ND

SRCROOT:
	@if [ -n "${$@}" ]; then exit 0; \
	else echo Must define $@; exit 1; fi







