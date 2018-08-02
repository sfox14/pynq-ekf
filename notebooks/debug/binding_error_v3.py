
# coding: utf-8

# In[ ]:


import numpy as np
from ekf.gps_ekf import GPS_EKF, GPS_EKF_HWSW

data = np.loadtxt("gps_data.csv", delimiter=",", skiprows=1)

ekf = GPS_EKF_HWSW(8, 4, qval=0.1, rval=20.0, pval=0.5)
ekf.set_state(np.array([0.2574, 0.3, -0.908482, -0.1, -0.378503, 0.3, 0.02, 0.0]))

res_sw = ekf.run_sw(data)
ekf.set_state(np.array([0.2574, 0.3, -0.908482, -0.1, -0.378503, 0.3, 0.02, 0.0]))
res_hw = ekf.run_hw(data)
print("Equal: ", np.allclose(res_sw, ekf.toFloat(res_hw), rtol=1e-2))

