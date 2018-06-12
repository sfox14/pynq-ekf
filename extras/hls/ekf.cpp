
/*  This code is adapted from "TinyEKF: Extended Kalman Filter
	for embedded processors", https://github.com/simondlevy/TinyEKF
*/

#include "ekf_config.h"


// LU Decomposition
static void LU(data_t A[Mobs][Mobs])
{
	int i,j,k;
	data_t x;
	
	for(k=0;k<Mobs-1;k++) {
		for(j=k+1;j<Mobs;j++) {
			x = (data_t)(A[j][k])/A[k][k];
			for(i=k;i<Mobs;i++) {  
				A[j][i] = A[j][i] - (x * A[k][i]);
			}
		A[j][k] = x;
		}
	}

}	

// Matrix Inversion via LU Decomposition
static void LUInv(data_t A[Mobs][Mobs], data_t Ainv[Mobs][Mobs])
{
	#pragma HLS INLINE off
	#pragma HLS INLINE region
	
	int i,j,m;
	data_t d[Mobs], y[Mobs];
	data_t x1, x2;
	
	LU(A);
	
	// Find the Inverse
 	for(m=0; m<Mobs; m++) { 
        // only d[m] = 1
		for(i=0; i<Mobs; i++) {
			d[i] = 0;
		}
		d[m]=1.0;
		
		for(i=0; i<Mobs; i++) { 
			x1 = 0.0; 
			for(j=0; j<=i-1; j++) { 
				x1 = x1 + A[i][j] * y[j];
			}
			y[i]=(d[i] - x1);
		}

		for (i=Mobs-1; i>=0; i--) { 
			x2 = 0.0; 
			for(j=i+1;j<Mobs;j++) { 
				x2 = x2 + A[i][j] * Ainv[j][m];
			}
			data_t term = (y[i] - x2);
			Ainv[i][m] = (data_t)(term)/A[i][i];
		}
	}
}






/* m */
static void choldc1(data_t a[Mobs][Mobs], data_t p[Mobs]) 
{
    int i,j,k;
    data_t sum, imm;

    for (i = 0; i < Mobs; i++) {
        for (j = i; j < Mobs; j++) {
            sum = a[i][j];
            for (k = i - 1; k >= 0; k--) {
                sum -= a[i][k] * a[j][k];
            }
            if (i == j) {
                p[i] = hls::sqrt(sum);
            }
            else {
				imm = (data_t)(sum)/p[i];
                a[j][i] = imm;
            	//a[j][i] = sum/p[i];
            }
        }
    }
	
}

/* m */
static void choldcsl(data_t A[Mobs][Mobs], data_t a[Mobs][Mobs], data_t p[Mobs]) 
{	
	#pragma HLS INLINE region

    int i,j,k;
    data_t sum, imm0, imm1;
    for (i = 0; i < Mobs; i++) {
        for (j = 0; j < Mobs; j++) {
            a[i][j] = A[i][j];
        }
    }
	choldc1(a, p);
	
	for (i = 0; i < Mobs; i++) {
        imm0 = (data_t)(1)/p[i];
		a[i][i] = imm0;
        //a[i][i] = 1/p[i];
		for (j = i + 1; j < Mobs; j++) {
            sum = 0;
            for (k = i; k < j; k++) {
                sum -= a[j][k] * a[k][i];
            }
            imm1 = (data_t)(sum)/p[j];
            a[j][i] = imm1;
		}
    }

}

/* m */
static void cholsl(data_t A[Mobs][Mobs], data_t a[Mobs][Mobs], data_t p[Mobs]) 
{
	#pragma HLS INLINE off
	#pragma HLS INLINE region
	
	//#pragma HLS pipeline II=512
	
	int i,j,k;
	
    choldcsl(A,a,p);
    
	for (i = 0; i < Mobs; i++) {
        for (j = i + 1; j < Mobs; j++) {
            a[i][j] = 0.0;
        }
    }
    for (i = 0; i < Mobs; i++) {
        a[i][i] *= a[i][i];
        for (k = i + 1; k < Mobs; k++) {
            a[i][i] += a[k][i] * a[k][i];
        }
        for (j = i + 1; j < Mobs; j++) {
            for (k = j; k < Mobs; k++) {
                a[i][j] += a[k][i] * a[k][j];
            }
        }
    }
	
    for (i = 0; i < Mobs; i++) {
        for (j = 0; j < i; j++) {
            a[i][j] = a[j][i];
        }
    }
}


