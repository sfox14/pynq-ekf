
# Rebuilding SDSoC Project

This is a short guide for rebuilding the EKF project. The targeted SDSoC/HLS source files can be found in `/build/src`.
This flow is only used in very rare cases; in general, users do not have to 
rebuild anything when using this package.

Steps for rebuilding the hardware on your X86 host machine are given below. 
This flow generates bitstreams and cross-compiled shared object files.
The shared object files (`*.so`) can be used directly on a PYNQ-supported board.


#### Step 1: Clone and Fork

On your host computer, clone this repository and change to the `build` directory.

```shell
git clone https://github.com/<fork_name>/pynq-ekf.git <local_git>
cd <local_git>/build
```

#### Step 2: Source Vivado and SDx

For example:

```shell
source <sdx_installation_path>/installs/lin64/SDx/<version>/settings64.sh
source <vivado_installation_path>/installs/lin64/Vivado/<version>/settings64.sh
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

This will launch the sds++ compiler, and will generate both the bitstreams 
and shared object files. More information can be found by:

```shell
make help
```
