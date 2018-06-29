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

/* states */
#define Nsta 8

/* observables */
#define Mobs 4


#pragma SDS data access_pattern(obs:SEQUENTIAL, output:SEQUENTIAL)
#pragma SDS data access_pattern(F_i:SEQUENTIAL, H_i:SEQUENTIAL)
#pragma SDS data copy(obs[0:Mobs], output[0:Nsta])
#pragma SDS data copy(F_i[0:(w1*w1)], H_i[0:(w1*w2)])
#pragma SDS data data_mover(obs:AXIDMA_SIMPLE, params:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data data_mover(fx_i:AXIDMA_SIMPLE, hx_i:AXIDMA_SIMPLE, F_i:AXIDMA_SIMPLE, H_i:AXIDMA_SIMPLE)
#pragma SDS data mem_attribute(obs:PHYSICAL_CONTIGUOUS, params:PHYSICAL_CONTIGUOUS, output:PHYSICAL_CONTIGUOUS)
#pragma SDS data mem_attribute(fx_i:PHYSICAL_CONTIGUOUS, hx_i:PHYSICAL_CONTIGUOUS, F_i:PHYSICAL_CONTIGUOUS, H_i:PHYSICAL_CONTIGUOUS)
void gps_ekf(	port_t *obs,
				port_t fx_i[Nsta],
				port_t hx_i[Mobs],
				port_t F_i[Nsta*Nsta],
				port_t H_i[Mobs*Nsta],
				port_t params[Nsta+(2*Nsta*Nsta)+(Mobs*Mobs)], 
				port_t *output,  
				int ctrl,
				int w1,
				int w2
			);

void ekf_step(	data_t x[Nsta], 
				data_t fx[Nsta], 
				data_t F[Nsta][Nsta],
				data_t hx[Mobs], 
				data_t H[Mobs][Nsta], 
				data_t P[Nsta][Nsta],
				data_t Q[Nsta][Nsta], 
				data_t R[Mobs][Mobs],
				data_t din[Mobs]
			);
			

void model(	float x[Nsta], 
			float meas[12],
			port_t *fx_i,
			port_t *hx_i,
			port_t *F_i,
			port_t *H_i
		);

void readEKF( 	data_t fx[Nsta], 
				data_t hx[Mobs], 
				data_t F[Nsta][Nsta], 
				data_t H[Mobs][Nsta], 
				port_t fx_i[Nsta],
				port_t hx_i[Mobs],
				port_t F_i[Nsta][Nsta],
				port_t H_i[Mobs][Nsta]
			);

void readKF( 	data_t fx[Nsta], 
				data_t hx[Mobs], 
				port_t fx_i[Nsta],
				port_t hx_i[Mobs]
			);

void init(	data_t x[Nsta], 
			data_t fx[Nsta], 
			data_t hx[Mobs], 
			data_t F[Nsta][Nsta], 
			data_t H[Mobs][Nsta], 
			data_t P[Nsta][Nsta], 
			data_t Q[Nsta][Nsta], 
			data_t R[Mobs][Mobs], 
			port_t *fx_i,
			port_t *hx_i,
			port_t *F_i,
			port_t *H_i,
			port_t params[Nsta+(2*Nsta*Nsta)+(Mobs*Mobs)]
		);
			

float toFloat(int32_t a);
int32_t toFixed(float a);
