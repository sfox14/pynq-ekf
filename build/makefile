#+-------------------------------------------------------------------------------
# Makefile for building SDSoC design
#
# Requirements:
#	1. source SDx 2017.4
#	2. source Vivado 2017.4
#+-------------------------------------------------------------------------------

BOARD := Pynq-Z1
OVERLAY := bare
PLATFORM := 
NAME :=
CC := arm-linux-gnueabihf-g++

.PHONY: all gps n2m2 n8m4
all: help gps

gps:
	make -f example.mk BOARD=$(BOARD) OVERLAY=$(OVERLAY) PLATFORM=$(PLATFORM) PROC=full NAME=gps

n2m2:
	make -f example.mk BOARD=$(BOARD) OVERLAY=$(OVERLAY) PLATFORM=$(PLATFORM) PROC=base NAME=n2m2

n8m4:
	make -f example.mk BOARD=$(BOARD) OVERLAY=$(OVERLAY) PLATFORM=$(PLATFORM) PROC=base NAME=n8m4

install:
	make -f example.mk install BOARD=$(BOARD) NAME=$(NAME) ACC=$(CC)
	
ECHO:= @echo

.PHONY: help
help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "	make all"
	$(ECHO) "		Command to compile source and generate stub object files. A shared object is created"
	$(ECHO)	"		during pip installation by linking these files with 'libsds_lib.so' on the PYNQ board"
	$(ECHO) ""
	$(ECHO) "	make clean"
	$(ECHO) "		Command to remove the generated non-hardware files."
	$(ECHO) ""
	$(ECHO) "	make cleanall"
	$(ECHO) "		Command to remove all the generated files."
	$(ECHO) ""
	$(ECHO) " 	Arguments:"
	$(ECHO) "	----------"
	$(ECHO) "	PLATFORM:	path to the platform (.pfm) file" 
	$(ECHO) "					eg. home/usr/sdx-platforms/Pynq-Z1/bare"
	$(ECHO) "	OVERLAY:	name of the overlay (default=bare)" 
	$(ECHO) "					eg. bare, hdmi, ultra"
	$(ECHO) "	BOARD:		name of the board (default=Pynq-Z1)"
	$(ECHO) "					eg. Pynq-Z1, Pynq-Z2, Ultra96"
	$(ECHO) ""
	$(ECHO) "	Additional Arguments (for setup.py):"
	$(ECHO) "	------------------------------------"
	$(ECHO) "	NAME:		name of the design"
	$(ECHO) "					eg. gps, n2m2"
	$(ECHO) "	CC:		target board compiler"
	$(ECHO) "					eg. arm-linux-gnueabihf-g++, aarch64-linux-gnu-g++"
	$(ECHO) ""
