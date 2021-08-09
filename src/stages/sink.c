#include "sink.h"
#include "../stage_private.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct {
    flw_stage_t  stage;
    scope_t *    scope;
    unsigned int channel;
} flw_scopesink_t;

static void
destroy( flw_stage_t* stage ) {
    // Nothing to do...
}

static void
consume( flw_stage_t* stage ) {
    flw_scopesink_t* ss =(flw_scopesink_t*)stage;

    if( atomq_estimateLength( stage->inq ) < stage->inPortSize )
        return;

    int chan =ss->channel;
    int n    =scope_channelSampleSize( ss->scope, chan );

    // Distribute the samples over multiple channels if needed
    for( int i=0; i < stage->inPortSize; i++ ) {
        double in;
        atomq_dequeue( stage->inq, &in );

        if( n == 0 ) {
            chan++;
            n =scope_channelSampleSize( ss->scope, chan );
        }
        scope_pushChannelFifo( ss->scope, chan, in );
        n--;
    }
}

flw_stage_t*
flw_createScopeSink( scope_t* scope, unsigned int channel, int port_size ) {
    flw_scopesink_t* ss =malloc( sizeof(flw_scopesink_t) );
    assert( scope && port_size > 0 );

    flw_initStage( (flw_stage_t*)ss );

    ss->stage.destroy     = (flw_destroy_func_t)destroy;
    ss->stage.consume     = (flw_consume_func_t)consume;
    ss->stage.outPortSize = 0;
    ss->stage.inPortSize  = port_size;
    ss->stage.name        = "scope";
    ss->stage.runAsThread = false;
    ss->stage.synchronize = false;

    // Move
    ss->stage.inq         = atomq_create( FLW_DEFAULT_FIFO_DEPTH * port_size );

    ss->scope             = scope;
    ss->channel           = channel;

    return (flw_stage_t*)ss;
}

