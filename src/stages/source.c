#include "source.h"

#include "../stage_private.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct {
    flw_stage_t stage;
    sampler_t *sampler;
} flw_samplesource_t;

static void
destroy( flw_stage_t* stage ) {
    // Nothing to do...
}

static void
consume( flw_stage_t* stage ) {
    flw_samplesource_t* ss =(flw_samplesource_t*)stage;

    if( atomq_capacity( stage->outq ) - atomq_estimateLength( stage->outq ) < stage->outPortSize )
        return;

    for( int i=0; i < stage->outPortSize; i++ ) {
        double in = sampler_getNext( ss->sampler );
        atomq_enqueue( stage->outq, in );
    }
}

flw_stage_t*
flw_createSampleSource( sampler_t* sampler, int port_size ) {
    flw_samplesource_t* ss =malloc( sizeof(flw_samplesource_t) );
    assert( sampler && port_size > 0 );

    flw_initStage( (flw_stage_t*)ss );

    ss->stage.destroy     = (flw_destroy_func_t)destroy;
    ss->stage.consume     = (flw_consume_func_t)consume;
    ss->stage.inPortSize  = 0;
    ss->stage.outPortSize = port_size;
    ss->stage.name        = "sample_source";
    ss->stage.runAsThread = false;
    ss->stage.synchronize = true;

    ss->sampler =sampler;

    return (flw_stage_t*)ss;
}

