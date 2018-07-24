# Rebuild SDSoC Project

This is a short guide for rebuilding the EKF project on different and new platforms. You can also use HLS directives to build different (N,M) configurations of the HW-SW co-design architectecture. The targeted SDSoC/HLS source files can be found in [/build/src](./src).

## Hardware (x86):

Below are steps for rebuilding the hardware on your host machine. This flow generates a bitstream and cross-compiled stub object files. The stubs are outputs of SDSoC used to call IP on the FPGA. They must be linked with /usr/lib/libsds_lib.so on the PYNQ board to generate a c-callable software library. This is discussed in Software below.


#### Step 1: Clone and Fork

On your host computer, clone this repository and change to the /build directory.

```shell
git clone https://github.com/sfox14/pynq-ekf.git
cd pynq-ekf/build
```
#### Step 2: Source Vivado and SDx 2017.4

SDx:
```shell
source <SDx Installation Dir>/installs/lin64/SDx/2017.4/settings64.sh
```

Vivado:
```shell
source <Vivado Installation Dir>/installs/lin64/Vivado/2017.4/settings64.sh
```

#### Step 3: Build Target

From ./build directory run:

```shell
make PLATFORM=<eg. /home/usr/platform/Pynq-Z1/bare> BOARD=<eg. Pynq-Z1, Ultra96> 
```
This will launch the sds++ compiler, and will generate both the bitstream and compiled stub object files. This step requires the path to a working SDSoC platform. You can visit this [repository](https://github.com/yunqu/PYNQ-derivative-overlays) if you would like to build your own platform using a DSA derived from an existing PYNQ project. 


## Software (PYNQ board)

The software library can be built on the PYNQ board using setup.py, or alternatively by running the makefile separately.

#### Step 4: Push Changes to Fork

Once the hardware has been built, changes to the repository should be pushed back to your fork at remote/origin/master. This will include the generated bitstream and compiled stub object files.

```shell
git add .
git commit -m "EKF rebuild" 
git push origin/master
```

#### Step 5: Pip Install

Open a terminal on your PYNQ board and run:

```
sudo pip3.6 install --upgrade git+https://github.com/yourusername/pynq-ekf.git 
```

## Note: 

If you chose to ignore Step 4 and Step 5 and instead copied the bitstream and object files directly to the board. You can then run:

On ARM Cortex-A9 (eg. Z1, Z2):
```shell
arm-linux-gnueabihf-g++ -shared $(OBJS) -l usr/lib/libsds_lib.so -o libekf.so
```

On ARM Cortex-A53 (eg. Ultra96, ZCU104):
```shell
aarch64-linux-gnu-g++ -shared $(OBJS) -l usr/lib/libsds_lib.so -o libekf.so
```