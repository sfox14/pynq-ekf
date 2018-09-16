
import os
import numpy as np
from rig.type_casts import NumpyFloatToFixConverter, NumpyFixToFloatConverter
from . import EKF


__author__ = "Sean Fox"


FRAC_WIDTH = 20
MAX_OUT = 1000
ROOT_DIR = os.path.dirname(os.path.realpath(__file__))


class Light_EKF(EKF):
    """Python class for the hybrid HW-SW light sensor fusion example.

    Attributes
    ----------
    n : int
        number of states
    m : int
        number of observations
    x : np.ndarray
        current mean state estimate, shape=(n,)
    F : np.ndarray
        state transition matrix. the GPS model assumes the state transition,
        equation is linear, therefore F is constant. shape=(n,n)
    P : np.ndarray
        state covariance matrix, shape=(n,n)
    Q : np.ndarray
        process covariance matrix, shape=(n,n)
    R : np.ndarray
        observation covariance matrix, shape=(m,m)
    pars : np.ndarray
        flattened array containing P,Q,R, shape=(n*n + n*n + m*m, 1)
    cacheable : int
        Whether the buffers should be cacheable - defaults to 0

    """
    def __init__(self, n=2, m=2, pval=0.01, qval=0.01, rval=2.5,
                 bitstream=None, library=None, cacheable=0):
        if bitstream is None:
            bitstream = os.path.join(ROOT_DIR, "n2m2", "ekf_n2m2.bit")
        if library is None:
            library = os.path.join(ROOT_DIR, "n2m2", "libekf_n2m2.so")
        super().__init__(n, m, pval, qval, rval, bitstream, library, cacheable)

        self.n = n
        self.m = m
        self.P = np.eye(self.n) * pval
        self.Q = np.eye(self.n) * qval
        self.R = np.eye(self.m) * np.array([0.1, 1.0])
        self.toFixed = NumpyFloatToFixConverter(signed=True, n_bits=32,
                                                n_frac=FRAC_WIDTH)
        self.toFloat = NumpyFixToFloatConverter(FRAC_WIDTH)
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
        return """void _p0_top_ekf_1_noasync(int obs[2], int fx_i[2], 
        int hx_i[2], int F_i[4], int H_i[4], int params[12], int output[2], 
        int ctrl, int w1, int w2);"""

    def set_state(self, x=None):
        """Method to set the state

        Parameters
        ----------
        x: numpy.ndarray
            A numpy array storing the state.

        """
        if x is None:
            self.x = np.array([6.00, 0.1])
        else:
            self.x = x

    def configure(self):
        """Prepare the arrays and parameters.

        Start with the default state.

        """
        self.set_state()
        self.params = self.copy_array(self.pars)

        self.obs = self.copy_array(np.zeros(self.m))
        self.out_buffer_hw = self.copy_array(np.zeros((MAX_OUT, self.n)))
        self.out_buffer_sw = np.zeros((MAX_OUT, 3))

        self.F_hw = self.copy_array(
            self.toFixed(np.array([[1, 1], [0, 1]]).flatten()))
        self.H_hw = self.copy_array(
            self.toFixed(np.array([[1, 0], [1, 0]]).flatten()))

        self.fx_hw = self.copy_array(np.zeros(self.n))
        self.hx_hw = self.copy_array(np.zeros(self.m))

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
            obs = np.array(line).astype(np.float32)
            state = self.step(obs)
            self.out_buffer_sw[i][0:2] = [state[0], state[1]]
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
        np.copyto(self.obs, self.toFixed(line))

        self.compute_model(self.x)

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
            obs = (self.toFixed(line))
            np.copyto(self.obs, obs)

            # compute fx, hx, F, H in python floating point, convert and copy
            self.compute_model(self.x)

            # output point offset adjustment
            offset += 8
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
        return self.out_buffer_hw[:len(x), :]

    def compute_model(self, x):
        """Intermediate step for hardware computation.

        Compute fx_hw, hx_hw, F_hw and H_hw in fixed point, and copy
        to contiguous memory

        """
        fx, _ = self.f(x)
        hx, _ = self.h(fx)

        fx_hw = self.toFixed(fx.flatten())
        hx_hw = self.toFixed(hx.flatten())

        np.copyto(self.fx_hw, fx_hw)
        np.copyto(self.hx_hw, hx_hw)

    def f(self, x, **kwargs):
        F = np.array([[1, 1], [0, 1]])
        x = np.dot(x, F.T)
        return x, F

    def h(self, x, **kwargs):
        hx = np.array([x[0], x[0]])
        return hx, np.array([[1, 0], [1, 0]])
