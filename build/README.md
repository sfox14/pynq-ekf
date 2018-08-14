
# Rebuilding SDSoC Project

This is a short guide for rebuilding the EKF project. The targeted SDSoC/HLS source files can be found in [/build/src](./src).

## 1. Hardware (x86):

Steps for rebuilding the hardware on your host machine are given below. This flow generates a bitstream and cross-compiled stub object files. The stubs are outputs of SDSoC used to call IP on the FPGA, which must be linked with /usr/lib/libsds_lib.so on the PYNQ board. This will generate a c-callable software library, and is discussed more in the Software section below.


#### Step 1: Clone and Fork

On your host computer, clone this repository and change to the `build` directory.

```shell
git clone https://github.com/<fork_name>/pynq-ekf.git <local_git>
cd <local_git>/build
```

#### Step 2: Source Vivado and SDx

For example, for 2017.4 tools:

```shell
source <sdx_installation_path>/installs/lin64/SDx/2017.4/settings64.sh
source <vivado_installation_path>/installs/lin64/Vivado/2017.4/settings64.sh
```

#### Step 3: Get Platform Information

To find platform informtion, such as the available clock ID's, run the following:

```shell
make info PLATFORM=<platform_path>
```

You must provide the path to a compatible SDSoC platform.

#### Step 4: Build Target

From the `build` directory run:

```shell
make PLATFORM=<platform_path> BOARD=<board_name> CLK_ID=<clock_id>
```

This will launch the sds++ compiler, and will generate both the bitstream and compiled stub object files. 

## 2. Software (PYNQ board)

The software library can be built on the PYNQ board using `setup.py`, or alternatively by running the makefile in `build/arm`.

(Optional) Once the hardware has been built, changes to the repository can be pushed back to your fork at remote/origin/master. This will include the generated bitstream and compiled stub object files.

```shell
git add .
git commit -m "EKF rebuild" 
git push origin/master
```

#### Step 5: Pip Install

Open a terminal on your PYNQ board and run:

```
sudo pip3 install --upgrade git+https://github.com/<fork_name>/pynq-ekf.git 
```

If you instead only copy the bitstream and object files directly to the board,
you can use the makefile in `build/arm`. More information can be found by:

```shell
make help
```

