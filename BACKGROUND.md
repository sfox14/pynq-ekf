# EKF Background

This document provides additional notes and slides on the Kalman Filter and Extended Kalman Filter. Much of the theory was taken from this [online course](http://ais.informatik.uni-freiburg.de/teaching/ws13/mapping/), with the notation adjusted for consistency with our designs.

## Inroduction:

The Kalman Filter (KF) and Extended Kalman Filter (EKF) are recursive state estimators for linear and non-linear systems with additive white noise. Kalman Filters are popular in edge applications because their computation involves only matrix arithmetic which is efficient to implement on most modern processors. KF's are also optimal estimators. This is because linear functions of Gaussian variables are themselves Gaussian which means that probability distribution functions (pdf's) can be computed exactly, without relying on approximations (eg. sampling, linearisation). On the otherhand, Extended Kalman Filters deal with systems which are non-linear and are thus much better suited to many real-world applications. They assume Gaussian pdf's by linearising a non-linear function about an estimate of the input's mean and covariance. This maintains much of the computational benefits of the Kalman Filter, but introduces an extra computational step involving partial derviatives and Jacobian matrices.

## 