/* tmp0 = F * P */
static void step1_1(data_t F[Nsta][Nsta], data_t P[Nsta][Nsta], 
				data_t tmp0[Nsta][Nsta], data_t Ft[Nsta][Nsta])
{
	#pragma HLS inline off
	
	int i, j, l;
	
	for (i=0; i<Nsta; i++) {
		for (j=0; j<Nsta; j++) {
			#pragma HLS pipeline
			data_t result = 0;
			for (l=0; l<Nsta; l++) {
				data_t op1 = F[i][l];
				data_t term = P[l][j];
				Ft[l][i] = op1;
				if (op1 == 1) {
					result += term;
				}
			}
			tmp0[i][j] = result;
		}
			
	}
}

/* Pp = tmp0 * F^T + Q */
static void step1_2(data_t tmp0[Nsta][Nsta], data_t Ft[Nsta][Nsta],
				data_t Q[Nsta][Nsta], data_t Pp[Nsta][Nsta])
{
	#pragma HLS inline off
	
	int i, j, l;
	
	for (i=0; i<Nsta; i++) {
		for (j=0; j<Nsta; j++) {
			#pragma HLS pipeline
			data_t result = 0;
			for (l=0; l<Nsta; l++) {
				data_t term = tmp0[i][l] * Ft[l][j];
				result += term;
			}
			Pp[i][j] = result + Q[i][j];
		}
	}

}


/* tmp6 = H * Pp */
static void step2_1(data_t H[Mobs][Nsta], data_t Pp[Nsta][Nsta],
				data_t Ht[Nsta][Mobs], data_t tmp6[Mobs][Nsta])
{

	#pragma HLS inline off
	
	int i, j, l;
	
	for (i=0; i<Mobs; i++) {
		for (j=0; j<Nsta; j++) {
			#pragma HLS pipeline
			data_t result = 0;
			for (l=0; l<Nsta; l++) {
				data_t op1 = H[i][l];
				data_t term = op1 * Pp[j][l];
				result += term;
				Ht[l][i] = op1; //transpose
			}
			tmp6[i][j] = result;
		}
	}
	
}

/* tmp1 = Pp * Ht */
static void step2_2(data_t Pp[Nsta][Nsta], data_t Ht[Nsta][Mobs], data_t tmp1[Nsta][Mobs])
{
	#pragma HLS inline off
	
	int i, j, l;
	
	for (i=0; i<Nsta; i++) {
		for (j=0; j<Mobs; j++) {
			#pragma HLS pipeline
			data_t result = 0;
			for (l=0; l<Nsta; l++) {
				data_t term = Pp[i][l] * Ht[l][j];
				result += term;
			}
			tmp1[i][j] = result;
		}
	}
	
}

/* tmp3 = tmp6 * Ht + R */
static void step2_3(data_t tmp6[Mobs][Nsta], data_t Ht[Nsta][Mobs], 
				data_t R[Mobs][Mobs], data_t tmp3[Mobs][Mobs])
{
	#pragma HLS inline off
	
	int i, j, l;
	
	for (i=0; i<Mobs; i++) {
		for (j=0; j<Mobs; j++) {
			#pragma HLS pipeline
			data_t result = 0;
			for (l=0; l<Nsta; l++) {
				data_t term = tmp6[i][l] * Ht[l][j];
				result += term;
			}
			tmp3[i][j] = result + R[i][j];
		}
	}
	
}

/* G = tmp1 * tmp4 */
static void step2_4(data_t tmp1[Nsta][Mobs], data_t tmp4[Mobs][Mobs], data_t G[Nsta][Mobs])
{

	#pragma HLS inline off
	
	int i, j, l;
	
	for (i=0; i<Nsta; i++) {
		for (j=0; j<Mobs; j++) {
			#pragma HLS pipeline 
			data_t result = 0;
			for (l=0; l<Mobs; l++) {
				data_t term = tmp1[i][l] * tmp4[l][j];
				result += term;
			}
			G[i][j] = result;
		}
	}
}





/* tmp0 = G * H */
static void step4_1(data_t G[Nsta][Mobs], data_t H[Mobs][Nsta], data_t tmp7[Nsta][Nsta])
{
	#pragma HLS inline off
	
	int i, j, l;
	
	for (i=0; i<Nsta; i++) {
		//#pragma HLS pipeline II=4 
		for (j=0; j<Nsta; j++) {
			#pragma HLS pipeline
			data_t result = 0;
			for (l=0; l<Mobs; l++) {
				data_t term = G[i][l] * H[l][j];
				result += term;
			}
			data_t imm0 = -result;
			if (i == j) {
				tmp7[i][j] = 1 + imm0;
			} else {
				tmp7[i][j] = imm0;
			}
		}
	}
}

