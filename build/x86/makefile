#+-------------------------------------------------------------------------------
# Makefile for building SDSoC design
#
# Requirements:
#	1. source SDx 2017.4
#	2. source Vivado 2017.4
#+-------------------------------------------------------------------------------

BOARD := Pynq-Z1 
PLATFORM :=
CLK_ID := 0 

.PHONY: all gps n2m2 n8m4
all: help n8m4

gps:
	make -f example.mk BOARD=$(BOARD) PLATFORM=$(PLATFORM) DESIGN=hw NAME=gps CLK_ID=$(CLK_ID)

n2m2:
	make -f example.mk BOARD=$(BOARD) PLATFORM=$(PLATFORM) DESIGN=hw-sw NAME=n2m2 CLK_ID=$(CLK_ID)

n8m4:
	make -f example.mk BOARD=$(BOARD) PLATFORM=$(PLATFORM) DESIGN=hw-sw NAME=n8m4 CLK_ID=$(CLK_ID)

clean:
	rm -rf ./$(BOARD) .Xil
	
cleanall: clean
	rm -rf ./Pynq-Z1 ./Pynq-Z2 ./Ultra96 ./ZCU104

ECHO:= @echo

.PHONY: help
help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "	make all"
	$(ECHO) "		Command to compile source and generate stub object files. A shared object is created"
	$(ECHO)	"		during pip installation by linking these files with 'libsds_lib.so' on the PYNQ board"
	$(ECHO) ""
	$(ECHO) "	make clean"
	$(ECHO) "		Command to remove generated files for the specified board"
	$(ECHO) ""
	$(ECHO) "	make cleanall"
	$(ECHO) "		Command to remove all generated files"
	$(ECHO) ""
	$(ECHO) " 	Arguments:"
	$(ECHO) "	----------"
	$(ECHO) "	PLATFORM:	path to the platform (.pfm) file" 
	$(ECHO) "					eg. home/usr/sdx-platforms/Pynq-Z1/bare"
	$(ECHO) "	BOARD:		name of the board (default=Pynq-Z1)"
	$(ECHO) "					eg. Pynq-Z1, Pynq-Z2, Ultra96, ZCU104"
	$(ECHO) "	CLK_ID:		platform clock id"
	$(ECHO) "					eg. 0, 1 -> number of clocks specified in platform"
	$(ECHO) ""
