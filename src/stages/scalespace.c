#include "scalespace.h"

#include "../stage_private.h"
#include "../gaussian.h"
#include "../cfifo.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    flw_stage_t  stage;
    gaussian_t** kernels;
    cfifo_t**    buffers;
    // Number of scales
    int          scales;
    // Size of the largest kernel
    size_t       maxKSize;
} flw_sst_t;

static void
destroy( flw_stage_t* stage ) {
    flw_sst_t* p =(flw_sst_t*)stage;
    for( int i=0; i < p->scales; i++ ) {
        gaussian_destroy( p->kernels[i] );
        cfifo_destroy( p->buffers[i] );
    }
    free( p->kernels ); free( p->buffers );
}

/** Super naive convolution that assumes |signal| = |kernel|.
    Returns only the single overlap element in the center */
static double
convolveCenter( cfifo_t* signal, gaussian_t* kernel ) {
    size_t s          =gaussian_kernelSize( kernel );
    double d          =0.;
    const double* ptr =gaussian_dataPtr( kernel );
    for( size_t i=0; i < s; i++ ) {
        // We don't care about flipping the kernel because it is assumed to be symmetrical
        d += ptr[i] * cfifo_read( signal, i );
    }
    return d;
}

static void
consume( flw_stage_t* stage ) {
    flw_sst_t* p =(flw_sst_t*)stage;

    if( atomq_estimateLength( stage->inq ) < stage->inPortSize )
        return;

    if( atomq_capacity( stage->outq ) - atomq_estimateLength( stage->outq ) < stage->outPortSize )
        return;
    
    double in;
    atomq_dequeue( stage->inq, &in );

    double scale[p->scales];

    for( int i=0; i < p->scales; i++ ) {
        cfifo_push2( p->buffers[i], in );
        // Check if the buffer is filled to capacity
        if( cfifo_full( p->buffers[i] ) ) {
            scale[i] = convolveCenter( p->buffers[i], p->kernels[i] );
        } else
        // Pad the output with zeroes?
            scale[i] = 0.;
    }

    for( int i=0; i < p->scales; i++ ) { 
        if( i == p->scales-1 )
            atomq_enqueue( stage->outq, scale[i] );
        else
            atomq_enqueue( stage->outq, scale[i]-scale[i+1] );
    }
}

flw_stage_t*
flw_createScaleSpaceTransform( int n_scales ) {
    flw_sst_t* p         =malloc( sizeof(flw_sst_t) );

    flw_initStage( (flw_stage_t*)p );

    p->stage.destroy     =(flw_destroy_func_t)destroy;
    p->stage.consume     =(flw_consume_func_t)consume;
    p->stage.inPortSize  = 1;
    p->stage.outPortSize = n_scales;
    p->stage.name        = "scalespace transform";
    p->stage.runAsThread = true;

    p->scales            = n_scales;
    p->kernels           = malloc( n_scales * sizeof(gaussian_t*) );
    p->buffers           = malloc( n_scales * sizeof(cfifo_t*) );

    const double cutoff =3.0; // parameter
    int sigma =1;

    // Create the kernels
    for( int i=0; i < n_scales; i++ ) {
        p->kernels[i]    = gaussian_create( (double)sigma, cutoff );
        p->buffers[i]    = cfifo_create( gaussian_kernelSize( p->kernels[i] ) );

        sigma            = sigma << 1;
    }

    p->maxKSize          = gaussian_kernelSize( p->kernels[n_scales-1] );
    
    // Move
    size_t depth = FLW_DEFAULT_FIFO_DEPTH;
    if( depth < p->maxKSize )
        depth = p->maxKSize;
    p->stage.inq         = atomq_create( depth );

    return (flw_stage_t*)p;
}