/* P = tmp0 * Pp */
static void step4_2(data_t tmp7[Nsta][Nsta], data_t Pp[Nsta][Nsta], data_t P[Nsta][Nsta])
{
	#pragma HLS inline off
	
	int i, j, l;
	
	for (i=0; i<Nsta; i++) {
		for (j=0; j<Nsta; j++) {
			#pragma HLS pipeline
			data_t result = 0;
			for (l=0; l<Nsta; l++) {
				data_t term = tmp7[i][l] * Pp[l][j];
				result += term;
			}
			P[i][j] = result;
		}
	}
}






/* P_k = F_{k-1} P_{k-1} F^T_{k-1} + Q_{k-1} */
static void step1(data_t F[Nsta][Nsta], data_t P[Nsta][Nsta], 
				data_t Q[Nsta][Nsta], data_t Pp[Nsta][Nsta],
				data_t Ft[Nsta][Nsta], data_t tmp0[Nsta][Nsta])
{
	#pragma HLS inline off
	#pragma HLS inline region
	
	#pragma HLS array_partition variable=F block factor=2 dim=2
	#pragma HLS array_partition variable=P block factor=2 dim=2

	/*  Note: In this example, F is a constant binary matrix. Therefore, we
		can implement the matrix multiplication using adds only */

	/* tmp0 = F * P */
	step1_1(F, P, tmp0, Ft);
    
	/* Pp = tmp0 * F^T + Q */
	step1_2(tmp0, Ft, Q, Pp);

}

/* G_k = P_k H^T_k (H_k P_k H^T_k + R)^{-1} */
static void step2(data_t H[Mobs][Nsta], data_t Pp[Nsta][Nsta], 
					data_t R[Mobs][Mobs], data_t G[Nsta][Mobs],
					data_t Ht[Nsta][Mobs], data_t tmp1[Nsta][Mobs],
					data_t tmp3[Mobs][Mobs], data_t tmp4[Mobs][Mobs],
					data_t tmp6[Mobs][Nsta], data_t tmp5[Mobs])
{
	
	#pragma HLS inline off
	#pragma HLS inline region
	
	/* tmp6 = H * Pp */
	step2_1(H, Pp, Ht, tmp6);
	
	/* tmp1 = Pp * Ht */
	step2_2(Pp, Ht, tmp1);
	
	/* tmp3 = tmp6 * Ht + R */
	step2_3(tmp6, Ht, R, tmp3);
	
	/* tmp4 = (tmp3)^-1 */
    cholsl(tmp3, tmp4, tmp5);
	//LUInv(tmp3, tmp4);
	
	/* G = tmp1 * tmp4 */
    step2_4(tmp1, tmp4, G);	
}

/* \hat{x}_k = \hat{x_k} + G_k(z_k - h(\hat{x}_k)) */
static void step3(data_t SV_Rho[4], data_t hx[Mobs], data_t fx[Nsta], 
					data_t x[Nsta], data_t G[Nsta][Mobs], 
					data_t tmp2[Nsta], data_t tmp5[Mobs])
{
	#pragma HLS inline off
	#pragma HLS inline region
	
	int i, j;
	
	// err = SV_Rho - hx
	for (j=0; j<Mobs; j++) {
		tmp5[j] = SV_Rho[j] - hx[j];
	}
	
	// x = G*err + fx
	for (i=0; i<Nsta; i++) {
		data_t result = 0;
		for (j=0; j<Mobs; j++) {
			data_t term = G[i][j] * tmp5[j];
			result += term;
		}
		x[i] = fx[i] + result;		
	}
	
}


/* P_k = (I - G_k H_k) P_k */
static void step4(data_t H[Mobs][Nsta], data_t G[Nsta][Mobs], data_t Pp[Nsta][Nsta], 
					data_t P[Nsta][Nsta], data_t tmp7[Nsta][Nsta])
{
	#pragma HLS inline off
	#pragma HLS inline region	
	
	/* tmp0 = (I - G*H) */
	step4_1(G, H, tmp7);
	
	/* P = tmp0 * Pp */
	step4_2(tmp7, Pp, P);
}




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
			)
{        

	//#pragma HLS PIPELINE II=512
	#pragma HLS inline off
	
    /* P_k = F_{k-1} P_{k-1} F^T_{k-1} + Q_{k-1} */
    step1(F, P, Q, Pp, Ft, tmp0);
	
    /* G_k = P_k H^T_k (H_k P_k H^T_k + R)^{-1} */
	step2(H, Pp, R, G, Ht, tmp1, tmp3, tmp4, tmp6, tmp5);
	
    /* \hat{x}_k = \hat{x_k} + G_k(z_k - h(\hat{x}_k)) */
    step3(SV_Rho, hx, fx, x, G, tmp2, tmp5);
	
    /* P_k = (I - G_k H_k) P_k */
	step4(H, G, Pp, P, tmp7);

}
