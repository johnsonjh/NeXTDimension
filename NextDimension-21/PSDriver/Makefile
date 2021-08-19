#
# PSDriver root makefile
#
# Specify SUBDIRS in the order that they should be built.
SUBDIRS = common client bitmap devpattern device server

default: all

all:
	for i in $(SUBDIRS); do ( cd $$i && $(MAKE) $(MAKEOPTS) $@ ); done
	
install:
	for i in $(SUBDIRS); do ( cd $$i && $(MAKE) $(MAKEOPTS) $@ ); done

install_local:

installh:

clean:
	for i in $(SUBDIRS); do ( cd $$i && $(MAKE) $(MAKEOPTS) clean ); done
