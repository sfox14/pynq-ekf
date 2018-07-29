/* gps_ekf: TinyEKF test case using You Chong's GPS example:
 * 
 *   http://www.mathworks.com/matlabcentral/fileexchange/31487-extended-kalman-filter-ekf--for-gps
 * 
 * Reads file gps.csv of satellite data and writes file ekf.csv of mean-subtracted estimated positions.
 *
 *
 * References:
 *
 * 1. R G Brown, P Y C Hwang, "Introduction to random signals and applied 
 * Kalman filtering : with MATLAB exercises and solutions",1996
 *
 * 2. Pratap Misra, Per Enge, "Global Positioning System Signals, 
 * Measurements, and Performance(Second Edition)",2006
 * 
 * Copyright (C) 2015 Simon D. Levy
 *
 * MIT License
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>
#include <time.h>

#include "tinyekf_config.h"
//#include "tiny_ekf.h"

#define SEC_TO_NS (1000000000)

// positioning interval
//static const data_t T = 1;


static void readline(char * line, FILE * fp)
{
    fgets(line, 1000, fp);
}

static void skipline(FILE * fp)
{
    char line[1000];
    readline(line, fp);
}

static void readdata(data_t *xin, const char fname[], int datalen)
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
            xin[i*(Nsats*(Nxyz+1)) + j] = atof(p);
            p = strtok(NULL, ",");
        }
    }

    fclose(fp);
}

static void writedata(data_t *output, const char fname[], int datalen)
{
    FILE * fp = fopen(fname, "w");
    fprintf(fp, "X,Y,Z\n");


    // Compute means of filtered positions
    data_t mean_Pos_KF[3] = {0, 0, 0};

    int i,j;
    for (i=0; i<datalen; i++)
        for (j=0; j<3; j++)
            mean_Pos_KF[j] += output[i*3 + j];
    for (j=0; j<3; j++)
        mean_Pos_KF[j] /= datalen;

    // write filtered positions minus means
    for (i=0; i<datalen; i++) {
        fprintf(fp, "%f,%f,%f\n", output[i*3]-mean_Pos_KF[0], output[i*3 +1]-mean_Pos_KF[1],
                      output[i*3 +2]-mean_Pos_KF[2]);
        printf("%f %f %f\n", output[i*3], output[i*3 +1], output[i*3 +2]);
    }
    printf("Wrote file %s\n", fname);

    fclose(fp);
}


int main(int argc, char ** argv)
{    
    // input data file
    static const char INFILE[] = "gps_data.csv";
    static const char OUTFILE[] = "ekf.csv";

    // Make a place to store the data from the file and the output of the EKF
    int datalen = 50;
    data_t *xin, *output;
    xin = (data_t *)malloc(datalen*Nsats*(Nxyz+1)*sizeof(data_t));
    output = (data_t *)malloc(datalen*Nxyz*sizeof(data_t));


    struct timespec * start = (struct timespec *)malloc(sizeof(struct timespec));
    struct timespec * stop = (struct timespec *)malloc(sizeof(struct timespec));
	
    // write csv data to xin
    readdata(xin, INFILE, datalen);

	clock_gettime(CLOCK_REALTIME, start);
    gps_ekf(xin, output, datalen);
	clock_gettime(CLOCK_REALTIME, stop);

	writedata(output, OUTFILE, datalen);
	
	int totalTime = (stop->tv_sec*SEC_TO_NS + stop->tv_nsec) - (start->tv_sec*SEC_TO_NS + start->tv_nsec);
    printf("time = %f s\n", ((data_t)totalTime/1000000000));
	
    free(xin);
    free(output);

    // Done!
    return 0;
}
