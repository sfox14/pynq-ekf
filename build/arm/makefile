#+-----------------------------------------------------------------------------
# Makefile for building shared library (.so) on the PYNQ board
#+-----------------------------------------------------------------------------

BOARD := Pynq-Z1
NAME := gps

CPP := g++
LIBRARY := libekf_$(NAME).so
ECHO:= @echo

.PHONY: all install help

all: install

install: 
	mkdir -p ../../boards/$(BOARD)
	$(CPP) -shared -fPIC -rdynamic ../x86/dist/$(BOARD)/$(NAME)/*.o \
		-l pthread /usr/lib/libsds_lib.so  \
		-o ../../boards/$(BOARD)/$(NAME)/$(LIBRARY)

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO)
	$(ECHO) "make BOARD=<board_name> NAME=<design_name>"
	$(ECHO) "   Link objects and generate shared library (*.so) file."
	$(ECHO)
	$(ECHO) "Arguments"
	$(ECHO) "---------"
	$(ECHO) "BOARD" 
	$(ECHO) "   board name" 
	$(ECHO) "   eg. Pynq-Z1, Ultra96, ZCU104"
	$(ECHO) "NAME" 
	$(ECHO) "   name of the design (default is gps)" 
	$(ECHO) "   eg. gps, n2m2, n8m4"
	$(ECHO)
