# Copyright 2005, Stephen Cleary
# See the accompanying file "readme.html" for licence information

# Expects BOOST environment variable to be set to the location of the Boost libraries
INCLUDES = -I$(BOOST) -Iinclude
CFLAGS = -s -Os -mno-cygwin
FLAGS = $(CFLAGS) -fno-enforce-eh-specs
LFLAGS =

VERSION = 1.0

all: ntsuspend.exe

ntsuspend.exe: ntsuspend.cpp
	g++ $(INCLUDES) $(FLAGS) $(LFLAGS) -o ntsuspend.exe ntsuspend.cpp -lmpr
	upx --best ntsuspend.exe

dist: ntutils-$(VERSION).tar.gz ntutils-$(VERSION)-src.tar.gz

ntutils-$(VERSION).tar.gz: all
	rm -rf ntutils-$(VERSION).tar.gz ntutils-$(VERSION).tar
	tar cf ntutils-$(VERSION).tar ntsuspend.exe readme.html docs/*
	gzip ntutils-$(VERSION).tar

ntutils-$(VERSION)-src.tar.gz: all
	rm -rf ntutils-$(VERSION)-src.tar.gz ntutils-$(VERSION)-src.tar
	tar cf ntutils-$(VERSION)-src.tar ntsuspend.cpp readme.html makefile docs/* include/*
	gzip ntutils-$(VERSION)-src.tar
