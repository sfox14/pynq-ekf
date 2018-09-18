
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


int main(int argc, char ** argv)
{    

    int datalen = 50;
    int PARAMS_IN = (2*Nsta*Nsta)+(Mobs*Mobs);
    
    // hardware I/O
    port_t *obs, *xout, *params;
    port_t *fx_i, *hx_i, *F_i, *H_i;

    float *xout_fl;
    
#if P_CACHEABLE == 0
    obs= (port_t *)sds_alloc_non_cacheable(datalen*Mobs*sizeof(port_t));
    params = (port_t *)sds_alloc_non_cacheable(PARAMS_IN*sizeof(port_t));
    xout = (port_t *)sds_alloc_non_cacheable(datalen*Nsta*sizeof(port_t));
    fx_i = (port_t *)sds_alloc_non_cacheable(Nsta*sizeof(port_t));
    hx_i = (port_t *)sds_alloc_non_cacheable(Mobs*sizeof(port_t));
    F_i = (port_t *)sds_alloc_non_cacheable(Nsta*Nsta*sizeof(port_t));
    H_i = (port_t *)sds_alloc_non_cacheable(Mobs*Nsta*sizeof(port_t));
#else
    obs= (port_t *)sds_alloc(datalen*Mobs*sizeof(port_t));
    params = (port_t *)sds_alloc(PARAMS_IN*sizeof(port_t));
    xout = (port_t *)sds_alloc(datalen*Nsta*sizeof(port_t));
    fx_i = (port_t *)sds_alloc(Nsta*sizeof(port_t));
    hx_i = (port_t *)sds_alloc(Mobs*sizeof(port_t));
    F_i = (port_t *)sds_alloc(Nsta*Nsta*sizeof(port_t));
    H_i = (port_t *)sds_alloc(Mobs*Nsta*sizeof(port_t));
#endif

    xout_fl = (float *)malloc(Nsta*sizeof(float));
    
    int ctrl;
    int w1 = Nsta;
    int w2 = Mobs;
    
    struct timespec * start = (struct timespec *)malloc(sizeof(struct timespec));
    struct timespec * stop = (struct timespec *)malloc(sizeof(struct timespec));
    clock_gettime(CLOCK_REALTIME, start);
    
    //init
    ctrl=0;
    //model()
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
        //model()
        // step ekf
        top_ekf(&obs[1*Mobs], fx_i, hx_i, F_i, H_i, params, &xout[1*Nsta], ctrl, w1, w2);
        // copy result from fixed to float
        for (int j=0; j<Nsta; j++) {        
            uint32_t oval_uint = xout[i*Nsta + j];
            int32_t oval_fx = (int32_t) oval_uint;
            xout_fl[j] = toFloat(oval_fx);
        }
    }
    clock_gettime(CLOCK_REALTIME, stop);
    int totalTime = (stop->tv_sec*SEC_TO_NS + stop->tv_nsec) - (start->tv_sec*SEC_TO_NS + start->tv_nsec);
    printf("time = %f s\n", ((float)totalTime/1000000000));

    sds_free(obs);
    sds_free(params);
    sds_free(xout);
    sds_free(fx_i);
    sds_free(hx_i);
    sds_free(F_i);
    sds_free(H_i);
    free(xout_fl);
    
    // Done!
    return 0;
}
