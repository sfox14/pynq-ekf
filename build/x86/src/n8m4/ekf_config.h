
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

/* If Z1 */
//#define Z1_ENABLE 0

/* P_ENABLE=1 for larger devices or small problems, eg. zcu104, n8m4 */
#define P_ENABLE 0

/* 	Inner-loop block factors:
	------------------------
		(BSIZE_1 <= Mobs) and (Mobs%BSIZE_1 == 0)
		(BSIZE_2 <= Nsta) and (Nsta%BSIZE_2 == 0)
		(BSIZE_3 <= Mobs) and (Mobs%BSIZE_3 == 0)
		(BSIZE_4 <= Nsta) and (Nsta%BSIZE_4 == 0)
*/
#define BSIZE_1 4
#define BSIZE_2 8
#define BSIZE_3 4
#define BSIZE_4 8

/* Assign 1 if the BSIZE_1 != Mobs or BSIZE_2 != Nsta */
#define PARTIAL_N 0
#define PARTIAL_M 0

/* Assign 1 if BSIZE_4 != Nsta */
#define PARTIAL_N_2 0


#ifdef __cplusplus
extern "C" {
#endif

#pragma SDS data access_pattern(obs:SEQUENTIAL, output:SEQUENTIAL)
//#pragma SDS data access_pattern(F_i:SEQUENTIAL, H_i:SEQUENTIAL)
#pragma SDS data copy(obs[0:Mobs], params[0: ((2*Nsta*Nsta)+(Mobs*Mobs))], output[0:Nsta])
#pragma SDS data copy(F_i[0:(w1*w1)], H_i[0:(w1*w2)])
#pragma SDS data data_mover(obs:AXIDMA_SIMPLE, params:AXIDMA_SIMPLE, output:AXIDMA_SIMPLE)
#pragma SDS data data_mover(fx_i:AXIDMA_SIMPLE, hx_i:AXIDMA_SIMPLE, F_i:AXIDMA_SIMPLE, H_i:AXIDMA_SIMPLE)
#pragma SDS data mem_attribute(obs:PHYSICAL_CONTIGUOUS|NON_CACHEABLE, params:PHYSICAL_CONTIGUOUS|NON_CACHEABLE, output:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
#pragma SDS data mem_attribute(fx_i:PHYSICAL_CONTIGUOUS|NON_CACHEABLE, hx_i:PHYSICAL_CONTIGUOUS|NON_CACHEABLE, F_i:PHYSICAL_CONTIGUOUS|NON_CACHEABLE, H_i:PHYSICAL_CONTIGUOUS|NON_CACHEABLE)
void top_ekf(	port_t *obs,
				port_t fx_i[Nsta],
				port_t hx_i[Mobs],
				port_t F_i[Nsta*Nsta],
				port_t H_i[Mobs*Nsta],
				port_t params[(2*Nsta*Nsta)+(Mobs*Mobs)], 
				port_t *output,  
				int ctrl,
				int w1,
				int w2
			);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif			
void ekf_step(	data_t x[Nsta], 
				data_t fx[Nsta],
				data_t hx[Mobs],				
				data_t F[Nsta][Nsta],	 
				data_t H_1[Mobs][Nsta],
				data_t H_2[Mobs][Nsta],				
				data_t P[Nsta][Nsta],
				data_t Q[Nsta][Nsta], 
				data_t R[Mobs][Mobs],
				data_t Ft[Nsta][Nsta],	 
				data_t Ht_1[Nsta][Mobs],
				data_t Ht_2[Nsta][Mobs],
				data_t din[Mobs]
			);
#ifdef __cplusplus
}
#endif			

void init(	data_t P[Nsta][Nsta], 
			data_t Q[Nsta][Nsta], 
			data_t R[Mobs][Mobs], 
			port_t params[(2*Nsta*Nsta)+(Mobs*Mobs)]
		);
