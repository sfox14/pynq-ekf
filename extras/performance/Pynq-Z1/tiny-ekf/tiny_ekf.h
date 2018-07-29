/*
 * TinyEKF: Extended Kalman Filter for embedded processors.
 *
 * Copyright (C) 2015 Simon D. Levy
 *
 * MIT License
 */

typedef float data_tt; 

void ekf_init(void * ekf, int n, int m);

/**
  * Runs one step of EKF prediction and update. Your code should first build a model, setting
  * the contents of <tt>ekf.fx</tt>, <tt>ekf.F</tt>, <tt>ekf.hx</tt>, and <tt>ekf.H</tt> to appropriate values.
  * @param ekf pointer to structure EKF 
  * @param z array of measurement (observation) values
  * @return 0 on success, 1 on failure caused by non-positive-definite matrix.
  */
int ekf_step(void * ekf, data_tt * z);
