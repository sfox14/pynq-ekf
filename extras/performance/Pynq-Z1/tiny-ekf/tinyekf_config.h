/*
 * TinyEKF: Extended Kalman Filter for embedded processors.
 *
 * tinyekf_config.h: static configuration parameters
 *
 * Copyright (C) 2015 Simon D. Levy
 *
 * MIT License
 */


typedef float data_t;

/* states */
#define Nsta 8

/* observables */
#define Mobs 4

/* Satellites and Co-ordinate system */
#define Nsats 4  // number of satellites
#define Nxyz 3   // (x,y,z) co-ordinate system


typedef struct {

    int n;           /* number of state values */
    int m;           /* number of observables */

    data_t x[Nsta];     /* state vector */

    data_t P[Nsta][Nsta];  /* prediction error covariance */
    data_t Q[Nsta][Nsta];  /* process noise covariance */
    data_t R[Mobs][Mobs];  /* measurement error covariance */

    data_t G[Nsta][Mobs];  /* Kalman gain; a.k.a. K */

    data_t F[Nsta][Nsta];  /* Jacobian of process model */
    data_t H[Mobs][Nsta];  /* Jacobian of measurement model */

    data_t Ht[Nsta][Mobs]; /* transpose of measurement Jacobian */
    data_t Ft[Nsta][Nsta]; /* transpose of process Jacobian */
    data_t Pp[Nsta][Nsta]; /* P, post-prediction, pre-update */

    data_t fx[Nsta];   /* output of user defined f() state-transition function */
    data_t hx[Mobs];   /* output of user defined h() measurement function */

    /* temporary storage */
    data_t tmp0[Nsta][Nsta];
    data_t tmp1[Nsta][Mobs];
    data_t tmp2[Mobs][Nsta];
    data_t tmp3[Mobs][Mobs];
    data_t tmp4[Mobs][Mobs];
    data_t tmp5[Mobs]; 

} ekf_t;


void model(ekf_t * ekf, data_t SV[4][3]);
void init(ekf_t * ekf);
void blkfill(ekf_t * ekf, const data_t * a, int off);
void gps_ekf(data_t *xin, data_t *output, int datalen);
