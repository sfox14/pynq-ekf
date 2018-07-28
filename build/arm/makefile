#+-------------------------------------------------------------------------------
# Makefile for building shared library (.so) on the PYNQ board
#+-------------------------------------------------------------------------------

BOARD := Pynq-Z1
NAME :=
CC := arm-linux-gnueabihf-g++

LIBRARY := libekf_$(NAME).so
ECHO:= @echo

.PHONY: all install help

all: install

install: 
	mkdir -p ../../boards/$(BOARD)
	$(CC) -shared -fPIC -rdynamic $(shell find ../x86/dist/$(BOARD)/$(NAME) -name '*.o') \
		-l pthread /usr/lib/libsds_lib.so  \
		-o ../../boards/$(BOARD)/$(LIBRARY)

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) ""
	$(ECHO) "	make"
	$(ECHO) "		Command to link objects and generate shared library (*.so) file."
	$(ECHO) ""
	$(ECHO) " 	Arguments:"
	$(ECHO) "	----------"
	$(ECHO) "	PLATFORM:	path to the platform (.pfm) file" 
	$(ECHO) "					eg. home/usr/sdx-platforms/Pynq-Z1/bare"
	$(ECHO) "	NAME:		name of the design (default=bare)" 
	$(ECHO) "					eg. gps, n2m2, n8m4"
	$(ECHO) "	CC:		target compiler"
	$(ECHO) "					eg. arm-linux-gnueabihf-g++, aarch64-linux-gnu-g++"
	$(ECHO) ""
