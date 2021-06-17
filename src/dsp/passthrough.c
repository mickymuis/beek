#include "passthrough.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    dsp_worker_t worker;
} dsp_passthrough_t;

void
destroy( dsp_passthrough_t* w ) {
    // Nothing to do...
}

void
consume( dsp_passthrough_t* w ) {

}

dsp_worker_t*
dsp_createPassThrough( int channels ) {
    dsp_passthrough_t* w =malloc( sizeof(dsp_passthrough_t) );

    memset( w, 0, sizeof(dsp_passthrough_t) );

    w->worker.destroy =(dsp_destroy_func_t)destroy;
    w->worker.consume =(dsp_consume_func_t)consume;

    return (dsp_worker_t*)w;
}

