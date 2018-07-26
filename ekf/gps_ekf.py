
import numpy as np
import os
from . import EKF
from pynq.xlnk import ContiguousArray
from rig.type_casts import NumpyFloatToFixConverter, NumpyFixToFloatConverter


FRAC_WIDTH=20
MAX_LENGTH = 1000
ROOT_DIR = os.path.dirname(os.path.realpath(__file__))

class GPS_EKF(EKF):

    def __init__(self, n, m, pval=0.5, qval=0.1, rval=20.0, load_overlay=True):

        OVERLAY_DIR = os.path.join(ROOT_DIR, "hw")
        bitstream = os.path.join(os.path.abspath(OVERLAY_DIR), "ekf_gps.bit")
        lib = os.path.join(os.path.abspath(OVERLAY_DIR), "libekf_gps.so")

        EKF.__init__(self, n, m, pval, qval, rval, bitstream, lib, load_overlay)
        self.x = np.array([0.25739993, 0.3,
                           -0.90848143, -0.1,
                           -0.37850311, 0.3,
                           0.02, 0])
        self.x_hw = self.x
        self.toFixed = NumpyFloatToFixConverter(signed=True, n_bits=32,
                                                n_frac=20)
        self.toFloat = NumpyFixToFloatConverter(20)
        self.n = n
        self.m = m
        self.pval = pval
        self.qval = qval
        self.rval = rval


    @property
    def ffi_interface(self):
        return """ void _p0_top_ekf_1_noasync(int *xin, int params[182],  
        int *output, int pout[64], int datalen); """


    def configure(self, dtype=np.int32):
        '''
        prepare hw params
            * fixed-point conversion
            * contiguous memory allocation
        '''
        fx = np.zeros(self.n)
        hx = np.zeros(self.m)
        F = np.kron(np.eye(4), np.array([[1,1],[0,1]]))
        H = np.zeros(shape=(self.m, self.n))
        P = np.eye(self.n)*self.pval

        params = np.concatenate(
            (self.x_hw, fx, hx, F.flatten(), H.flatten(), P.flatten(),
             np.array([self.qval]), np.array([self.rval])), axis=0)

        params = self.toFixed(params)
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
        self.interface._p0_top_ekf_1_noasync(xin, params, output, pout, dlen)


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



