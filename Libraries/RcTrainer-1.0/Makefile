# Makefile for Arduino libraries

DISTFILE = VirtualWire-1.10.zip

all:	dist upload

ci:
	ci -l `cat VirtualWire/MANIFEST`

dist:	
	zip $(DISTFILE) `cat VirtualWire/MANIFEST`

upload:
	scp $(DISTFILE) ../doc/VirtualWire.pdf server2:/var/www/html/mikem/arduino
