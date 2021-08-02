#include "lowpass.h"

#include "../stage_private.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    flw_stage_t stage;
    double*     avg;
} flw_lowpass_t;

static void
destroy( flw_stage_t* stage ) {
    flw_lowpass_t* lp =(flw_lowpass_t*)stage;
    free( lp->avg );
}

static void
consume( flw_stage_t* stage ) {
    flw_lowpass_t* lp =(flw_lowpass_t*)stage;

    if( atomq_estimateLength( stage->inq ) < stage->inPortSize )
        return;
    
    if( atomq_capacity( stage->outq ) - atomq_estimateLength( stage->outq ) < stage->outPortSize )
        return;

    for( int i=0; i < stage->inPortSize; i++ ) {
        double in;
        atomq_dequeue( stage->inq, &in );
        lp->avg[i] =lp->avg[i] * 0.95 + in * 0.05;
        atomq_enqueue( stage->outq, lp->avg[i] );
    }
}

flw_stage_t*
flw_createLowPass( int port_size ) {
    flw_lowpass_t* l =malloc( sizeof(flw_lowpass_t) );

    flw_initStage( (flw_stage_t*)l );

    l->avg =malloc( port_size * sizeof(double) );

    l->stage.destroy     =(flw_destroy_func_t)destroy;
    l->stage.consume     =(flw_consume_func_t)consume;
    l->stage.inPortSize  =l->stage.outPortSize = port_size;
    l->stage.name        = "lowpass";
    l->stage.runAsThread = true;
    
    // Move
    l->stage.inq = atomq_create( FLW_DEFAULT_FIFO_DEPTH * port_size );

    return (flw_stage_t*)l;
}


