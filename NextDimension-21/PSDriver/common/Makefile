#
# MIG Package Makefile
#

CFILES = argstream.c markpack.c

OTHER_SOURCES = ps.defs

include ../config-cpu
include ../Makefile.common

MIGGENERATED = psUser.c psServer.c ps.h

all: $(MIGGENERATED)

$(MIGGENERATED): ps.defs
	mig $(MIGFLAGS) $(INCLUDES) ps.defs

install: all

installh:

install_local:

clean:
	-rm -rf $(MIGGENERATED) make.log
