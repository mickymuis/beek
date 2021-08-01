#include "passthrough.h"

#include "../stage_private.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    flw_stage_t stage;
} flw_passthrough_t;

static void
destroy( flw_stage_t* stage ) {
    // Nothing to do...
}

static void
consume( flw_stage_t* stage ) {
    //flw_passthrough_t* lp =(flw_passthrough_t*)stage;

    if( atomq_estimateLength( stage->inq ) < stage->inPortSize )
        return;

    if( atomq_capacity( stage->outq ) - atomq_estimateLength( stage->outq ) < stage->outPortSize )
        return;

    for( int i=0; i < stage->inPortSize; i++ ) {
        double in;
        atomq_dequeue( stage->inq, &in );
        atomq_enqueue( stage->outq, in );
    }
}

flw_stage_t*
flw_createPassThrough( int port_size ) {
    flw_passthrough_t* p =malloc( sizeof(flw_passthrough_t) );

    memset( p, 0, sizeof(flw_passthrough_t) );

    p->stage.destroy     =(flw_destroy_func_t)destroy;
    p->stage.consume     =(flw_consume_func_t)consume;
    p->stage.inPortSize  =p->stage.outPortSize = port_size;
    p->stage.name        = "passthrough";
    p->stage.runAsThread = true;

    // Move
    p->stage.inq = atomq_create( FLW_DEFAULT_FIFO_DEPTH * port_size );


    return (flw_stage_t*)p;
}
