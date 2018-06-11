
import numpy as np
from . import EKF
from pynq.xlnk import ContiguousArray

FRAC_WIDTH=20
MAX_LENGTH = 1000

class GPS_EKF(EKF):

    def __init__(self, n, m, pval=0.5, qval=0.1, rval=20.0,
                 bitstream=None, lib=None, load_overlay=True):

        EKF.__init__(self, n, m, pval, qval, rval, bitstream, lib, load_overlay)
        self.x = np.array([0.25739993, 0.3,
                           -0.90848143, -0.1,
                           -0.37850311, 0.3,
                           0.02, 0])
        self.x_hw = self.x

        self.n = n
        self.m = m
        self.pval = pval
        self.qval = qval
        self.rval = rval


    @property
    def ffi_interface(self):
        return """ void _p0_gps_ekf_1_noasync(int *xin, int params[182],  
        int *output, int pout[64], int datalen); """


    def configure(self, dtype=np.int32):
        '''
        prepare hw params
            * fixed-point conversion
            * contiguous memory allocation
        '''
        def toFixed(a, dtype=np.int32):
            return (a*(1<<FRAC_WIDTH)).astype(dtype)

        fx = np.zeros(self.n)
        hx = np.zeros(self.m)
        F = np.kron(np.eye(4), np.array([[1,1],[0,1]]))
        H = np.zeros(shape=(self.m, self.n))
        P = np.eye(self.n)*self.pval

        params = np.concatenate(
            (self.x_hw, fx, hx, F.flatten(), H.flatten(), P.flatten(),
             np.array([self.qval]), np.array([self.rval])), axis=0)

        params = toFixed(params)
        self.paramBuffer = self.copy_array(params)
        self.poutBuffer = self.xlnk.cma_array(shape=(64,1), dtype=np.int32)
        self.outBuffer = self.xlnk.cma_array(shape=(MAX_LENGTH, 3),
                                             dtype=np.int32)
        return self

    def run_hw(self, x):
        datalen = len(x)
        if datalen > MAX_LENGTH:
            raise RuntimeError("Buffer overflow: outBuffer required "
                               "exceeds MAX_LENGTH")
        if isinstance(x, ContiguousArray) and x.pointer:
            inBuffer = x.pointer
        else:
            raise RuntimeError("Unknown input buffer")

        self.offload_hw(inBuffer, self.paramBuffer.pointer,
                        self.outBuffer.pointer, self.poutBuffer.pointer,
                        datalen)
        return self.outBuffer[:datalen]

    def offload_hw(self, xin, params, output, pout, dlen):
        if any("cdata" not in elem for elem in [str(xin), str(params),
                                                str(output)]):
            raise RuntimeError("Unknown buffer type!")
        self.interface._p0_gps_ekf_1_noasync(xin, params, output, pout, dlen)


    def run_sw(self, x):
        output = np.zeros((len(x), 3))
        for i, line in enumerate(x):
            # unpack sensor data
            SV_pos = np.array(line[:12]).astype(np.float32).reshape(4, 3)
            SV_rho = np.array(line[12:]).astype(np.float32)
            state = self.step(SV_rho, SV_pos=SV_pos)
            output[i][0] = state[0]
            output[i][1] = state[2]
            output[i][2] = state[4]
        return output


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