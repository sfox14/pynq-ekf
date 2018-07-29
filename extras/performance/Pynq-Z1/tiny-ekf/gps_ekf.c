/* gps_ekf: TinyEKF test case using You Chong's GPS example:
 * 
 *   http://www.mathworks.com/matlabcentral/fileexchange/31487-extended-kalman-filter-ekf--for-gps
 * 
 * Reads file gps.csv of satellite data and writes file ekf.csv of mean-subtracted estimated positions.
 *
 *
 * References:
 *
 * 1. R G Brown, P Y C Hwang, "Introduction to random signals and applied 
 * Kalman filtering : with MATLAB exercises and solutions",1996
 *
 * 2. Pratap Misra, Per Enge, "Global Positioning System Signals, 
 * Measurements, and Performance(Second Edition)",2006
 * 
 * Copyright (C) 2015 Simon D. Levy
 *
 * MIT License
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>

#include "tinyekf_config.h"
#include "tiny_ekf.h"

// positioning interval
static const data_t T = 1;

void blkfill(ekf_t * ekf, const data_t * a, int off)
{
    off *= 2;

    ekf->Q[off]   [off]   = a[0]; 
    ekf->Q[off]   [off+1] = a[1];
    ekf->Q[off+1] [off]   = a[2];
    ekf->Q[off+1] [off+1] = a[3];
}


void init(ekf_t * ekf)
{
    // Set Q, see [1]
    const data_t Sf    = 36;
    const data_t Sg    = 0.01;
    const data_t sigma = 5;         // state transition variance
    const data_t Qb[4] = {Sf*T+Sg*T*T*T/3, Sg*T*T/2, Sg*T*T/2, Sg*T};
    const data_t Qxyz[4] = {sigma*sigma*T*T*T/3, sigma*sigma*T*T/2, sigma*sigma*T*T/2, sigma*sigma*T};

    blkfill(ekf, Qxyz, 0);
    blkfill(ekf, Qxyz, 1);
    blkfill(ekf, Qxyz, 2);
    blkfill(ekf, Qb,   3);

    // initial covariances of state noise, measurement noise
    data_t P0 = 10;
    data_t R0 = 36;

    int i;

    for (i=0; i<8; ++i)
        ekf->P[i][i] = P0;

    for (i=0; i<4; ++i)
        ekf->R[i][i] = R0;

    // position
    ekf->x[0] = -2.168816181271560e+006;
    ekf->x[2] =  4.386648549091666e+006;
    ekf->x[4] =  4.077161596428751e+006;

    // velocity
    ekf->x[1] = 0;
    ekf->x[3] = 0;
    ekf->x[5] = 0;

    // clock bias
    ekf->x[6] = 3.575261153706439e+006;

    // clock drift
    ekf->x[7] = 4.549246345845814e+001;
}

void model(ekf_t * ekf, data_t SV[4][3])
{ 

    int i, j;

    for (j=0; j<8; j+=2) {
        ekf->fx[j] = ekf->x[j] + T * ekf->x[j+1];
        ekf->fx[j+1] = ekf->x[j+1];
    }

    for (j=0; j<8; ++j)
        ekf->F[j][j] = 1;

    for (j=0; j<4; ++j)
        ekf->F[2*j][2*j+1] = T;

    data_t dx[4][3];

    for (i=0; i<4; ++i) {
        ekf->hx[i] = 0;
        for (j=0; j<3; ++j) {
            data_t d = ekf->fx[j*2] - SV[i][j];
            dx[i][j] = d;
            ekf->hx[i] += d*d;
        }
        ekf->hx[i] = sqrt(ekf->hx[i]) + ekf->fx[6];
	//printf("%f\n", ekf->hx[i]);
    }
    //printf("\n");

    for (i=0; i<4; ++i) {
        for (j=0; j<3; ++j) 
            ekf->H[i][j*2]  = dx[i][j] / ekf->hx[i];
        ekf->H[i][6] = 1;
    }   
}

// top function
void gps_ekf(data_t *xin, data_t *output, int datalen)
{

    data_t SV_Pos[Nsats][Nxyz];
    data_t SV_Rho[Nsats];

    // initialisation
    ekf_t ekf;
    ekf_init(&ekf, Nsta, Mobs);
    init(&ekf);

    for (int i=0; i<datalen; i++) {
        // read xin into SV_Pos and SV_Rho
        for (int j=0; j<Nsats; j++) {
            for (int k=0; k<Nxyz; k++) {
                SV_Pos[j][k] = xin[i*(Nsats*(Nxyz+1)) + (j*Nxyz + k)];
            }
        }
        for (int j=0; j<Nsats; j++) {
            SV_Rho[j] = xin[i*(Nsats*(Nxyz+1)) + (Nsats*Nxyz) + j];
        }

        // model
        model(&ekf, SV_Pos);

        // ekf_step
        ekf_step(&ekf, SV_Rho);

        // return positions, ignoring velocities
        for (int k=0; k<Nxyz; k++) {
            output[i*Nxyz + k] = ekf.x[2*k];
        }

    }


}
