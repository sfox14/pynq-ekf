#+-------------------------------------------------------------------------------
# Makefile for building SDSoC design
#
# Requirements:
#	1. source SDx
#	2. source Vivado
#+-------------------------------------------------------------------------------

BOARD := Pynq-Z1
PLATFORM := 
CLK_ID := 0
P_ENABLE := 0

ifeq ($(BOARD), "Pynq-Z1")
	P_ENABLE := 0
else
	P_ENABLE := 1
endif

.PHONY: all
ifeq ($(BOARD), "Pynq-Z1")
all: help check_env gps n8m4 n2m2
endif

ifeq ($(BOARD), "Ultra96")
all: help check_env gps n8m4 n2m2
endif

ifeq ($(BOARD), "ZCU104")
all: help gps n8m4 n2m2 n72m8
endif

gps: check_env
	make -f example.mk BOARD=$(BOARD) PLATFORM=$(PLATFORM) NAME=gps CLK_ID=$(CLK_ID) P_ENABLE=0

n2m2: check_env
	make -f example.mk BOARD=$(BOARD) PLATFORM=$(PLATFORM) NAME=n2m2 CLK_ID=$(CLK_ID) P_ENABLE=1

n8m4: check_env
	make -f example.mk BOARD=$(BOARD) PLATFORM=$(PLATFORM) NAME=n8m4 CLK_ID=$(CLK_ID) P_ENABLE=$(P_ENABLE)

n72m8: check_env
	make -f example.mk BOARD=$(BOARD) PLATFORM=$(PLATFORM) NAME=n72m8 CLK_ID=$(CLK_ID) P_ENABLE=1

info:
	sds++ -sds-pf-info $(PLATFORM)

clean: 
	rm -rf .Xil

cleanall: clean
	rm -rf Pynq-Z1 Ultra96 ZCU104

ECHO:= @echo

.SILENT:
check_env:
ifeq ($(shell "env" | grep "XILINX" | grep "SDX" | grep "2017.4"),)
	$(ECHO) "ERROR: Please source <SDx_directory>/installs/lin64/SDx/2017.4/settings64.sh"
	exit 1
endif
ifeq ($(shell "env" | grep "XILINX" | grep "Vivado" | grep "2017.4"),)
	$(ECHO) "ERROR: Please source <Vivado_directory>/installs/lin64/Vivado/2017.4/settings64.sh"
	exit 1
endif

.PHONY: help
help::
	$(ECHO) "Makefile Usage:"
	$(ECHO)
	$(ECHO) "example"
	$(ECHO) "-------"
	$(ECHO) "make BOARD=<target_board> PLATFORM=<platform_location>"
	$(ECHO) "   Compile source and generate stub object files for chose platform." 
	$(ECHO) "   A shared object is created during pip installation by linking these"
	$(ECHO) "   files with 'libsds_lib.so' on the board."
	$(ECHO)
	$(ECHO) "options"
	$(ECHO) "-------"
	$(ECHO) "info"
	$(ECHO) "   Get platform information, including available clock ID's"
	$(ECHO)
	$(ECHO) "clean"
	$(ECHO) "   Remove generated files for the specified board"
	$(ECHO)
	$(ECHO) "cleanall"
	$(ECHO) "   Remove all generated files"
	$(ECHO)
	$(ECHO) "Arguments"
	$(ECHO) "---------"
	$(ECHO) "PLATFORM" 
	$(ECHO) "   path to the platform (.pfm) file" 
	$(ECHO) "   eg. home/usr/sdx-platforms/Pynq-Z1/bare"
	$(ECHO) "BOARD"
	$(ECHO) "   name of the board (default is Pynq-Z1)"
	$(ECHO) "   eg. Pynq-Z1, Pynq-Z2, Ultra96, ZCU104"
	$(ECHO) "CLK_ID"
	$(ECHO) "   platform clock id"
	$(ECHO) "   ranging from 0 to the number of clocks specified in platform"
	$(ECHO) "P_ENABLE"
	$(ECHO) "   hardware configuration parameter. P_ENABLE=0 to fit on Pynq-Z1"
	$(ECHO) "   eg. 0 or 1 -> increases resource utilisation if equal to 1"
	$(ECHO)
