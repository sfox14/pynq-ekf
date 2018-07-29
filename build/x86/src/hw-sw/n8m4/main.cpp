
/*  This code is adapted from "TinyEKF: Extended Kalman Filter
	for embedded processors", https://github.com/simondlevy/TinyEKF
	
	gps_ekf: TinyEKF test case using You Chong's GPS example:
  
		http://www.mathworks.com/matlabcentral/fileexchange/31487-extended-kalman-filter-ekf--for-gps
 
	Reads file gps.csv of satellite data and writes file ekf.csv of mean-subtracted estimated positions.
	
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <strings.h>
#include <time.h>
#include <math.h>
#include "sds_lib.h"

#include "ekf_config.h"

#define SEC_TO_NS (1000000000)


static void readline(char * line, FILE * fp)
{
    fgets(line, 1000, fp);
}

static void skipline(FILE * fp)
{
    char line[1000];
    readline(line, fp);
}

static float toFloat(int32_t a)
{
	float result;
	float val = 1 << frac_width;
	result = a/val;
	
	return result;
}


static int32_t toFixed(float a)
{
	int32_t result;
	int32_t val = 1 << frac_width; //logical shift
	
	//if (a<0) result = a*val-0.5; //rounding
	//else result = a*val +0.5;
	result = a*val;
	
	return result;
}

static void readdata(port_t *obs, float *meas, const char fname[], int datalen)
{
    FILE * fp = fopen(fname, "r");

    // Skip CSV header
    skipline(fp);

    for (int i=0; i<datalen; i++) {

        char line[1000];

        readline(line, fp);

        char * p = strtok(line, ",");

        // read transpose, i.e. [Nxyz][Nsats] instead of [Nsats][Nxyz] like csv
        for (int j=0; j<12; j++) {
			float imm = atof(p);
			p = strtok(NULL, ",");
            meas[i*12 + j] = imm; 
        }
		
		for (int j=0; j<Mobs; j++) {
			float imm = atof(p);
			int32_t imm_fx = toFixed(imm);
			p = strtok(NULL, ",");
            obs[i*Mobs+ j] = imm_fx; 
		}

    }

    fclose(fp);
}

static void writedata(port_t *output, float *output_fl, int datalen)
{
	int i,j;

	// convert outputs to float
	for (i=0; i<datalen*Nsta; i++) {
		uint32_t oval_uint = output[i];
		int32_t oval_fx = (int32_t) oval_uint;
		output_fl[i] = toFloat(oval_fx);
	}

    // write filtered positions measus means
    for (i=0; i<datalen; i++) {
        printf("[%d] %f %f %f\n", i, output_fl[i*Nsta + 0], output_fl[i*Nsta + 2], output_fl[i*Nsta + 4]);
    }
    printf("Finished EKF\n");

}

static void append_result(port_t *xout, port_t *output, int i)
{	
	for (int j=0; j<Nsta; j++) {
		output[i*Nsta + j] = xout[j];
	}
}

static void model( float x[Nsta], float meas[12], port_t *fx_i, port_t *hx_i, 
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

int main(int argc, char ** argv)
{    
    // input data file
    static const char INFILE[] = "gps_data.csv";
	
    // Make a place to store the data from the file and the output of the EKF
    int datalen = 50;
	int PARAMS_IN = Nsta+(2*Nsta*Nsta)+(Mobs*Mobs);
	
	// hardware I/O
	port_t *obs, *xout, *params;
	port_t *fx_i, *hx_i, *F_i, *H_i;
	// software I/O
	float *meas, *xout_fl, *output_fl; 
	
	// allocate memory 
	obs= (port_t *)sds_alloc_non_cacheable(datalen*Mobs*sizeof(port_t));
	meas= (float *)malloc(datalen*12*sizeof(float));
	params = (port_t *)sds_alloc_non_cacheable(PARAMS_IN*sizeof(port_t));
    xout = (port_t *)sds_alloc_non_cacheable(datalen*Nsta*sizeof(port_t));
	xout_fl = (float *)malloc(Nsta*sizeof(float));
	output_fl = (float *)malloc(datalen*Nsta*sizeof(float));
	
	fx_i = (port_t *)sds_alloc_non_cacheable(Nsta*sizeof(port_t));
	hx_i = (port_t *)sds_alloc_non_cacheable(Mobs*sizeof(port_t));
	F_i = (port_t *)sds_alloc_non_cacheable(Nsta*Nsta*sizeof(port_t));
	H_i = (port_t *)sds_alloc_non_cacheable(Mobs*Nsta*sizeof(port_t));
	
	// set params
	xout_fl[0] = 0.2574;
	xout_fl[1] = 0.3;
	xout_fl[2] = -0.908482;
	xout_fl[3] = -0.1;
	xout_fl[4] = -0.378503;
	xout_fl[5] = 0.3;
	xout_fl[6] = 0.02;
	xout_fl[7] = 0.0;
	
	/* params[Nsta+(2*Nsta*Nsta)+(Mobs*Mobs)]:
		1 - x[Nsta]
		2 - P[Nsta*Nsta]
		3 - Q[Nsta*Nsta]
		4 - R[Mobs*Mobs]
	*/
	#include "params.dat"
	
	for (int i=0; i<Nsta; i++) {
		fx_i[i] = 0;
	}
	for (int i=0; i<Mobs; i++) {
		hx_i[i] = 0;
	}
	for (int i=0; i<(Mobs*Nsta); i++) {
		H_i[i] = 0;
	}
	
	
	// read csv data into obs
	readdata(obs, meas, INFILE, datalen);
	
	// control registers
	int ctrl;
	int w1 = Nsta;
	int w2 = Mobs;

	struct timespec * start = (struct timespec *)malloc(sizeof(struct timespec));
	struct timespec * stop = (struct timespec *)malloc(sizeof(struct timespec));
	clock_gettime(CLOCK_REALTIME, start);
	
	// init
	ctrl=0;
	model(xout_fl, &meas[0*12], fx_i, hx_i, F_i, H_i);
	top_ekf(&obs[0*Mobs], fx_i, hx_i, F_i, H_i, params, &xout[0*Nsta], ctrl, w1, w2);
	
	// copy result from fixed to float
	for (int j=0; j<Nsta; j++) {		
		uint32_t oval_uint = xout[0*Nsta + j];
		int32_t oval_fx = (int32_t) oval_uint;
		xout_fl[j] = toFloat(oval_fx);
	}

	// run ekf
	ctrl = 1; 
	for (int i=1; i<datalen; i++) {	
		// compute model
		model(xout_fl, &meas[i*12], fx_i, hx_i, F_i, H_i);
		// step ekf
		top_ekf(&obs[i*Mobs], fx_i, hx_i, F_i, H_i, params, &xout[i*Nsta], ctrl, w1, w2);
		// copy result from fixed to float
		for (int j=0; j<Nsta; j++) {		
			uint32_t oval_uint = xout[i*Nsta + j];
			int32_t oval_fx = (int32_t) oval_uint;
			xout_fl[j] = toFloat(oval_fx);
		}
	}
	clock_gettime(CLOCK_REALTIME, stop);
	
	writedata(xout, output_fl, datalen);
	
	int totalTime = (stop->tv_sec*SEC_TO_NS + stop->tv_nsec) - (start->tv_sec*SEC_TO_NS + start->tv_nsec);
	printf("time = %f s\n", ((float)totalTime/1000000000));


	sds_free(obs);
	free(meas);
	sds_free(params);
	sds_free(xout);
	free(xout_fl);
	free(output_fl);
	sds_free(fx_i);
	sds_free(hx_i);
	sds_free(F_i);
	sds_free(H_i);
	
    // Done!
    return 0;
}
