
from abc import ABCMeta, abstractmethod
import cffi
import os
import numpy as np
from pynq import Overlay, Xlnk


__author__ = "Sean Fox"


class EKF(object):
    """EKF abstract class.

    The bitstream and lib inputs to this class are defaulted to None, so this
    method should only be used as the parent class. Directly using it will
    lead to exceptions.

    """
    __metaclass__ = ABCMeta

    def __init__(self, n, m, pval=0.5, qval=0.1, rval=20.0,
                 bitstream=None, lib=None):
        """Initialize the EKF object.

        Parameters
        ----------
        n : int
            number of states
        m : int
            number of observables/measurements
        pval : float
            prediction noise covariance
        qval : float
            state noise covariance
        rval : float
            measurement noise covariance
        bitstream : str
            string identifier of the bitstream
        lib : str
            string identifier of the C library

        """
        self.bitstream_name = bitstream
        self.overlay = Overlay(self.bitstream_name)

        self.library_name = lib
        self._ffi = cffi.FFI()
        self.dlib = self._ffi.dlopen(self.library_name)
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

        # Identity matrix
        self.I = np.eye(n)

    def reload_overlay(self):
        """Reloading the bitstream onto PL.

        This method is no needed during initialization, but can be called
        explicitly by users.

        """
        self.overlay.download()

    def step(self, z, **kwargs):
        """Runs one step of the EKF on observations z.

        This is the SW only implementation.

        Parameters
        ----------
        z: tuple
            A tuple of length m

        Returns
        -------
        numpy.ndarray
            representing the updated state.

        """

        # Predict $\hat{x}_k = f(\hat{x}_{k-1})$
        self.x, F = self.f(self.x, **kwargs)

        # $P_k = F_{k-1} P_{k-1} F^T_{k-1} + Q_{k-1}$
        self.P_pre = np.dot(np.dot(F, self.P_post), F.T) + self.Q

        # Update
        h, H = self.h(self.x, **kwargs)

        # $G_k = P_k H^T_k (H_k P_k H^T_k + R)^{-1}$
        G = np.dot(np.dot(self.P_pre, H.T), np.linalg.inv(
            np.dot(np.dot(H, self.P_pre), H.T) + self.R))

        # $\hat{x}_k = \hat{x_k} + G_k(z_k - h(\hat{x}_k))$
        self.x += np.dot(G, (z - h))

        # $P_k = (I - G_k H_k) P_k$
        self.P_post = np.dot(self.I - np.dot(G, H), self.P_pre)

        return self.x

    def copy_array(self, x, dtype=np.int32):
        """Copy numpy array to contiguous memory.

        Parameters
        ----------
        x: np.array
            Input numpy array, not necessarily in contiguous memory.
        dtype : type
            Data type for the give numpy array.

        Returns
        -------
        xlnk.cma_array
            Physically contiguous memory.

        """
        size = x.shape
        data_buffer = self.xlnk.cma_array(shape=size, dtype=dtype)
        np.copyto(data_buffer, x.astype(dtype), casting="unsafe")
        return data_buffer

    @abstractmethod
    def ffi_interface(self):
        raise NotImplementedError("ffi_interface is not implemented.")

    @abstractmethod
    def f(self, x):
        """Abstract method for f(x)

        Your implementing class should define this method for the
        state-transition function f(x).
        Your state-transition fucntion should return a NumPy array of n
        elements representing the
        new state, and a nXn NumPy array of elements representing the the
        Jacobian of the function
        with respect to the new state.

        """
        raise NotImplementedError("Method f() is not implemented.")

    @abstractmethod
    def h(self, x):
        """Abstract method for h(x)

        Your implementing class should define this method for the
        observation function h(x), returning
        a NumPy array of m elements, and a NumPy array of m x n elements
        representing the Jacobian matrix
        H of the observation function with respect to the observation.

        """
        raise NotImplementedError("Method h() is not implemented.")
