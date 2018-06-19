# Rebuild SDSoC Project

This is a short guide for rebuilding the EKF project on different and new platforms (SDSoC/HLS source files found in /build/src). 

## Quick Start:

From the /build directory, run:

```shell
make PLATFORM=<eg. /home/usr/platform/Pynq-Z1/bare> BOARD=<eg. Pynq-Z1, Ultra96> OVERLAY=<eg. bare, hdmi, ultra> 
```
This will launch the sds++ compiler, and will generate both the bitstream and compiled stub object files. The stubs are outputs of SDSoC
used to call IP on the FPGA. The bitstream and .o files are written to /boards/bitstreams/$(BOARD)/ekf.bit and /build/dist/*.o respectively. 
To generate a shared object (.so), you must link these binaries with usr/lib/libsds_lib.so on the PYNQ board. From a
terminal on the board, you can run:

```shell
arm-linux-gnueabihf-g++ -shared $(OBJS) -l usr/lib/libsds_lib.so -o libekf.so
```

We include build/dist directory in our package and complete this step in setup.py
