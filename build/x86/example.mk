#+-------------------------------------------------------------------------------
# Makefile for building SDSoC design
#
# Requirements:
#	1. source SDx 2017.4
#	2. source Vivado 2017.4
#+-------------------------------------------------------------------------------

BOARD :=
PLATFORM :=
NAME := gps
CLK_ID := 0
P_ENABLE ?= 0

# Target OS:
#     linux (Default), standalone
TARGET_OS := linux

# Build Directory
BUILD_DIR := $(BOARD)/$(NAME)

# Current Directory
pwd := $(CURDIR)

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
VIV_BIT := $(BUILD_DIR)/$(LIBRARY).bit
VIV_TCL := $(BUILD_DIR)/_sds/p0/_vpl/ipi/sources_1/bd/*/hw_handoff/*_bd.tcl

#+--------------------------------------------------------------------------
# Makefile Data
#+--------------------------------------------------------------------------

# Source Files
SRC_DIR := src/$(NAME)
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
HW_FLAGS += -sds-hw top_ekf top_ekf.cpp -files $(pwd)/$(SRC_DIR)/ekf.cpp -clkid $(CLK_ID) -sds-end


# Compilation and Link Flags
IFLAGS := -I.
CFLAGS = -Wall -O3 -c -fPIC
CFLAGS += -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" 
CFLAGS += $(ADDL_FLAGS)
LFLAGS = "$@" "$<" 
#+---------------------------------------------------------------------

CONFIG_FLAGS := -DP_ENABLE=${P_ENABLE}

SDSFLAGS := -sds-pf $(PLATFORM) \
		-target-os $(TARGET_OS) 
ifeq ($(VERBOSE), 1)
SDSFLAGS += -verbose 
endif

# SDS Compiler
CPP := sds++ $(SDSFLAGS)
CC := sdscc $(SDSFLAGS)


.PHONY: all elf clean
all: $(BUILD_DIR)/$(LIBRARY) clean

elf: $(BUILD_DIR)/$(EXECUTABLE)

$(BUILD_DIR)/$(EXECUTABLE): $(OBJECTS)
	mkdir -p $(BUILD_DIR)
	@echo 'Building Target: $@'
	@echo 'Trigerring: SDS++ Linker'
	cd $(BUILD_DIR) ; $(CPP) -fPIC -Wall -O3 -o $(EXECUTABLE) $(OBJECTS)
	@echo ' '

$(BUILD_DIR)/$(LIBRARY): $(OBJECTS)
	mkdir -p $(BUILD_DIR)
	@echo 'Building Target: $@'
	@echo 'Trigerring: SDS++ Linker'
	cd $(BUILD_DIR) ; $(CPP) -fPIC -Wall -shared -o $(LIBRARY) $(OBJECTS)
	@echo 'SDx Completed Building Target: $@'
	@echo 'Copy compiled stub files'
	mkdir -p dist/$(BOARD)/$(NAME)
	cp $(DISTS) dist/$(BOARD)/$(NAME)
	@echo 'Copy bitstream'
	mkdir -p ../../boards/$(BOARD)/$(NAME)
	cp $(VIV_BIT) ../../boards/$(BOARD)/$(NAME)/ekf_$(NAME).bit
	@echo 'Copy tcl file'
	cp $(VIV_TCL) ../../boards/$(BOARD)/$(NAME)/ekf_$(NAME).tcl
	@echo

$(pwd)/$(BUILD_DIR)/%.o: $(pwd)/$(SRC_DIR)/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: SDS++ Compiler'
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) ; $(CC) $(CFLAGS) $(CONFIG_FLAGS) -o $(LFLAGS) $(HW_FLAGS)
	@echo 'Finished building: $<'
	@echo ' '

clean:
	$(RM) $(OBJECTS)
