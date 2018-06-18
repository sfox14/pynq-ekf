
/*  This code is adapted from "TinyEKF: Extended Kalman Filter
	for embedded processors", https://github.com/simondlevy/TinyEKF

	gps_ekf: TinyEKF test case using You Chong's GPS example:

		http://www.mathworks.com/matlabcentral/fileexchange/31487-extended-kalman-filter-ekf--for-gps

	Reads file gps.csv of satellite data and writes file ekf.csv of mean-subtracted estimated positions.

*/

#include "ekf_config.h"


void init(data_t x[Nsta], data_t fx[Nsta], data_t hx[Mobs], data_t F[Nsta][Nsta],
			data_t H[Mobs][Nsta], data_t P[Nsta][Nsta], data_t Q[Nsta][Nsta],
				data_t R[Mobs][Mobs], data_t local_mem[182])
{

	#pragma HLS INLINE off

	data_t qval, rval;

	/* params[Nsta+Nsta+Mobs+(2*Nsta*Nsta)+(Mobs*Nsta)+2]:
		1 - x[Nsta]
		2 - fx[Nsta]
		3 - hx[Mobs]
		4 - F[Nsta*Nsta]
		5 - H[Mobs*Nsta]
		6 - P[Nsta*Nsta]
		7 - qval
		8 - rval
	*/

	int offset = 0;

	// read state vector x
	for (int i=0; i<Nsta; i++) {
		#pragma HLS PIPELINE
		x[i] = local_mem[i];
	}
	offset += Nsta;

	// read state function fx
	for (int i=0; i<Nsta; i++) {
		#pragma HLS PIPELINE
		fx[i] = local_mem[i + offset];
	}
	offset += Nsta;

	// read measurement function hx
	for (int i=0; i<Mobs; i++) {
		#pragma HLS PIPELINE
		hx[i] = local_mem[i + offset];
	}
	offset += Mobs;

	// read state Jacobian
	for (int i=0; i<Nsta; i++) {
		for (int j=0; j<Nsta; j++) {
			#pragma HLS PIPELINE
			F[i][j] = local_mem[i*Nsta + j + offset];
		}
	}
	offset += (Nsta*Nsta);

	// read measurement Jacobian
	for (int i=0; i<Mobs; i++) {
		for (int j=0; j<Nsta; j++) {
			#pragma HLS PIPELINE
			H[i][j] = local_mem[i*Nsta + j + offset];
		}
	}
	offset += (Mobs*Nsta);

	// read state/prediction covariance
	for (int i=0; i<Nsta; i++) {
		for (int j=0; j<Nsta; j++) {
			#pragma HLS PIPELINE
			P[i][j] = local_mem[i*Nsta + j + offset];
		}
	}
	offset += (Nsta*Nsta);

	qval = local_mem[offset];
	rval = local_mem[offset+1];

	// read Q
	for (int i=0; i<Nsta; i++) {
		#pragma HLS PIPELINE
		Q[i][i] = qval;
	}

	// read R
	for (int i=0; i<Mobs; i++) {
		#pragma HLS PIPELINE
		R[i][i] = rval;
	}

}


void model(data_t x[Nsta], data_t fx[Nsta], data_t F[Nsta][Nsta],
			data_t hx[Mobs], data_t H[Mobs][Nsta], data_t SV[4][3])
{
	// This function is application dependent. You need to know f(x) and h(x), and then
	// compute the partial derivatives w.r.t. the state vector. In this example, f(x) is
	// a constant linear function. This means the F Jacobian matrix will not change each
	// iteration.


    int i, j;
    data_t imm;

	/* compute f(x) */
    for (j=0; j<8; j+=2) {
        fx[j] = x[j] + x[j+1];
        fx[j+1] = x[j+1];
    }

	/* compute new F Jacobian */
	// F is constant at init

	/* compute h(x) */
    data_t dx[4][3];
    for (i=0; i<4; ++i) {
        hx[i] = 0;
        for (j=0; j<3; ++j) {
            data_t d = fx[j*2] - SV[i][j];
            dx[i][j] = d;
            hx[i] += d*d;
        }
        imm = hx[i];
        hx[i] = hls::sqrt(imm);
		hx[i] = hx[i] + fx[6]; // pow(hx[i], 0.5)
    }

	/* compute new H Jacobian */
    for (i=0; i<4; ++i) {
        for (j=0; j<3; ++j) {
        	H[i][j*2] = dx[i][j]/hx[i];
    	}
        H[i][6] = 1.0;
    }

}

