
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
	
	if (a<0) result = a*val-0.5; //rounding
	else result = a*val +0.5;
	
	return result;
}


int main(int argc, char ** argv)
{    

    int datalen = 50;
	int PARAMS_IN = Nsta+(2*Nsta*Nsta)+(Mobs*Mobs);
	
	// hardware I/O
	port_t *obs, *xout, *params;
	port_t *fx_i, *hx_i, *F_i, *H_i;

	// allocate memory 
	obs= (port_t *)sds_alloc_non_cacheable(datalen*Mobs*sizeof(port_t));
	params = (port_t *)sds_alloc_non_cacheable(PARAMS_IN*sizeof(port_t));
    	xout = (port_t *)sds_alloc_non_cacheable(datalen*Nsta*sizeof(port_t));
	fx_i = (port_t *)sds_alloc_non_cacheable(Nsta*sizeof(port_t));
	hx_i = (port_t *)sds_alloc_non_cacheable(Mobs*sizeof(port_t));
	F_i = (port_t *)sds_alloc_non_cacheable(Nsta*Nsta*sizeof(port_t));
	H_i = (port_t *)sds_alloc_non_cacheable(Mobs*Nsta*sizeof(port_t));
	
	int ctrl, w1, w2;
	
	// init
	ctrl=0;
	w1=2;
	w2=2;
	top_ekf(&obs[0*Mobs], fx_i, hx_i, F_i, H_i, params, &xout[0*Nsta], ctrl, w1, w2);
	
	ctrl=1;
	top_ekf(&obs[1*Mobs], fx_i, hx_i, F_i, H_i, params, &xout[1*Nsta], ctrl, w1, w2);

	ctrl=2;
	top_ekf(&obs[2*Mobs], fx_i, hx_i, F_i, H_i, params, &xout[2*Nsta], ctrl, w1, w2);

	sds_free(obs);
	sds_free(params);
	sds_free(xout);
	sds_free(fx_i);
	sds_free(hx_i);
	sds_free(F_i);
	sds_free(H_i);
	
    // Done!
    return 0;
}
