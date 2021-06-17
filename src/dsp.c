#include "dsp.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

dsp_t*
dsp_create() {
    dsp_t* dsp = malloc( sizeof( dsp_t ) );
    memset( dsp, 0, sizeof( dsp_t ) );

    return dsp;
}

void
dsp_destroy( dsp_t* dsp ) {
    if( dsp->workers != NULL ) free( dsp->workers );
    free( dsp );
}

void
dsp_destroyWorker( dsp_worker_t* worker ) {
    if( worker == NULL ) return;
    if( worker->destroy != NULL ) worker->destroy( worker );
    free( worker );
}

void
dsp_addWorker( dsp_t* dsp, dsp_worker_t* worker ) {

    int idx =0;
    if( dsp->workers == NULL ) {
        // There is no array yet, create it
        dsp->workers = malloc( sizeof( dsp_worker_t* ) );
        dsp->workers_size =1;
    } else {
        // Find a free position in the array, or allocate more space
        while( 1 ) {
            if( dsp->workers[idx] == NULL ) break;
            if( dsp->workers_size == idx ) {
                dsp->workers = realloc( dsp->workers, (idx+1) * sizeof( dsp_worker_t* ) );
                dsp->workers_size = idx+1;
                break;
            }
            idx++;
        }
    }

    dsp->workers[idx] = worker;
    dsp->nworkers++;
}

void
dsp_removeWorker( dsp_t* dsp, dsp_worker_t* worker ) {
    if( dsp->workers == NULL || worker == NULL ) return;
    for( int i =0; i < dsp->workers_size; i++ ) {
        if( dsp->workers[i] == worker ) {
            dsp->workers[i] =NULL;
            dsp->nworkers--;
            // We should free up space here eventually... 
        }
    }
}

void
dsp_tick( dsp_t* dsp ) {
    double sample =sampler_getNext( dsp->sampler );
    for( int i =0; i < dsp->workers_size; i++ ) {
        dsp_worker_t *worker =dsp->workers[i];

        worker->in[0] = sample;
        worker->consume( worker );

    }
}