class GPS_EKF_GENERAL(EKF):

    def __init__(self, n=8, m=4, pval=0.5, qval=0.1, rval=20,
                 load_overlay=True):

        OVERLAY_DIR = os.path.join(ROOT_DIR, "hw-sw")
        bitstream = os.path.join(os.path.abspath(OVERLAY_DIR), "ekf_n8m4.bit")
        lib = os.path.join(os.path.abspath(OVERLAY_DIR), "libekf_n8m4.so")

        EKF.__init__(self, n, m, pval, qval, rval, bitstream, lib,
                     load_overlay)
        self.n = n
        self.m = m

        # sw params
        self.x = np.array(
            [0.2574, 0.3, -0.908482, -0.1, -0.378503, 0.3, 0.02, 0.0])
        self.F = np.kron(np.eye(4), np.array([[1, 1], [0, 1]]))
        self.H = np.zeros(shape=(m, n))
        self.P = np.eye(n) * pval
        self.Q = np.eye(n) * qval
        self.R = np.eye(m) * rval
        self.fx = np.zeros(n)
        self.hx = np.zeros(m)

        self.toFixed = NumpyFloatToFixConverter(signed=True, n_bits=32,
                                                n_frac=20)
        self.toFloat = NumpyFixToFloatConverter(20)

        # hw persisitent params
        self.pars = np.concatenate(
            (self.x, self.P.flatten(), self.Q.flatten(), self.R.flatten()),
            axis=0) * (1 << 20)

        # hw params
        self.hw_init()

    def hw_init(self):
        self.params = self.copy_array(self.pars)
        self.x_fl = np.array(
            [0.2574, 0.3, -0.908482, -0.1, -0.378503, 0.3, 0.02, 0.0])
        self.F_hw = self.copy_array(
            self.toFixed(self.F.flatten()))  # *(1<<20) )
        self.fx_hw = self.copy_array(np.zeros(self.n))
        self.hx_hw = self.copy_array(np.zeros(self.m))
        self.H_hw = self.copy_array(np.zeros(self.n * self.m))
        # self.outBuffer = self.copy_array( np.zeros((50,self.n)) )
        self.outBuffer = self.xlnk.cma_array(shape=(50, self.n),
                                             dtype=np.int32)
        self.obs = self.copy_array(np.zeros(self.m))

    def reset(self):
        self.xlnk.xlnk_reset()
        self.hw_init()

    @property
    def ffi_interface(self):
        return """void _p0_top_ekf_1_noasync(int obs[4], int fx_i[8], int hx_i[4], 
        int F_i[64], int H_i[32], int params[152], int output[8], int ctrl, int w1, int w2);"""

    def run_sw(self, x):
        output = np.zeros((len(x), 3))
        for i, line in enumerate(x):
            # unpack sensor data
            obs = np.array(line[12:])
            pos = np.array(line[:12]).reshape(4, 3)
            state = self.step(obs, SV_pos=pos)
            output[i][0] = state[0]
            output[i][1] = state[2]
            output[i][2] = state[4]
        return output


    def run_hw(self, x):

        # Fetch first observation+measurement, convert observation to fixed, copy into contiguous memory buffer
        line = x[0]
        pos = np.array(line[:12]).reshape(4, 3)
        rho = np.array(line[12:])
        np.copyto(self.obs, self.toFixed(rho).astype(np.int32))

        # compute fx, hx, F, H in python floating point, convert back to fixed, copy to contiguous memory
        self.model(self.x_fl, pos)

        # output ptr
        offset = 0
        out_ptr = self.outBuffer.pointer + offset

        # Intialise HW by setting ctrl=0
        self.offload_kf(self.obs, self.fx_hw, self.hx_hw, self.F_hw, self.H_hw,
                        self.params, out_ptr, int(0), self.n, self.m)

        # convert state into float for next iteration model()
        self.x_fl = self.toFloat(self.outBuffer[0])  # *(1.0/(1<<20))

        # repeat for len(x)-1 iterations
        for i, line in enumerate(x[1:]):

            os.system("sh -c 'sync; echo 3 > /proc/sys/vm/drop_caches' ")

            # Fetch next observation+measurement, convert observation to fixed, copy into contiguous memory buffer
            pos = np.array(line[:12]).reshape(4, 3)
            rho = np.array(line[12:])
            np.copyto(self.obs, self.toFixed(rho).astype(np.int32))

            # compute fx, hx, F, H in python floating point, convert back to fixed, copy to contiguous memory
            self.model(self.x_fl, pos)

            # output ptr
            offset += 32
            out_ptr = self.outBuffer.pointer + offset

            # Run next iteration in HW (without reinitialising state), by setting ctrl=1
            self.offload_kf(self.obs, self.fx_hw, self.hx_hw, self.F_hw,
                            self.H_hw, self.params, out_ptr, int(1), self.n,
                            self.m)

            # convert state into float for next iteration model()
            self.x_fl = self.toFloat(self.outBuffer[i + 1])  # *(1.0/(1<<20))

        return self.outBuffer[:, [0, 2, 4]]  # output #

    def model(self, x, pos):
        """ Compute fx_hw, hx_hw, F_hw and H_hw in fixed point, and copy
        to contiguous memory
        """
        fx, F = self.f(x)
        hx, H = self.h(fx, SV_pos=pos)
        np.copyto(self.fx_hw, self.toFixed(fx.flatten()).astype(np.int32))
        np.copyto(self.hx_hw, self.toFixed(hx.flatten()).astype(np.int32))
        np.copyto(self.H_hw, self.toFixed(H.flatten()).astype(np.int32))
        return

    def offload_kf(self, obs, fx, hx, F, H, params, out, ctrl, w1, w2):
        if any("cdata" not in elem for elem in
               [str(obs.pointer), str(fx.pointer), str(H.pointer),
                str(params.pointer), str(out)]):
            raise RuntimeError("Unknown buffer type!")
        self.interface._p0_top_ekf_1_noasync(obs.pointer, fx.pointer,
                                             hx.pointer, F.pointer,
                                             H.pointer, params.pointer,
                                             out, ctrl, w1, w2)

    def f(self, x, **kwargs):
        F = self.F
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