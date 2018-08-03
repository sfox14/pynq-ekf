#+-------------------------------------------------------------------------------
# Makefile for building SDSoC design
#
# Requirements:
#	1. source SDx 2017.4
#	2. source Vivado 2017.4
#+-------------------------------------------------------------------------------

BOARD := 
PLATFORM :=
CLK_ID := 0 
P_ENABLE := 0

ifeq ($(BOARD), "Pynq-Z1")
	P_ENABLE := 0
else
	P_ENABLE := 1
endif

.PHONY: all
all: help

gps: check_env
	make -f example.mk BOARD=$(BOARD) PLATFORM=$(PLATFORM) DESIGN=hw NAME=gps CLK_ID=$(CLK_ID) P_ENABLE=0

n2m2: check_env
	make -f example.mk BOARD=$(BOARD) PLATFORM=$(PLATFORM) DESIGN=hw-sw NAME=n2m2 CLK_ID=$(CLK_ID) P_ENABLE=1

n8m4: check_env
	make -f example.mk BOARD=$(BOARD) PLATFORM=$(PLATFORM) DESIGN=hw-sw NAME=n8m4 CLK_ID=$(CLK_ID) P_ENABLE=$(P_ENABLE)

n72m8: check_env
	make -f example.mk BOARD=$(BOARD) PLATFORM=$(PLATFORM) DESIGN=hw-sw NAME=n72m8 CLK_ID=$(CLK_ID) P_ENABLE=1

	
zcu104: n8m4 gps n2m2 n72m8 
	
z1: gps n8m4 n2m2

ultra96: n8m4 gps n2m2	

	
info:
	sds++ -sds-pf-info $(PLATFORM)
	
clean: 
	rm -rf ./$(BOARD) .Xil
	
cleanall: clean
	rm -rf ./Pynq-Z1 ./Pynq-Z2 ./Ultra96 ./ZCU104

	
ECHO:= @echo

.SILENT:
check_env:
ifeq ($(shell "env" | grep "XILINX" | grep "SDX" | grep "2017.4"),)
	$(ECHO) ""
	$(ECHO) "NOT FOUND ERROR: SDX 2017.4 "
	$(ECHO) "	run 'source <SDx Installation Directory>/installs/lin64/SDx/2017.4/settings64.sh'"
	$(ECHO) ""
endif
ifeq ($(shell "env" | grep "XILINX" | grep "Vivado" | grep "2017.4"),)
	$(ECHO) "NOT FOUND ERROR: Vivado 2017.4 "
	$(ECHO) "	run 'source <Vivado Installation Directory>/installs/lin64/Vivado/2017.4/settings64.sh'"
	$(ECHO) ""
endif


.PHONY: help
help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "	make {z1 or zcu104 or ultra96}"
	$(ECHO) "		Command to compile source and generate stub object files for chose platform. A shared" 
	$(ECHO) "		object is created during pip installation by linking these files with 'libsds_lib.so'"
	$(ECHO)	"		on the PYNQ board"
	$(ECHO) ""
	$(ECHO) "	make info"
	$(ECHO) "		Command to get platform information, incl. available clock id's"
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
	$(ECHO) "	P_ENABLE:	hardware configuration parameter. P_ENABLE=0 to fit on Pynq-Z1"
	$(ECHO) "					eg. 0 or 1 -> increases resource utilisation if equal to 1"
	$(ECHO) ""
