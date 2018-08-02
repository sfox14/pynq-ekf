
# coding: utf-8

# In[ ]:


import numpy as np
from pynq import Overlay, Xlnk
import cffi

bitstream = "/usr/local/lib/python3.6/dist-packages/ekf/hw-sw/ekf_n8m4.bit"
library = "/usr/local/lib/python3.6/dist-packages/ekf/hw-sw/libekf_n8m4.so"
ffi_interface = "void _p0_top_ekf_1_noasync(int obs[4], int fx_i[8], int hx_i[4], int F_i[64], int H_i[32], int params[144], int output[8], int ctrl, int w1, int w2);"

ol = Overlay(bitstream)
ol.download()
ffi = cffi.FFI()
interface = ffi.dlopen(library)
ffi.cdef(ffi_interface)
xlnk = Xlnk()

