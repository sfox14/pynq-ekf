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
PROC := full
NAME := gps
ACC := arm-linux-gnueabihf-g++

# Target OS:
#     linux (Default), standalone
TARGET_OS := linux

# Build Directory
BUILD_DIR := $(BOARD)/$(OVERLAY)/$(PROC)/$(NAME)

# Current Directory
pwd := $(CURDIR)

# Include Libraries (NOT USED)
INSTALL_PATH :=
SDX_BUILD_PATH := $(INSTALL_PATH)/installs/lin64/SDx/2017.4
VIVADO_BUILD_PATH := $(INSTALL_PATH)/installs/lin64/Vivado/2017.4
IDIRS := -I$(SDX_BUILD_PATH)/target/aarch32-linux/include 
IDIRS += -I$(VIVADO_BUILD_PATH)/include

# Additional sds++ flags - this should be reserved for sds++ flags defined
# at run-time. Other sds++ options should be defined in the makefile data section below
ADDL_FLAGS := -Wno-unused-label

# Set to 1 (number one) to enable sds++ verbose output
VERBOSE :=

# Build Executable
EXECUTABLE := ekf_$(NAME).elf

# Build Dynamic Library
LIBRARY := libekf_$(NAME).so

# Vivado outputs
TCL_SOURCE := src/make_tcl.tcl
VIV_BIT := $(BUILD_DIR)/_sds/p0/vpl/system.bit
VIV_XPR := $(BUILD_DIR)/_sds/p0/_vpl/ipi/syn/syn.xpr
VIV_BD := $(BUILD_DIR)/_sds/p0/_vpl/ipi/syn/syn.srcs/sources_1/bd/system/system.bd
 
 
#+--------------------------------------------------------------------------
# Makefile Data
#+--------------------------------------------------------------------------

# Source Files
SRC_DIR := src/$(PROC)/$(NAME)
OBJECTS += \
$(pwd)/$(BUILD_DIR)/main.o \
$(pwd)/$(BUILD_DIR)/top_ekf.o \
$(pwd)/$(BUILD_DIR)/ekf.o

# Compiled Stub Files
DISTS += \
$(pwd)/$(BUILD_DIR)/_sds/swstubs/cf_stub.o \
$(pwd)/$(BUILD_DIR)/_sds/swstubs/portinfo.o \
$(pwd)/$(BUILD_DIR)/_sds/swstubs/top_ekf.o \
$(pwd)/$(BUILD_DIR)/ekf.o \


# SDS Options
HW_FLAGS :=
HW_FLAGS += -sds-hw top_ekf top_ekf.cpp -files $(pwd)/$(SRC_DIR)/ekf.cpp -clkid 0 -sds-end


# Compilation and Link Flags
IFLAGS := -I.
CFLAGS = -Wall -O3 -c -fPIC
CFLAGS += -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" 
CFLAGS += $(ADDL_FLAGS)
LFLAGS = "$@" "$<" 
#+---------------------------------------------------------------------

SDSFLAGS := -sds-pf $(PLATFORM) \
		-target-os $(TARGET_OS) 
ifeq ($(VERBOSE), 1)
SDSFLAGS += -verbose 
endif

# SDS Compiler
CPP := sds++ $(SDSFLAGS)
CC := sdscc $(SDSFLAGS)


.PHONY: all library
all: help $(BUILD_DIR)/$(EXECUTABLE) clean

elf: $(BUILD_DIR)/$(EXECUTABLE)

$(BUILD_DIR)/$(EXECUTABLE): $(OBJECTS)
	mkdir -p $(BUILD_DIR)
	@echo 'Building Target: $@'
	@echo 'Trigerring: SDS++ Linker'
	cd $(BUILD_DIR) ; $(CPP) -o $(EXECUTABLE) $(OBJECTS)
	@echo 'SDx Completed Building Target: $@'
	@echo 'Copy compiled stub files'
	mkdir -p dist/$(BOARD)/$(PROC)/$(NAME)
	cp $(DISTS) dist/$(BOARD)/$(PROC)/$(NAME)
	@echo 'Copy bitstream'
	mkdir -p ../boards/$(BOARD)/$(PROC)
	cp $(VIV_BIT) ../boards/$(BOARD)/$(PROC)/ekf_$(NAME).bit
	@echo 'Extract block design tcl from vivado project'
	vivado -mode batch -source $(TCL_SOURCE) -tclargs $(VIV_XPR) $(VIV_BD) ../boards/$(BOARD)/$(PROC)/ekf_$(NAME).tcl
	rm vivado*
	@echo ' '
	
$(pwd)/$(BUILD_DIR)/%.o: $(pwd)/$(SRC_DIR)/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: SDS++ Compiler'
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) ; $(CC) $(CFLAGS) -o $(LFLAGS) $(HW_FLAGS)
	@echo 'Finished building: $<'
	@echo ' '


install: 
	mkdir -p ../boards/$(BOARD)
	$(ACC) -g3 -gstabs -shared -fPIC -rdynamic $(shell find dist/$(BOARD)/$(NAME) -name '*.o') \
		-Wl,--start-group -l pthread /usr/lib/libsds_lib.so -Wl,--end-group \
		-o ../boards/$(BOARD)/$(LIBRARY)
	
	
.PHONY: cleanall clean
clean:
	$(RM) $(OBJECTS)

cleanall: clean
	$(RM) -rf ./Pynq-Z1 ./Pynq-Z2 ./Ultra96 .Xil


ECHO:= @echo

.PHONY: help
help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "	make all"
	$(ECHO) "		Command to compile source and generate stub object files. A shared object is created"
	$(ECHO)	"		during installation by linking these files with the Pynq board's 'libsds_lib.so' library"
	$(ECHO) ""
	$(ECHO) "	make elf"
	$(ECHO) "		Command to generate the .elf"
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
	@echo $(OBJECTS)
