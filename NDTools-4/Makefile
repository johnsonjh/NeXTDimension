#
# Root makefile for a multi-directory project
#
# Specify SUBDIRS in the order that they should be built.
SUBDIRS= as gcc ld otool unixsyms runtime860
OTHERDIRS= gcc-1.36 gdb include
SOURCEFILES=Makefile $(SUBDIRS) $(OTHERDIRS)
SRCPROTECTION=777
MAKEOPTS= $(MFLAGS) "DSTROOT=$(DSTROOT)"
RELNOTES_DIR=/usr/local/RelNotes
RELNOTES=
#
# Everything we want in an internal update for ND project developers.
#
UPDATE_DIR= ../../update
UPDATE_STUFF=

default:	all

all:
	for i in $(SUBDIRS); do ( cd $$i && $(MAKE) $(MAKEOPTS) $@ ); done
	
install:
	for i in $(SUBDIRS); do ( cd $$i && $(MAKE) $(MAKEOPTS) $@ ); done

install_local:
	for i in $(SUBDIRS); do ( cd $$i && $(MAKE) $(MAKEOPTS) $@ ); done
#
# Build an nd_proj update tar archive!
#
update:	install_local $(UPDATE_DIR)
	-touch $(UPDATE_DIR)/NDTools-0
	-/bin/rm -f $(UPDATE_DIR)/NDTools-*
	tar cf ../../update/`vers_string -f NDTools` $(UPDATE_STUFF)
	
clean::
	for i in $(SUBDIRS); do ( cd $$i && $(MAKE) $(MAKEOPTS) clean ); done

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

#
#
SRCROOT:
	@if [ -n "${$@}" ]; then exit 0; \
	else echo Must define $@; exit 1; fi







