
import numpy as np
import os
from . import EKF
from pynq.xlnk import ContiguousArray
from rig.type_casts import NumpyFloatToFixConverter, NumpyFixToFloatConverter

FRAC_WIDTH=20
MAX_OUT = 1000
ROOT_DIR = os.path.dirname(os.path.realpath(__file__))

class Light_EKF(EKF):

    """Python class for the hybrid HW-SW light sensor fusion example.

        Attributes:
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

    def __init__(self, n=2, m=2, pval=0.01, qval=0.01, rval=2.5,
                 load_overlay=True):

        OVERLAY_DIR = os.path.join(ROOT_DIR, "hw-sw")
        bitstream = os.path.join(os.path.abspath(OVERLAY_DIR), "ekf_n2m2.bit")
        lib = os.path.join(os.path.abspath(OVERLAY_DIR), "libekf_n2m2.so")

        EKF.__init__(self, n, m, pval, qval, rval, bitstream, lib,
                     load_overlay)
        self.default_state()

        self.n = n
        self.m = m

        self.P = np.eye(self.n) * pval
        self.Q = np.eye(self.n) * qval
        self.R = np.eye(self.m) * np.array([0.1, 1.0])

        self.toFixed = NumpyFloatToFixConverter(signed=True, n_bits=32,
                                                n_frac=FRAC_WIDTH)
        self.toFloat = NumpyFixToFloatConverter(FRAC_WIDTH)

        # hw persistent params
        self.pars = np.concatenate(
            (self.P.flatten(), self.Q.flatten(), self.R.flatten()),
            axis=0) * (1 << 20)

        # hw params
        self.hw_init()

    def default_state(self):
        self.x = np.array([6.00, 0.1])
        return

    def set_state(self, x=None):
        '''
        Method to set the initial state:
            x - numpy.array(np.float)
        '''
        self.x = x
        if x is None:
           self.default_state()
        return

    def hw_init(self):
        self.params = self.copy_array(self.pars)
        self.default_state()
        self.obs = self.copy_array(np.zeros(self.m))
        self.outBuffer = self.copy_array(np.zeros((MAX_OUT, self.n)))

        self.F_hw = self.copy_array(
            self.toFixed(np.array([[1, 1], [0, 1]]).flatten()))
        self.H_hw = self.copy_array(
            self.toFixed(np.array([[1, 0], [1, 0]]).flatten()))

        self.fx_hw = self.copy_array(np.zeros(self.n))
        self.hx_hw = self.copy_array(np.zeros(self.m))

    def reset(self):
        self.xlnk.xlnk_reset()
        self.hw_init()

    @property
    def ffi_interface(self):
        return """void _p0_top_ekf_1_noasync(int obs[2], int fx_i[2], int hx_i[2], 
        int F_i[4], int H_i[4], int params[12], int output[2], int ctrl, int w1, int w2);"""

    def run_sw(self, x):
        output = np.zeros((len(x), self.n))
        for i, line in enumerate(x):
            # unpack sensor data
            obs = np.array(line).astype(np.float32)
            state = self.step(obs)
            output[i][0] = state[0]
            output[i][1] = state[1]
        return output

    def run_hw(self, x):

        # Fetch first observation+measurement, convert observation to fixed, copy into contiguous memory buffer
        line = x[0]
        np.copyto(self.obs, self.toFixed(line))

        # compute fx, hx in python floating point, convert back to fixed, copy to contiguous memory
        self.model(self.x)

        # output ptr
        offset = 0
        out_ptr = self.outBuffer.pointer + offset

        # Intialise HW by setting ctrl=0
        self.offload_kf(self.obs, self.fx_hw, self.hx_hw, self.F_hw, self.H_hw,
                        self.params, out_ptr, 0, self.n, self.m)

        # Convert state into float for next iteration model()
        self.x = self.toFloat(self.outBuffer[0])

        # repeat for len(x)-1 iterations
        for i, line in enumerate(x[1:]):
            os.system("sh -c 'sync; echo 3 > /proc/sys/vm/drop_caches' ")

            # Fetch next observation, convert observation to fixed, copy into contiguous memory buffer
            obs = (self.toFixed(line))
            np.copyto(self.obs, obs)

            # compute fx, hx in python floating point, convert back to fixed, copy to contiguous memory
            self.model(self.x)

            # output ptr
            offset += 8
            out_ptr = self.outBuffer.pointer + offset

            # Run next iteration in HW (without reinitialising state), by setting ctrl=2 (for KF only)
            self.offload_kf(self.obs, self.fx_hw, self.hx_hw, self.F_hw,
                            self.H_hw, self.params, out_ptr, 2, self.n, self.m)

            # Convert state into float for next iteration model()
            self.x = self.toFloat(self.outBuffer[i + 1])

        return self.outBuffer[:len(x), :]  # fixed point

    def model(self, x):
        """ Compute fx_hw, hx_hw, F_hw and H_hw in fixed point, and copy
        to contiguous memory
        """
        fx, _ = self.f(x)
        hx, _ = self.h(fx)

        fx_hw = self.toFixed(fx.flatten())
        hx_hw = self.toFixed(hx.flatten())

        np.copyto(self.fx_hw, fx_hw)
        np.copyto(self.hx_hw, hx_hw)
        return

    def offload_kf(self, obs, fx, hx, F, H, params, out, ctrl, w1, w2):
        if any("cdata" not in elem for elem in
               [str(obs.pointer), str(fx.pointer), str(params.pointer),
                str(out)]):
            raise RuntimeError("Unknown buffer type!")
        self.interface._p0_top_ekf_1_noasync(obs.pointer, fx.pointer,
                                             hx.pointer, F.pointer,
                                             H.pointer, params.pointer,
                                             out, ctrl, w1, w2)

    def f(self, x, **kwargs):
        F = np.array([[1, 1], [0, 1]])
        x = np.dot(x, F.T)
        return x, F

    def h(self, x, **kwargs):
        hx = np.array([x[0], x[0]])
        return hx, np.array([[1, 0], [1, 0]])
