'''
Extended Kalaman Filter:
    Adapted from "https://github.com/simondlevy/TinyEKF"
'''


import numpy as np
from abc import ABCMeta, abstractmethod
from pynq import Overlay, PL, Xlnk
import cffi
import os


class EKF(object):
    __metaclass__ = ABCMeta

    def __init__(self, n, m, pval=0.5, qval=0.1, rval=20.0,
                 bitstream=None, lib=None, load_overlay=True):
        '''
        Parameters:
            n : int
                number of states
            m : int
                number of observables/measurements
            pval : float = 0.5
                prediction noise covariance
            qval : float = 0.1
                state noise covariance
            rval : float = 20.0
                measurement noise covariance
            bitstream : str = None
                string identifier of the bitstream
            lib : str = None
                string identifier of the C library
            load_overlay : Bool = True

        Attributes:
        '''
        self.bitstream_name = bitstream
        self.library_name = lib
        if load_overlay:
            Overlay(self.bitstream_name).download()
        else:
            raise RuntimeError("Incorrect Overlay loaded")
        self._ffi = cffi.FFI()
        self.interface = self._ffi.dlopen(self.library_name)
        self._ffi.cdef(self.ffi_interface)
        self.xlnk = Xlnk()

        # No previous prediction noise covariance
        self.P_pre = None

        # Current state is zero, with diagonal noise covariance matrix
        self.x = np.zeros(n)
        self.P_post = np.eye(n) * pval

        # Set up covariance matrices for process noise and measurement noise
        self.Q = np.eye(n) * qval
        self.R = np.eye(m) * rval

        # Identity matrix will be usefel later
        self.I = np.eye(n)

    def step(self, z, **kwargs):
        '''
        Runs one step of the EKF on observations z, where z is a tuple of
        length m. Returns a NumPy array representing the updated state.
        '''

        # Predict ----------------------------------------------------
        # $\hat{x}_k = f(\hat{x}_{k-1})$
        self.x, F = self.f(self.x, **kwargs)

        # $P_k = F_{k-1} P_{k-1} F^T_{k-1} + Q_{k-1}$
        self.P_pre = np.dot(np.dot(F, self.P_post), F.T) + self.Q

        # Update -----------------------------------------------------
        h, H = self.h(self.x, **kwargs)

        # $G_k = P_k H^T_k (H_k P_k H^T_k + R)^{-1}$
        G = np.dot(np.dot(self.P_pre, H.T), np.linalg.inv(
            np.dot(np.dot(H, self.P_pre), H.T) + self.R))

        # $\hat{x}_k = \hat{x_k} + G_k(z_k - h(\hat{x}_k))$
        self.x += np.dot(G, (z - h))

        # $P_k = (I - G_k H_k) P_k$
        self.P_post = np.dot(self.I - np.dot(G, H), self.P_pre)

        return self.x

    def copy_array(self, X, dtype=np.int32):
        """
        param X: np.array
        return: xlnk.cma_array()
        """
        size = X.shape
        dataBuffer = self.xlnk.cma_array(shape=size, dtype=dtype)
        np.copyto(dataBuffer, X.astype(dtype), casting="unsafe")
        return dataBuffer

    @abstractmethod
    def ffi_interface(self):
        raise NotImplementedError()

    @abstractmethod
    def f(self, x):
        '''
        Your implementing class should define this method for the
        state-transition function f(x).
        Your state-transition fucntion should return a NumPy array of n
        elements representing the
        new state, and a nXn NumPy array of elements representing the the
        Jacobian of the function
        with respect to the new state.
        '''
        raise NotImplementedError()

    @abstractmethod
    def h(self, x):
        '''
        Your implementing class should define this method for the
        observation function h(x), returning
        a NumPy array of m elements, and a NumPy array of m x n elements
        representing the Jacobian matrix
        H of the observation function with respect to the observation.
        '''
        raise NotImplementedError()




from .gps_ekf import GPS_EKF

__author__ = "Sean Fox"

__all__ = ["EKF",
           "GPS_EKF",
           "GPS_EKF_GENERAL",
           "Light_EKF"]
__version__= 0.2