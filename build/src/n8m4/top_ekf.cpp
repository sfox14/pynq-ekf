#include "ekf_config.h"


void init(	data_t P[Nsta][Nsta], 
			data_t Q[Nsta][Nsta], 
			data_t R[Mobs][Mobs], 
			port_t params[(2*Nsta*Nsta)+(Mobs*Mobs)]
		)
{
	
	#pragma HLS INLINE off
	
	data_t local_mem[(2*Nsta*Nsta)+(Mobs*Mobs)];
	int ptotal = (2*Nsta*Nsta)+(Mobs*Mobs);
	
	// read params
load_params: for (int i=0; i<ptotal; i++) {
		#pragma HLS PIPELINE
		local_mem[i].V = params[i].range(bit_width-1,0);
	}
	
	
	/* params[(2*Nsta*Nsta)+(Mobs*Mobs)]:
		1 - P[Nsta*Nsta]
		2 - Q[Nsta*Nsta]
		3 - R[Mobs*Mobs]
	*/
	
	int offset = 0;
	
	// read state vector x
	//for (int i=0; i<Nsta; i++) {
	//	#pragma HLS PIPELINE
	//	x[i] = local_mem[i];
	//}
	//offset += Nsta;
	
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
			
}

// top function
void top_ekf( 	port_t obs[Mobs], 
				port_t fx_i[Nsta],
				port_t hx_i[Mobs],
				port_t F_i[Nsta*Nsta],
				port_t H_i[Mobs*Nsta],
				port_t params[(2*Nsta*Nsta)+(Mobs*Mobs)], 
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
	static data_t F[Nsta][Nsta] = {{0}};
	static data_t H_1[Mobs][Nsta] = {{0}};
	static data_t H_2[Mobs][Nsta] = {{0}}; // copy in hw
	
	/* ------------------ Fixed Covariance Matrices -------------------- */
	
	// prediction error covariance, i.e. x_new ~ N(fx, P)
	static data_t P[Nsta][Nsta] = {{0}};
	
	// process/state noise covariance, i.e. x ~ N(u, Q) 
	static data_t Q[Nsta][Nsta] = {{0}};
	
	// measurement/observed error covariance, i.e. z ~ N(u, R)
	static data_t R[Mobs][Mobs] = {{0}};
	
	/* ------------------ Transposed Jacobians -------------------------- */
	
	static data_t Ft[Nsta][Nsta] = {{0}};
	static data_t Ht_1[Nsta][Mobs] = {{0}};
	static data_t Ht_2[Nsta][Mobs] = {{0}};

	/* ---------------------- Control Inputs --------------------------- */
	
	int sig = ctrl;
	
	/* ---------------------- HLS PRAGMAs ----------------------------- */
	//step1_1
	#pragma HLS array_partition variable=F block factor=8 dim=2
	#pragma HLS array_partition variable=P block factor=8 dim=1
	
	//step1_2
	//#pragma HLS array_partition variable=tmp0 block factor=8 dim=2
	#pragma HLS array_partition variable=Ft block factor=8 dim=1
	
	//step2_1
	#pragma HLS array_partition variable=H_1 block factor=8 dim=2
	//#pragma HLS array_partition variable=Pp block factor=8 dim=2
	
	//step2_2
	//#pragma HLS array_partition variable=Pp_1 block factor=8 dim=2
	#pragma HLS array_partition variable=Ht_1 block factor=8 dim=1
	
	//step2_3
	//#pragma HLS array_partition variable=tmp6 block factor=8 dim=2
	#pragma HLS array_partition variable=Ht_2 block factor=8 dim=1
	
	//step2_4
	//#pragma HLS array_partition variable=tmp1 block factor=8 dim=2
	//#pragma HLS array_partition variable=tmp4 block factor=8 dim=1
	
	//step4_1
	// maybe replicate H (needs to be dim=1)
	#pragma HLS array_partition variable=H_2 block factor=4 dim=1  //M
	//#pragma HLS array_partition variable=K block factor=4 dim=2
	
	//step4_2
	// maybe replicate Pp (needs to be dim=1
	//#pragma HLS array_partition variable=Pp_2 block factor=8 dim=1
	//#pragma HLS array_partition variable=tmp7 block factor=8 dim=2
	
	//#pragma HLS RESOURCE variable=H_1 core=RAM_S2P_BRAM
	
	/* ----------------------- Read Input Data ------------------------- */
	
	// read H and F Jacobians
	/* w1=0 and w2=0 when KF only */
load_F:	for (int i=0; i<w1; i++) {
load_F_i:	for (int j=0; j<w1; j++) {
			#pragma HLS PIPELINE
			F[i][j].V = F_i[i*w1 + j].range(bit_width-1,0);
			Ft[j][i] = F[i][j];
		}
	}
load_H:	for (int i=0; i<w2; i++) {
load_H_i:	for (int j=0; j<w1; j++) {
			#pragma HLS PIPELINE
			H_1[i][j].V = H_i[i*w1 + j].range(bit_width-1,0);
			data_t imm = H_1[i][j];
			H_2[i][j] = imm;
			Ht_1[j][i] = imm;
			Ht_2[j][i] = Ht_1[j][i]; // we use Ht twice
		}
	}
	
	// read state function fx
load_fx:	for (int i=0; i<Nsta; i++) {
		#pragma HLS PIPELINE
		fx[i].V = fx_i[i].range(bit_width-1,0);
	}
	
	// read measurement function hx
load_hx:	for (int i=0; i<Mobs; i++) {
		#pragma HLS PIPELINE
		hx[i].V = hx_i[i].range(bit_width-1,0);
	}
	
	/* ------------------------ Init --------------------------------- */
	// w3=(2*Nsta*Nsta)+(Mobs*Mobs) when sig=1
	if (sig==0) {
		init(P, Q, R, params);
	}
	
	/* --------------------------------------------------------------- */
	

	// read measurements
	for (int i=0; i<Mobs; i++) {
		#pragma HLS PIPELINE
		din[i].V = obs[i].range(bit_width-1, 0);
	}

	// ekf_step
    ekf_step(x, fx, hx, F, H_1, H_2, P, Q, R, Ft, Ht_1, Ht_2, din);
	
	
	// write output
	for (int k=0; k<Nsta; k++) {
		#pragma HLS PIPELINE
		port_t imm;
		imm.range(bit_width-1,0) = x[k].V;
		output[k] = imm;
	}
	
}
