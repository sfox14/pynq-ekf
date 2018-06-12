# PYNQ Extended Kalman Filter (EKF) 

This repository provides an example of PYNQ supporting multiple boards from a single pip-installable package. From the same HLS/SDSoC source code, and using the same Python API and notebooks, we can develop applications which simply move across Xilinx boards. This has the potential to simplify the deployment of embedded applications, improve code reusability and reduce design time.  

## Quick Start:

Open a terminal on your PYNQ board and run:

```
sudo pip3.6 install git+https://github.com/sfox14/pynq-ekf.git 
```

This will install the "pynq-ekf" package to your board, and will create the "ekf" directory in PYNQ_JUPYTER_NOTEBOOKS. Here, you will find a notebook which tests our EKF design.
