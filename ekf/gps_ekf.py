
import os
import numpy as np
from rig.type_casts import NumpyFloatToFixConverter, NumpyFixToFloatConverter
from . import EKF


__author__ = "Sean Fox"


FRAC_WIDTH = 20
MAX_LENGTH = 1000
ROOT_DIR = os.path.dirname(os.path.realpath(__file__))


class GPS_EKF(EKF):
    """Python class for the HW-Only GPS example.

    Attributes:
    ----------
    n : int
        number of states
    m : int
        number of observations
    pval : float
        diagonal scaling factor for state covariance
    qval : float
        diagonal scaling factor for process covariance
    rval : float
        diagonal scaling factor for observation covariance
    x : numpy.ndarray
        The current mean state estimate

    """
    def __init__(self, n, m, pval=0.5, qval=0.1, rval=20.0,
                 bitstream=None, lib=None):
        if bitstream is None:
            bitstream = os.path.join(ROOT_DIR, "gps", "ekf_gps.bit")
        if lib is None:
            lib = os.path.join(ROOT_DIR, "gps", "libekf_gps.so")
        super().__init__(n, m, pval, qval, rval, bitstream, lib)

        self.toFixed = NumpyFloatToFixConverter(signed=True, n_bits=32,
                                                n_frac=20)
        self.toFloat = NumpyFixToFloatConverter(20)
        self.n = n
        self.m = m
        self.pval = pval
        self.qval = qval
        self.rval = rval
        self.param_buffer = None
        self.pout_buffer = None
        self.out_buffer_hw = None
        self.out_buffer_sw = None

        self.configure()

    @property
    def ffi_interface(self):
        return """ void _p0_top_ekf_1_noasync(int *xin, int params[182],  
        int *output, int pout[64], int datalen); """

    def set_state(self, x=None):
        """Method to set the state

        Parameters
        ----------
        x: numpy.ndarray
            A numpy array storing the state.

        """
        if x is None:
            self.x = np.array([0.25739993, 0.3, -0.90848143, -0.1, -0.37850311,
                               0.3, 0.02, 0])
        else:
            self.x = x

    def configure(self, dtype=np.int32):
        """Custom method to prepare hw accelerator.

        The following will be done:
        1. fixed-point conversion
        2.contiguous memory allocation

        """
        self.set_state()

        fx = np.zeros(self.n)
        hx = np.zeros(self.m)
        F = np.kron(np.eye(4), np.array([[1, 1], [0, 1]]))
        H = np.zeros(shape=(self.m, self.n))
        P = np.eye(self.n)*self.pval

        params = np.concatenate(
            (self.x, fx, hx, F.flatten(), H.flatten(), P.flatten(),
             np.array([self.qval]), np.array([self.rval])), axis=0)

        params = self.toFixed(params)
        self.param_buffer = self.copy_array(params)
        self.pout_buffer = self.xlnk.cma_array(shape=(64, 1), dtype=dtype)
        self.out_buffer_hw = self.xlnk.cma_array(shape=(MAX_LENGTH, 3),
                                                 dtype=dtype)
        self.out_buffer_sw = np.zeros((MAX_LENGTH, 3))

    def reset(self):
        """Reset all the contiguous memory.

        After this method is called, users have to call `configure()` to be
        able to run any computation.

        """
        self.xlnk.xlnk_reset()

    def run_hw(self, x):
        """Run the hardware-accelerated computation.

        Error checking is removed to improve the performance. However, users
        have to enforce:

        1. The `output_buffer` required should not exceeds MAX_LENGTH.

        2. The `in_buffer` has to be in contiguous memory.

        """
        datalen = len(x)
        in_buffer = x.pointer
        self.dlib._p0_top_ekf_1_noasync(in_buffer,
                                        self.param_buffer.pointer,
                                        self.out_buffer_hw.pointer,
                                        self.pout_buffer.pointer,
                                        datalen)
        return self.out_buffer_hw[:datalen]

    def run_sw(self, x):
        """Run the software version of the computation.

        This method uses a designated buffer to store the outputs.

        """
        for i, line in enumerate(x):
            SV_pos = np.array(line[:12]).astype(np.float32).reshape(4, 3)
            SV_rho = np.array(line[12:]).astype(np.float32)
            state = self.step(SV_rho, SV_pos=SV_pos)
            self.out_buffer_sw[i,:] = [state[0], state[2], state[4]]
        return self.out_buffer_sw[:len(x)]

    def f(self, x, **kwargs):
        a = np.array([[1, 1], [0, 1]])
        F = np.kron(np.eye(4), a)
        x = np.dot(x, F.T)
        return x, F

    def h(self, x, **kwargs):
        if "SV_pos" not in kwargs:
            raise RuntimeError("Satellite positions required for observation "
                               "model")
        SV_pos = kwargs.get("SV_pos")

        # unpack state vector
        xyz = np.array([x[0], x[2], x[4]])
        bias = x[6]

        # pseudo range equation: hx = || xyz - SV_pos || + bias
        dx = xyz - SV_pos
        dx2 = dx ** 2
        hx = np.sqrt(np.sum(dx2, axis=1)) + bias

        H = np.zeros((4, 8))
        for i in range(4):
            for j in range(3):
                idx = 2 * j
                H[i][idx] = dx[i][j] / hx[i]
            H[i][6] = 1

        return hx, H


