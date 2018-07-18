
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

static void readdata(port_t *xin, const char fname[], int datalen)
{
    FILE * fp = fopen(fname, "r");

    // Skip CSV header
    skipline(fp);

    for (int i=0; i<datalen; i++) {

        char line[1000];

        readline(line, fp);

        char * p = strtok(line, ",");

        // read transpose, i.e. [Nxyz][Nsats] instead of [Nsats][Nxyz] like csv
        for (int j=0; j<(Nsats*(Nxyz+1)); j++) {
			float imm = atof(p);
			int32_t imm_fx = toFixed(imm);
			uint32_t imm_uint = (uint32_t) imm_fx;
			p = strtok(NULL, ",");
            xin[i*(Nsats*(Nxyz+1)) + j] = imm_uint;
        }
    }

    fclose(fp);
}

static void writedata(port_t *output, float *output_fl, const char fname[], int datalen)
{
    FILE * fp = fopen(fname, "w");
    fprintf(fp, "X,Y,Z\n");

	int i,j;

	// convert outputs to float
	for (i=0; i<datalen*3; i++) {
		uint32_t oval_uint = output[i];
		int32_t oval_fx = (int32_t) oval_uint;
		output_fl[i] = toFloat(oval_fx);
	}
		
    // Compute means of filtered positions
    double mean_Pos_KF[3] = {0, 0, 0};

    for (i=0; i<datalen; i++)
        for (j=0; j<3; j++)
            mean_Pos_KF[j] += output_fl[i*3 + j];
    for (j=0; j<3; j++)
        mean_Pos_KF[j] /= datalen;

    // write filtered positions minus means
    for (i=0; i<datalen; i++) {
        fprintf(fp, "%f,%f,%f\n", output_fl[i*3]-mean_Pos_KF[0], output_fl[i*3 +1]-mean_Pos_KF[1],
                      output_fl[i*3 +2]-mean_Pos_KF[2]);
        printf("%f %f %f\n", output_fl[i*3], output_fl[i*3 +1], output_fl[i*3 +2]);
    }
    printf("Wrote file %s\n", fname);

    fclose(fp);
}

float toFloat(int32_t a)
{
	float result;
	float val = 1 << frac_width;
	result = a/val;
	
	return result;
}


int32_t toFixed(float a)
{
	int32_t result;
	int32_t val = 1 << frac_width; //logical shift
	
	if (a<0) result = a*val-0.5; //rounding
	else result = a*val +0.5;
	
	return result;
}


int main(int argc, char ** argv)
{    
    // input data file
    static const char INFILE[] = "gps_data.csv";
    static const char OUTFILE[] = "ekf.csv";
	
    // Make a place to store the data from the file and the output of the EKF
    int datalen = 50;
	int PARAMS_IN = 182; //(((2*Nsta)+2+Mobs)*Nsta)+Mobs+2;
    port_t *xin, *output, *params, *pout;
	float *output_fl;
	output_fl = (float *)malloc(datalen*Nxyz*sizeof(float));
	xin = (port_t *)sds_alloc(datalen*Nsats*(Nxyz+1)*sizeof(port_t));
    params = (port_t *)sds_alloc(182*sizeof(port_t));
    output = (port_t *)sds_alloc(datalen*Nxyz*sizeof(port_t));
	pout = (port_t *)sds_alloc((Nsta*Nsta)*sizeof(port_t));

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
	#include "params.dat"
	
	struct timespec * start = (struct timespec *)malloc(sizeof(struct timespec));
    struct timespec * stop = (struct timespec *)malloc(sizeof(struct timespec));

	for (int i=0; i<2; i++) {
	
		// write csv data to xin
		readdata(xin, INFILE, datalen);

	    clock_gettime(CLOCK_REALTIME, start);
		top_ekf(xin, params, output, pout, datalen);
		clock_gettime(CLOCK_REALTIME, stop);

		writedata(output, output_fl, OUTFILE, datalen);
	}
	int totalTime = (stop->tv_sec*SEC_TO_NS + stop->tv_nsec) - (start->tv_sec*SEC_TO_NS + start->tv_nsec);
    printf("time = %f s\n", ((float)totalTime/1000000000));
	
	sds_free(xin);
    sds_free(params);
    sds_free(output);
	sds_free(pout);
	free(output_fl);

    // Done!
    return 0;
}
