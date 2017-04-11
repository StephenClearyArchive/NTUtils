# Copyright 2005, Stephen Cleary
# See the accompanying file "ntutils.chm" for licence information

# Expects BOOST environment variable to be set to the location of the Boost libraries
#  (must use forward slashes)
INCLUDES = -I$(BOOST) -Iinclude
CFLAGS = -s -Os -mno-cygwin
FLAGS = $(CFLAGS) -fno-enforce-eh-specs -fno-inline
LFLAGS =

VERSION = 1.3.0

PROGRAMS = ntsuspend.exe ntpriority.exe

all: $(PROGRAMS) ntutils.chm

$(PROGRAMS): %.exe: src/%.cpp src/%.inc src/include/basic/*.h src/include/ntutils/*.h
	cd src; \
	g++ $(INCLUDES) $(FLAGS) $(LFLAGS) -o ../$@ $(notdir $<) -lmpr
	upx --best $@

ntutils.chm: docs/ntutils.hhp docs/ntutils.hhc docs/*.html docs/*.css
	-cd docs; \
	hhc ntutils.hhp
	mv docs/ntutils.chm .

dist: ntutils-$(VERSION)-bin.tar.gz ntutils-$(VERSION)-src.tar.gz

ntutils-$(VERSION)-bin.tar.gz: $(PROGRAMS) ntutils.chm
	rm -rf ntutils-$(VERSION)-bin.tar.gz ntutils-$(VERSION)-bin.tar
	tar cf ntutils-$(VERSION)-bin.tar $^
	gzip ntutils-$(VERSION)-bin.tar

ntutils-$(VERSION)-src.tar.gz: $(PROGRAMS) ntutils.chm
	rm -rf ntutils-$(VERSION)-src.tar.gz ntutils-$(VERSION)-src.tar
	tar cf ntutils-$(VERSION)-src.tar docs/* src/* makefile ntutils.chm
	gzip ntutils-$(VERSION)-src.tar
