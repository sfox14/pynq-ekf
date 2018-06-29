
/*  Application dependent model	
*/

#include <math.h>
#include "ekf_config.h"

void model( float x[Nsta], float meas[12], port_t *fx_i, port_t *hx_i, 
			port_t *F_i, port_t *H_i )
{
	float fx[Nsta];
	float hx[Mobs];
	float F[Nsta][Nsta];
	float H[Mobs][Nsta] = {{0}};
	
    float imm;

	// compute f(x)
    for (int j=0; j<8; j+=2) {
        fx[j] = x[j] + x[j+1];
        fx[j+1] = x[j+1];
    }

	// compute new F Jacobian
	// F is constant at init 

	// compute h(x)
    float dx[4][3];
    for (int i=0; i<4; ++i) {
        hx[i] = 0;
        for (int j=0; j<3; ++j) {
            float d = fx[j*2] - meas[i*3 + j];
            dx[i][j] = d;
            hx[i] += d*d;
        }
        imm = hx[i];
        hx[i] = sqrt(imm);
		hx[i] = hx[i] + fx[6];
    }

	// compute new H Jacobian
    for (int i=0; i<4; ++i) {
        for (int j=0; j<3; ++j) {
        	H[i][j*2] = dx[i][j]/hx[i];
    	}
        H[i][6] = 1.0;
    }
	
	// copy to fixed point 
	for (int i=0; i<Nsta; i++) {
		int32_t imm_fx = toFixed(fx[i]);
		fx_i[i] = imm_fx;
	}
	
	for (int i=0; i<Mobs; i++) {
		int32_t imm_fx = toFixed(hx[i]);
		hx_i[i] = imm_fx;
	}
	
	for (int i=0; i<Mobs; i++) {
		for (int j=0; j<Nsta; j++) {
			int32_t imm_fx = toFixed(H[i][j]);
			H_i[i*Nsta + j] = imm_fx;
		}
	}
}
