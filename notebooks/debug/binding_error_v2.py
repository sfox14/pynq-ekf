
# coding: utf-8

# In[1]:


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


# In[8]:


a = xlnk.cma_array(shape=(4,), dtype=np.int32)
np.copyto(a, np.random.randint(321231, size=4) )

b = xlnk.cma_array(shape=(8,), dtype=np.int32)
np.copyto(b, np.random.randint(321231, size=8) )

c = xlnk.cma_array(shape=(4,), dtype=np.int32)
np.copyto(c, np.random.randint(321231, size=4) )

d = xlnk.cma_array(shape=(64,), dtype=np.int32)
np.copyto(d, np.random.randint(321231, size=64) )

e = xlnk.cma_array(shape=(32,), dtype=np.int32)
np.copyto(e, np.random.randint(321231, size=32) )

f = xlnk.cma_array(shape=(144,), dtype=np.int32)
np.copyto(f, np.random.randint(321231, size=144) )

g = xlnk.cma_array(shape=(8,), dtype=np.int32)
np.copyto(g, np.random.randint(321231, size=8) )

ctrl = 0
w1 = 8
w2 = 4

#sanity check
print(type(f), f.shape, f.pointer, f[:3])


# In[ ]:


interface._p0_top_ekf_1_noasync(a.pointer, b.pointer, c.pointer, d.pointer, e.pointer, f.pointer, h.pointer, ctrl, w1, w2)

ctrl = 1
w1=8 
w2=4

for i in range(50):
    a = xlnk.cma_array(shape=(4,), dtype=np.int32)
    np.copyto(a, np.random.randint(321231, size=4) )
    b = xlnk.cma_array(shape=(8,), dtype=np.int32)
    np.copyto(b, np.random.randint(321231, size=8) )
    c = xlnk.cma_array(shape=(4,), dtype=np.int32)
    np.copyto(c, np.random.randint(321231, size=4) )
    d = xlnk.cma_array(shape=(64,), dtype=np.int32)
    np.copyto(d, np.random.randint(321231, size=64) )
    e = xlnk.cma_array(shape=(32,), dtype=np.int32)
    np.copyto(e, np.random.randint(321231, size=32) )
    f = xlnk.cma_array(shape=(144,), dtype=np.int32)
    np.copyto(f, np.random.randint(321231, size=144) )
    g = xlnk.cma_array(shape=(8,), dtype=np.int32)
    np.copyto(g, np.random.randint(321231, size=8) )
    
    interface._p0_top_ekf_1_noasync(a.pointer, b.pointer, c.pointer, d.pointer, e.pointer, f.pointer, h.pointer, ctrl, w1, w2)

