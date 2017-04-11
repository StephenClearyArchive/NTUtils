# Copyright 2005, Stephen Cleary
# See the accompanying file "ntutils.chm" for licence information

# Expects BOOST environment variable to be set to the location of the Boost libraries
INCLUDES = -I$(BOOST) -Iinclude
CFLAGS = -s -Os -mno-cygwin
FLAGS = $(CFLAGS) -fno-enforce-eh-specs
LFLAGS =

VERSION = 1.1

PROGRAMS = ntsuspend.exe

all: $(PROGRAMS) ntutils.chm

*.exe: %.exe: %.cpp include/ntutils/%.inc include/basic/*.h include/ntutils/*.h
	g++ $(INCLUDES) $(FLAGS) $(LFLAGS) -o $@ $< -lmpr
	upx --best ntsuspend.exe

ntutils.chm: ntutils.hhp ntutils.hhc docs/*.html docs/*.css
	-hhc ntutils.hhp

dist: ntutils-$(VERSION)-bin.tar.gz ntutils-$(VERSION)-src.tar.gz

ntutils-$(VERSION)-bin.tar.gz: $(PROGRAMS) ntutils.chm
	rm -rf ntutils-$(VERSION)-bin.tar.gz ntutils-$(VERSION)-bin.tar
	tar cf ntutils-$(VERSION)-bin.tar *.exe ntutils.chm
	gzip ntutils-$(VERSION)-bin.tar

ntutils-$(VERSION)-src.tar.gz: $(PROGRAMS) ntutils.chm
	rm -rf ntutils-$(VERSION)-src.tar.gz ntutils-$(VERSION)-src.tar
	tar cf ntutils-$(VERSION)-src.tar docs/* include/* *.cpp makefile ntutils.chm ntutils.hhp ntutils.hhc
	gzip ntutils-$(VERSION)-src.tar