// top function
void gps_ekf(port_t *xin, port_t params[182], port_t *output,
				port_t pout[Nsta*Nsta], int datalen)
{

	// inputs
    data_t SV_Pos[Nsats][Nxyz];
    data_t SV_Rho[Nsats];

    /* ---------------- Application Specific Variables ------------------ */

	static data_t x[Nsta] = {0};

	// output of state-transition function
	static data_t fx[Nsta] = {0};

	// output of observation/measurement function
	static data_t hx[Mobs] = {0};

	// Jacobians
	static data_t F[Nsta][Nsta] = {{0}};
	static data_t H[Mobs][Nsta] = {{0}};

	/* ------------------ Fixed Covariance Matrices -------------------- */

	// prediction error covariance, i.e. x_new ~ N(fx, P)
	static data_t P[Nsta][Nsta] = {{0}};

	// process/state noise covariance, i.e. x ~ N(u, Q)
	static data_t Q[Nsta][Nsta] = {{0}};

	// measurement/observed error covariance, i.e. z ~ N(u, R)
	static data_t R[Mobs][Mobs] = {{0}};

	// Kalman Gain
	static data_t G[Nsta][Mobs] = {{0}};

	// post-prediction, pre-update, P
	static data_t Pp[Nsta][Nsta] = {{0}};

	/* ----------------- Temporary Variables --------------------------- */

	static data_t Ht[Nsta][Mobs] = {{0}};
	static data_t Ft[Nsta][Nsta] = {{0}};

	static data_t tmp0[Nsta][Nsta] = {{0}};
	static data_t tmp1[Nsta][Mobs] = {{0}};
	static data_t tmp2[Nsta] = {0};
	static data_t tmp3[Mobs][Mobs] = {{0}};
	static data_t tmp4[Mobs][Mobs] = {{0}};
	static data_t tmp5[Mobs] = {0};
	static data_t tmp6[Mobs][Nsta] = {{0}};
	static data_t tmp7[Nsta][Nsta] = {{0}};

	/* ----------------- Load Intital Params -------------------------- */
	data_t local_mem[182]; //[(((2*Nsta)+2+Mobs)*Nsta)+Mobs+2];
	// read params sequentially
	for (int i=0; i<182; i++) {
		#pragma HLS PIPELINE
		port_t imm = params[i];
		local_mem[i].V = imm.range(bit_width-1,0);
	}
	// Initialise state
	init(x, fx, hx, F, H, P, Q, R, local_mem);

	/* ---------------------- HLS PRAGMAs ----------------------------- */

	/* ---------------------------------------------------------------- */

	static const int num_inputs = Nsats*(Nxyz+1);
	data_t local_xin[num_inputs];

    for (int i=0; i<datalen; i++) {  //datalen

        // read xin into local memory
        for (int j=0; j<num_inputs; j++) {
			#pragma HLS PIPELINE
			local_xin[j].V = xin[i*num_inputs + j].range(bit_width-1, 0);
		}

		// SV_Pos and SV_rho
		for (int j=0; j<Nsats; j++)	{
            for (int k=0; k<Nxyz; k++) {
				#pragma HLS PIPELINE
				SV_Pos[j][k] = local_xin[j*Nxyz + k];
            }
        }
        for (int j=0; j<Nsats; j++) {
			#pragma HLS PIPELINE
            SV_Rho[j] = local_xin[j + num_inputs - Nsats];
        }


        // model
        model(x, fx, F, hx, H, SV_Pos);

        // ekf_step
        ekf_step(x, fx, F, hx, H, P, Q, R, G, Pp, SV_Rho, Ft, Ht,
					tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7);


        // return positions, ignoring velocities
        for (int k=0; k<Nxyz; k++) {
        	port_t imm;
        	imm.range(bit_width-1,0) = x[2*k].V;
            output[i*Nxyz + k] = imm;
        }

    }

	// write out P covariance matrix
	for (int i=0; i<Nsta; i++) {
		for (int j=0; j<Nsta; j++) {
			port_t imm;
			imm.range(bit_width-1,0) = P[i][j].V;
			pout[i*Nsta + j] = imm;
		}
	}

}