class GPS_EKF_HWSW(EKF):
    """Python class for the hybrid HW-SW GPS example.

    Attributes
    ----------
    n : int
        number of states
    m : int
        number of observations
    x : np.array(np.float), shape=(n,)
        current mean state estimate
    F : np.array(float), shape=(n,n)
        state transition matrix. the GPS model assumes the state transition
        equation is linear, therefore F is constant.
    P : np.array(float), shape=(n,n)
        state covariance matrix
    Q : np.array(float), shape=(n,n)
        process covariance matrix
    R : np.array(float), shape=(m,m)
        observation covariance matrix
    pars : np.array(float), shape=(n*n + n*n + m*m, 1)
        flattened array containing P,Q,R

    """
    def __init__(self, n=8, m=4, pval=0.5, qval=0.1, rval=20,
                 bitstream=None, lib=None):
        if bitstream is None:
            bitstream = os.path.join(ROOT_DIR, "n8m4", "ekf_n8m4.bit")
        if lib is None:
            lib = os.path.join(ROOT_DIR, "n8m4", "libekf_n8m4.so")
        super().__init__(n, m, pval, qval, rval, bitstream, lib)

        self.n = n
        self.m = m

        # sw params
        self.F = np.kron(np.eye(4), np.array([[1, 1], [0, 1]]))
        self.P = np.eye(n) * pval
        self.Q = np.eye(n) * qval
        self.R = np.eye(m) * rval
        self.toFixed = NumpyFloatToFixConverter(signed=True, n_bits=32,
                                                n_frac=20)
        self.toFloat = NumpyFixToFloatConverter(20)

        # hw params
        self.pars = np.concatenate(
            (self.P.flatten(), self.Q.flatten(), self.R.flatten()),
            axis=0) * (1 << 20)
        self.params = None
        self.F_hw = None
        self.fx_hw = None
        self.hx_hw = None
        self.H_hw = None
        self.out_buffer_hw = None
        self.out_buffer_sw = None
        self.obs = None

        self.configure()

    @property
    def ffi_interface(self):
        return """void _p0_top_ekf_1_noasync(int obs[4], int fx_i[8], 
        int hx_i[4], int F_i[64], int H_i[32], int params[144], int output[8], 
        int ctrl, int w1, int w2);"""

    def set_state(self, x=None):
        """Method to set the state

        Parameters
        ----------
        x: numpy.ndarray
            A numpy array storing the state.

        """
        if x is None:
            self.x = np.array([0.2574, 0.3, -0.908482, -0.1, -0.378503, 0.3,
                               0.02, 0.0])
        else:
            self.x = x

    def configure(self):
        """Prepare the arrays and parameters.

        Start with the default state.

        """
        self.set_state()
        self.params = self.copy_array(self.pars)
        self.F_hw = self.copy_array(
            self.toFixed(self.F.flatten()))
        self.fx_hw = self.copy_array(np.zeros(self.n))
        self.hx_hw = self.copy_array(np.zeros(self.m))
        self.H_hw = self.copy_array(np.zeros(self.n * self.m))
        self.out_buffer_hw = self.xlnk.cma_array(shape=(50, self.n),
                                                 dtype=np.int32)
        self.out_buffer_sw = np.zeros((50, 3))
        self.obs = self.copy_array(np.zeros(self.m))

    def reset(self):
        """Reset all the contiguous memory.

        After this method is called, users have to call `configure()` to be
        able to run any computation.

        """
        self.xlnk.xlnk_reset()

    def run_sw(self, x):
        """Run the software version of the computation.

        This method uses a designated buffer to store the outputs.

        """
        for i, line in enumerate(x):
            obs = np.array(line[12:])
            pos = np.array(line[:12]).reshape(4, 3)
            state = self.step(obs, SV_pos=pos)
            self.out_buffer_sw[i, :] = [state[0], state[2], state[4]]
        return self.out_buffer_sw[:len(x)]

    def run_hw(self, x):
        """Run the hardware-accelerated computation.

        Error checking is removed to improve the performance.

        The following steps are performed in the hardware computation:

        1. Fetch first observation and measurement,convert observation to
        fixed numbers, and copy into contiguous memory buffer

        2. Compute fx, hx, F, H in python floating point numbers,
        convert back to fixed, copy to contiguous memory

        3. Intialise HW by setting ctrl=0

        4. Convert state into float for next iteration model()

        5. Repeat for len(x)-1 iterations.

        """
        line = x[0]
        pos = np.array(line[:12]).reshape(4, 3)
        rho = np.array(line[12:])
        np.copyto(self.obs, self.toFixed(rho).astype(np.int32))

        self.compute_model(self.x, pos)

        offset = 0
        out_ptr = self.out_buffer_hw.pointer

        self.dlib._p0_top_ekf_1_noasync(self.obs.pointer,
                                        self.fx_hw.pointer,
                                        self.hx_hw.pointer,
                                        self.F_hw.pointer,
                                        self.H_hw.pointer,
                                        self.params.pointer,
                                        out_ptr, 0, self.n, self.m)
        self.x = self.toFloat(self.out_buffer_hw[0])

        for i, line in enumerate(x[1:]):
            # fetch next observation and measurement, convert and copy
            pos = np.array(line[:12]).reshape(4, 3)
            rho = np.array(line[12:])
            np.copyto(self.obs, self.toFixed(rho).astype(np.int32))

            # compute fx, hx, F, H in python floating point, convert and copy
            self.compute_model(self.x, pos)

            # output point offset adjustment
            offset += 32
            out_ptr = self.out_buffer_hw.pointer + offset

            # run next iteration in HW by setting ctrl=1
            self.dlib._p0_top_ekf_1_noasync(self.obs.pointer,
                                            self.fx_hw.pointer,
                                            self.hx_hw.pointer,
                                            self.F_hw.pointer,
                                            self.H_hw.pointer,
                                            self.params.pointer,
                                            out_ptr, 1, self.n, self.m)

            # convert state into float for next iteration model
            self.x = self.toFloat(self.out_buffer_hw[i + 1])
        return self.out_buffer_hw[:len(x), [0, 2, 4]]

    def compute_model(self, x, pos):
        """Intermediate step for hardware computation.

        Compute fx_hw, hx_hw, F_hw and H_hw in fixed point, and copy
        to contiguous memory

        """
        fx, F = self.f(x)
        hx, H = self.h(fx, SV_pos=pos)
        np.copyto(self.fx_hw, self.toFixed(fx.flatten()).astype(np.int32))
        np.copyto(self.hx_hw, self.toFixed(hx.flatten()).astype(np.int32))
        np.copyto(self.H_hw, self.toFixed(H.flatten()).astype(np.int32))

    def f(self, x, **kwargs):
        F = self.F
        x = np.dot(x, F.T)
        return x, F

    def h(self, x, **kwargs):
        if "SV_pos" not in kwargs:
            raise RuntimeError("Satellite positions required for observation.")
        SV_pos = kwargs.get("SV_pos")

        # unpack state vector
        xyz = np.array([x[0], x[2], x[4]])
        bias = x[6]

        # pseudorange equation: hx = || xyz - SV_pos || + bias
        dx = xyz - SV_pos
        dx2 = dx ** 2
        hx = np.sqrt(np.sum(dx2, axis=1)) + bias

        H = np.zeros((4, 8))
        for i in range(4):
            for j in range(3):
                idx = 2 * j
                H[i][idx] = dx[i][j] / hx[i]
            H[i][6] = 1

        return hx, H
