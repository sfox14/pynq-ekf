
# coding: utf-8

# In[1]:


import numpy as np
from ekf.gps_ekf import GPS_EKF, GPS_EKF_HWSW


# In[2]:


data = np.loadtxt("gps_data.csv", delimiter=",", skiprows=1)


# In[3]:


ekf = GPS_EKF_HWSW(8, 4, qval=0.1, rval=20.0, pval=0.5)
ekf.set_state(np.array([0.2574, 0.3, -0.908482, -0.1, -0.378503, 0.3, 0.02, 0.0]))


# In[4]:


res_sw = ekf.run_sw(data)
ekf.set_state(np.array([0.2574, 0.3, -0.908482, -0.1, -0.378503, 0.3, 0.02, 0.0]))
res_hw = ekf.run_hw(data)
print("Equal: ", np.allclose(res_sw, ekf.toFloat(res_hw), rtol=1e-2))


# In[5]:


ekf.reset()
res_hw = ekf.run_hw(data)


# In[6]:


import timeit
number=3
ekf.reset()

def hwresp():
    y=ekf.run_hw(data)
    return

hw_time = timeit.timeit(hwresp, number=number)

def swresp():
    y=ekf.run_sw(data)
    return

sw_time = timeit.timeit(swresp,number=number)

print("Time taken by software", number,"times",sw_time)
print("Time taken by hardware", number,"times",hw_time)
print("HW Speedup = %.2fx"%(sw_time/hw_time))


# In[7]:


ekf.xlnk.xlnk_reset()


# In[8]:


ekf = GPS_EKF(8, 4, qval=0.1, rval=20.0, pval=0.5)
ekf.set_state(np.array([0.25739993, 0.3, -0.90848143, -0.1, -0.37850311, 0.3, 0.02, 0]))
res_sw = ekf.run_sw(data)


# In[9]:


from rig.type_casts import NumpyFloatToFixConverter, NumpyFixToFloatConverter
toFixed = NumpyFloatToFixConverter(signed=True, n_bits=32, n_frac=20)
toFloat = NumpyFixToFloatConverter(20)

data_hw = toFixed(data)
data_hw = ekf.copy_array(data_hw)
ekf = ekf.configure()
res_hw = ekf.run_hw(data_hw)
res_hw = toFloat(res_hw)
print("Equal: ", np.allclose(res_sw, res_hw, rtol=1e-2))


# In[10]:


import timeit
number=20

def hwresp():
    y=ekf.run_hw(data_hw)
    return

hw_time = timeit.timeit(hwresp,number=number)

def swresp():
    y=ekf.run_sw(data)
    return

sw_time = timeit.timeit(swresp,number=number)

print("Time taken by software", number,"times",sw_time)
print("Time taken by hardware", number,"times",hw_time)
print("HW Speedup = %.2fx"%(sw_time/hw_time))


# In[11]:


ekf.xlnk.xlnk_reset()

