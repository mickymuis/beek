#include "passthrough.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    dsp_worker_t worker;
} dsp_passthrough_t;

static void
destroy( dsp_worker_t* worker ) {
    // Nothing to do...
}

static void
consume( dsp_worker_t* worker ) {
    worker->out[0] =worker->in[0];
}

dsp_worker_t*
dsp_createPassThrough( int channels ) {
    dsp_passthrough_t* dspw =malloc( sizeof(dsp_passthrough_t) );

    memset( dspw, 0, sizeof(dsp_passthrough_t) );

    dspw->worker.destroy =(dsp_destroy_func_t)destroy;
    dspw->worker.consume =(dsp_consume_func_t)consume;
    dspw->worker.inputs  =dspw->worker.outputs = channels;

    return (dsp_worker_t*)dspw;
}

