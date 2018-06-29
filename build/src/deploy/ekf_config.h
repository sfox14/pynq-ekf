/*  This code is adapted from "TinyEKF: Extended Kalman Filter
	for embedded processors", https://github.com/simondlevy/TinyEKF
*/

#include <stdio.h>
#include <stdlib.h>
#include <hls_math.h>
#include <ap_int.h>
#include <ap_fixed.h>
using namespace std;

#define bit_width 32
#define frac_width 20
typedef ap_fixed<bit_width, (bit_width-frac_width)> data_t;
typedef ap_uint<bit_width> port_t;

//typedef ap_uint<bit_width> data_t;
//typedef float data_t;

/* states */
#define Nsta 8

/* observables */
#define Mobs 4

/* Satellites and Co-ordinate system */
#define Nsats 4  // number of satellites
#define Nxyz 3   // (x,y,z) co-ordinate system


#ifdef __cplusplus
extern "C" {
#endif

#pragma SDS data access_pattern(xin:SEQUENTIAL, output:SEQUENTIAL)
#pragma SDS data copy(xin[0:(datalen*(Nsats*(Nxyz+1)))], output[0:(datalen*Nxyz)])
#pragma SDS data data_mover(xin:AXIDMA_SIMPLE, params:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE, pout:AXIDMA_SIMPLE)
#pragma SDS data mem_attribute(xin:PHYSICAL_CONTIGUOUS, params:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS, pout: PHYSICAL_CONTIGUOUS)
void gps_ekf(port_t *xin, port_t params[182], port_t *output, port_t pout[Nsta*Nsta], int datalen);

#ifdef __cplusplus
}
#endif

void ekf_step(	data_t x[Nsta], 
				data_t fx[Nsta], 
				data_t F[Nsta][Nsta],
				data_t hx[Mobs], 
				data_t H[Mobs][Nsta], 
				data_t P[Nsta][Nsta],
				data_t Q[Nsta][Nsta], 
				data_t R[Mobs][Mobs],
				data_t G[Nsta][Mobs],
				data_t Pp[Nsta][Nsta],
				data_t SV_Rho[Mobs],
				data_t Ft[Nsta][Nsta],
				data_t Ht[Nsta][Mobs],
				data_t tmp0[Nsta][Nsta],
				data_t tmp1[Nsta][Mobs],
				data_t tmp2[Nsta],
				data_t tmp3[Mobs][Mobs],
				data_t tmp4[Mobs][Mobs],
				data_t tmp5[Mobs],
				data_t tmp6[Mobs][Nsta],
				data_t tmp7[Nsta][Nsta]				
			);
			
void model(data_t x[Nsta], data_t fx[Nsta], data_t F[Nsta][Nsta], data_t hx[Mobs],
			data_t H[Mobs][Nsta], data_t SV[4][3]);

float toFloat(int32_t a);
int32_t toFixed(float a);

