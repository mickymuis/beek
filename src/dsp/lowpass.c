#include "lowpass.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    dsp_worker_t worker;
    double avg;
} dsp_lowpass_t;

static void
destroy( dsp_worker_t* worker ) {
    // Nothing to do...
}

static void
consume( dsp_worker_t* worker ) {
    dsp_lowpass_t* lp =(dsp_lowpass_t*)worker;

    lp->avg =lp->avg * 0.99 + worker->in[0] * 0.01;
    worker->out[0] =lp->avg;
}

dsp_worker_t*
dsp_createLowPass( int channels ) {
    dsp_lowpass_t* dspw =malloc( sizeof(dsp_lowpass_t) );

    memset( dspw, 0, sizeof(dsp_lowpass_t) );

    dspw->worker.destroy =(dsp_destroy_func_t)destroy;
    dspw->worker.consume =(dsp_consume_func_t)consume;
    dspw->worker.inputs  =dspw->worker.outputs = channels;

    return (dsp_worker_t*)dspw;
}


