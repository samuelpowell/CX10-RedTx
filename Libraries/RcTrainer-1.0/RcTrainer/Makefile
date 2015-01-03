# Makefile
#
# Makefile for the Arduino RcTrainer project
#
# Author: Mike McCauley (mikem@airspayce.com)
# Copyright (C) 2011 Mike McCauley
# $Id: Makefile,v 1.6 2012/09/05 23:31:39 mikem Exp mikem $

PROJNAME = RcTrainer
# Dont forget to also change the version at the top of RF22.h:
DISTFILE = $(PROJNAME)-1.0.zip

all:	doxygen dist upload

doxygen: 
	doxygen project.cfg

ci:
	(cd ..;ci -l `cat $(PROJNAME)/MANIFEST`)

dist:	
	(cd ..; zip $(PROJNAME)/$(DISTFILE) `cat $(PROJNAME)/MANIFEST`)

upload:
	rsync -avz $(DISTFILE) doc/ www.airspayce.com:public_html/mikem/arduino/$(PROJNAME)
