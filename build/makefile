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
TOOL_VERSION := 2018.2
ECHO := @echo

# set P_ENABLE to 1 for small problem size on Zynq Ultrascale devices
# set P_CACHEABLE to 1 for Zynq Ultrascale devices
ifeq ($(BOARD),Pynq-Z1)
proj := gps n8m4 n2m2
P_CACHEABLE = 0
P_ENABLE = 0
endif
ifeq ($(BOARD),Pynq-Z2)
proj := gps n8m4 n2m2
P_CACHEABLE = 0
P_ENABLE = 0
endif
ifeq ($(BOARD),Ultra96)
proj := gps n8m4 n2m2
P_CACHEABLE = 1
P_ENABLE = 1
endif
ifeq ($(BOARD),ZCU104)
proj += gps n8m4 n2m2 n72m8
P_CACHEABLE = 1
P_ENABLE = 1
endif

.PHONY: all
all : help check_env $(proj)
	$(ECHO) "Projects for $(BOARD) built successfully!"

gps:
	make -f example.mk BOARD=$(BOARD) PLATFORM=$(PLATFORM) NAME=gps \
	CLK_ID=$(CLK_ID) P_ENABLE=0 \
	P_CACHEABLE=$(P_CACHEABLE)

n2m2:
	make -f example.mk BOARD=$(BOARD) PLATFORM=$(PLATFORM) NAME=n2m2 \
	CLK_ID=$(CLK_ID) P_ENABLE=1 \
	P_CACHEABLE=$(P_CACHEABLE)

n8m4:
	make -f example.mk BOARD=$(BOARD) PLATFORM=$(PLATFORM) NAME=n8m4 \
	CLK_ID=$(CLK_ID) P_ENABLE=$(P_ENABLE) \
	P_CACHEABLE=$(P_CACHEABLE)

n72m8:
	make -f example.mk BOARD=$(BOARD) PLATFORM=$(PLATFORM) NAME=n72m8 \
	CLK_ID=$(CLK_ID) P_ENABLE=1 \
	P_CACHEABLE=$(P_CACHEABLE)

info:
	sds++ -sds-pf-info $(PLATFORM)

clean: 
	rm -rf .Xil

cleanall: clean
	rm -rf Pynq-Z1
	rm -rf Pynq-Z2
	rm -rf Ultra96
	rm -rf ZCU104

.SILENT:
check_env:
ifeq ($(shell "env" | grep "XILINX" | grep "SDX" | grep $(TOOL_VERSION)),)
	$(ECHO) "ERROR: Please source SDx $(TOOL_VERSION) settings."
	exit 1
endif
ifeq ($(shell "env" | grep "XILINX" | grep "Vivado" | grep $(TOOL_VERSION)),)
	$(ECHO) "ERROR: Please source Vivado $(TOOL_VERSION) settings."
	exit 1
endif

help:
	$(ECHO) "Makefile Usage:"
	$(ECHO)
	$(ECHO) "example"
	$(ECHO) "-------"
	$(ECHO) "make BOARD=<target_board> PLATFORM=<platform_location>"
	$(ECHO) "   Compile source and generate stub object files for chose platform." 
	$(ECHO) "   A shared object is created during pip installation by linking these"
	$(ECHO) "   files with 'libcma.so' on the board."
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
	$(ECHO) "Internal Arguments"
	$(ECHO) "------------------"
	$(ECHO) "P_ENABLE"
	$(ECHO) "   hardware configuration parameter. P_ENABLE=0 to fit on Pynq-Z1"
	$(ECHO) "   eg. 0 or 1 -> increases resource utilisation if equal to 1"
	$(ECHO) "P_CACHEABLE"
	$(ECHO) "   whether to allocate cacheable or non-cacheable for sds calls"
	$(ECHO) "   a good practice is to always use non-cacheable on Zynq, and"
	$(ECHO) "   use cacheable on Zynq Ultrascale for best performance"
	$(ECHO) "TOOL_VERSION"
	$(ECHO) "   Version of Xilinx tools (SDx and Vivado)"
	$(ECHO)
	$(ECHO) "User Arguments"
	$(ECHO) "--------------"
	$(ECHO) "PLATFORM" 
	$(ECHO) "   path to the SDx platform (.pfm) file" 
	$(ECHO) "   eg. home/usr/sdx-platforms/Pynq-Z1/bare"
	$(ECHO) "BOARD"
	$(ECHO) "   name of the board (default is Pynq-Z1)"
	$(ECHO) "   Currently supported board: "
	$(ECHO) "       Pynq-Z1"
	$(ECHO) "       Pynq-Z2"
	$(ECHO) "       Ultra96"
	$(ECHO) "       ZCU104"
	$(ECHO) "CLK_ID"
	$(ECHO) "   platform clock id"
	$(ECHO) "   ranging from 0 to the number of clocks specified in platform"
	$(ECHO)
