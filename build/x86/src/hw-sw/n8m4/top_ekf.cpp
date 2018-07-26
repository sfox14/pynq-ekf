#include <math.h>
#include "ekf_config.h"


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
		)
{
	
	#pragma HLS INLINE off
	
	data_t local_mem[Nsta+(2*Nsta*Nsta)+(Mobs*Mobs)];
	int ptotal = Nsta+(2*Nsta*Nsta)+(Mobs*Mobs);
	
	// must read data sequentially
	for (int i=0; i<ptotal; i++) {
		#pragma HLS PIPELINE
		local_mem[i].V = params[i].range(bit_width-1,0);
	}
	
	/* params[Nsta+Nsta+Mobs+(2*Nsta*Nsta)+(Mobs*Nsta)+2]:
		1 - x[Nsta]
		2 - P[Nsta*Nsta]
		3 - Q[Nsta*Nsta]
		4 - R[Mobs*Mobs]
	*/
	
	int offset = 0;
	
	// read state vector x
	for (int i=0; i<Nsta; i++) {
		#pragma HLS PIPELINE
		x[i] = local_mem[i];
	}
	offset += Nsta;
	
	// read state/prediction covariance
	for (int i=0; i<Nsta; i++) {
		for (int j=0; j<Nsta; j++) {
			#pragma HLS PIPELINE
			P[i][j] = local_mem[i*Nsta + j + offset];
		}
	}
	offset += (Nsta*Nsta);
	
	// read process covariance
	for (int i=0; i<Nsta; i++) {
		for (int j=0; j<Nsta; j++) {
			#pragma HLS PIPELINE
			Q[i][j] = local_mem[i*Nsta + j + offset];
		}
	}
	offset += (Nsta*Nsta);
	
	// read process covariance
	for (int i=0; i<Mobs; i++) {
		for (int j=0; j<Mobs; j++) {
			#pragma HLS PIPELINE
			R[i][j] = local_mem[i*Mobs + j + offset];
		}
	}
	
	//-----------------------------------------------------
	
	// read state function fx
	for (int i=0; i<Nsta; i++) {
		#pragma HLS PIPELINE
		fx[i].V = fx_i[i].range(bit_width-1,0);
	}
	
	// read measurement function hx
	for (int i=0; i<Mobs; i++) {
		#pragma HLS PIPELINE
		hx[i].V = hx_i[i].range(bit_width-1,0);
	}
	
	// read state Jacobian
	for (int i=0; i<Nsta; i++) {
		for (int j=0; j<Nsta; j++) {
			#pragma HLS PIPELINE
			F[i][j].V = F_i[i*Nsta + j].range(bit_width-1,0);
		}
	}
	
	// read measurement Jacobian
	for (int i=0; i<Mobs; i++) {
		for (int j=0; j<Nsta; j++) {
			#pragma HLS PIPELINE
			H[i][j].V = H_i[i*Nsta + j].range(bit_width-1,0);
		}
	}
	
	//--------------------------------------------------------
		
}


void readEKF(	data_t fx[Nsta], 
				data_t hx[Mobs], 
				data_t F[Nsta][Nsta], 
				data_t H[Mobs][Nsta], 
				port_t *fx_i,
				port_t *hx_i,
				port_t *F_i,
				port_t *H_i
			)
{
	
	int i, j;
    
	// read from off-chip
	for (int i=0; i<Nsta; i++) {
		fx[i].V = fx_i[i].range(bit_width-1,0);
	}
		
	for (int i=0; i<Mobs; i++) {
		hx[i].V = hx_i[i].range(bit_width-1,0);
	}
	
	for (int i=0; i<Nsta; i++) {
		for (int j=0; j<Nsta; j++) {
			F[i][j].V = F_i[i*Nsta + j].range(bit_width-1,0);
		}
	}
	
	for (int i=0; i<Mobs; i++) {
		for (int j=0; j<Nsta; j++) {
			H[i][j].V = H_i[i*Nsta + j].range(bit_width-1,0);
		}
	}
	
}

void readKF(	data_t fx[Nsta], 
				data_t hx[Mobs],
				port_t *fx_i,
				port_t *hx_i
			)
{
	
	int i, j;
    
	// read from off-chip
	for (int i=0; i<Nsta; i++) {
		fx[i].V = fx_i[i].range(bit_width-1,0);
	}
		
	for (int i=0; i<Mobs; i++) {
		hx[i].V = hx_i[i].range(bit_width-1,0);
	}
		
}


// top function
void top_ekf( 	port_t obs[Mobs], 
				port_t fx_i[Nsta],
				port_t hx_i[Mobs],
				port_t F_i[Nsta*Nsta],
				port_t H_i[Mobs*Nsta],
				port_t params[Nsta+(2*Nsta*Nsta)+(Mobs*Mobs)], 
				port_t output[Nsta], 
				int ctrl,
				int w1,
				int w2
			)
{

	// inputs
    data_t din[Mobs];

    /* ---------------- Application Specific Variables ------------------ */

	static data_t x[Nsta];
	
	// output of state-transition function
	static data_t fx[Nsta] = {0};
	
	// output of observation/measurement function
	static data_t hx[Mobs] = {0};
	
	// Jacobians
	static data_t F[Nsta][Nsta] = {0};
	static data_t H[Mobs][Nsta] = {{0}};
	
	/* ------------------ Fixed Covariance Matrices -------------------- */
	
	// prediction error covariance, i.e. x_new ~ N(fx, P)
	static data_t P[Nsta][Nsta] = {{0}};
	
	// process/state noise covariance, i.e. x ~ N(u, Q) 
	static data_t Q[Nsta][Nsta] = {{0}};
	
	// measurement/observed error covariance, i.e. z ~ N(u, R)
	static data_t R[Mobs][Mobs] = {{0}};

	int sig = ctrl;
	
	if (sig == 0) {
		printf("sig=0\n");
		init(x, fx, hx, F, H, P, Q, R, fx_i, hx_i, F_i, H_i, params);
	} else if (sig == 1) {
		printf("sig=1\n");
		readEKF(fx, hx, F, H, fx_i, hx_i, F_i, H_i);
	} else if (sig == 2) {
		printf("sig=2\n");
		readKF(fx, hx, fx_i, hx_i);
	}
	
	
	/* ---------------------- HLS PRAGMAs ----------------------------- */

	/* ---------------------------------------------------------------- */
	

	// read measurements
	for (int i=0; i<Mobs; i++) {
		#pragma HLS PIPELINE
		din[i].V = obs[i].range(bit_width-1, 0);
	}

	// ekf_step
    ekf_step(x, fx, F, hx, H, P, Q, R, din);
	
	
	// write output
	for (int k=0; k<Nsta; k++) {
		#pragma HLS PIPELINE
		port_t imm;
		imm.range(bit_width-1,0) = x[k].V;
		output[k] = imm;
	}
	
}
