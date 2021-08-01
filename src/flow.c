#include "flow.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdatomic.h>
#include <threads.h>
#include <math.h>

#include "stage_private.h"
// TODO: Replace with another clock/timer
#include <SDL2/SDL.h>

struct flw {
    flw_stage_t**       stages;
    int                 stages_size;
    int                 nstages; 
    int                 frequency;
    bool                running;
    atomic_bool         stop;
    thrd_t              thread;
};

static inline void
flw_tick( flw_t* flw ) {
/*    double sample =sampler_getNext( flw->sampler );
    for( int i =0; i < flw->stages_size; i++ ) {
        flw_stage_t *stage =flw->stages[i];

        stage->in[0] = sample;
        stage->consume( stage );

    }*/
    for( int i =0; i < flw->stages_size; i++ ) {
        flw_stage_t* stage1 =flw->stages[i];
        // Find stages that are not NULL and have no input ('sources')
        if( stage1 == NULL ) continue;
        if( stage1->inStage != NULL ) continue;

        flw_stage_t* stage2 =stage1;
        do {
            stage2->consume( stage2 );
            stage2 =stage2->outStage;
        } while( stage2 != NULL );
    }
}

static int
flw_main( void* arg ) {
    flw_t* flw =(flw_t*)arg;

    while( !atomic_load( &flw->stop ) ) {
    
        uint64_t time =0UL;
        uint64_t now  =SDL_GetPerformanceCounter();
        double elapsed;
        if( time == 0UL ) {
            elapsed =1.f;
        } else {
            elapsed =(double)(now - time) / (double)SDL_GetPerformanceFrequency();
        }

        time =SDL_GetPerformanceCounter();

        int delta =floorf(flw->frequency * elapsed);

        for( int i=0; i < delta; i++ ) {
            flw_tick( flw );
        }

        thrd_yield();
    }
    return 0;
}

flw_t*
flw_create() {
    flw_t* flw = malloc( sizeof( struct flw ) );
    memset( flw, 0, sizeof( struct flw ) );

    return flw;
}

void
flw_destroy( flw_t* flw ) {
    if( flw == NULL ) return;
    if( flw->stages != NULL ) free( flw->stages );
    free( flw );
}

static bool
flw_containsStage( flw_t* flw, flw_stage_t* stage ) {
    assert( flw != NULL );
    if( flw->stages == NULL ) return false;
    for( int i =0; i < flw->stages_size; i++ ) {
        if( flw->stages[i] == stage ) return true;
    }

    return false;
}

void
flw_addStage( flw_t* flw, flw_stage_t* stage ) {
    assert( flw != NULL );
    if( flw->running ) return;

    int idx =0;
    if( flw->stages == NULL ) {
        // There is no array yet, create it
        flw->stages = malloc( sizeof( flw_stage_t* ) );
        flw->stages_size =1;
    } else {
        // Find a free position in the array, or allocate more space
        while( 1 ) {
            if( flw->stages_size == idx ) {
                flw->stages = realloc( flw->stages, (idx+1) * sizeof( flw_stage_t* ) );
                flw->stages_size = idx+1;
                break;
            } else if( flw->stages[idx] == NULL ) break;
            idx++;
        }
    }

    flw->stages[idx] = stage;
    flw->nstages++;
}

void
flw_removeStage( flw_t* flw, flw_stage_t* stage ) {
    assert( flw != NULL );
    if( flw->running ) return;
    if( flw->stages == NULL || stage == NULL ) return;
    for( int i =0; i < flw->stages_size; i++ ) {
        if( flw->stages[i] == stage ) {
            flw->stages[i] =NULL;
            flw->nstages--;
            // We should free up space here eventually... 
        }
    }
}

bool
flw_connect( flw_t* flw, flw_stage_t* stage1, flw_stage_t* stage2 ) {
    assert( flw != NULL && stage1 != NULL && stage2 != NULL );
    if( flw->running ) return;
    // So much can go wrong...
    if( !flw_containsStage( flw, stage1 ) || !flw_containsStage( flw, stage2 ) ) {
        fprintf( stderr, "flw_connect(): stage(s) not in flow\n" );
        return false;
    }
    if( stage1->outPortSize == 0 ) {
        fprintf( stderr, "flw_connect(): cannot connect sink" );
        return false;
    }
    if( stage2->inPortSize == 0 ) {
        fprintf( stderr, "flw_connect(): cannot connect to source" );
        return false;
    }
    if( stage1->outStage != NULL || stage2->inStage != NULL ) {
        fprintf( stderr, "flw_connect(): specified slot(s) not empty" );
        return false;
    }
    if( stage1->outPortSize != stage2->inPortSize ) {
        fprintf( stderr, "flw_connect(): port sizes do not match" );
        return false;
    }

    // TODO insert cycle check here...
    stage1->outStage = stage2;
    stage2->inStage  = stage1;
    stage1->outq = stage2->inq;

    return true;
}

void
flw_disconnect( flw_t* flw, flw_stage_t* stage1, flw_stage_t* stage2 ) {
    assert( flw != NULL && stage1 != NULL && stage2 != NULL );
    if( flw->running ) return;
    if( !flw_containsStage( flw, stage1 ) || !flw_containsStage( flw, stage2 ) ) {
        fprintf( stderr, "flw_disconnect(): stage(s) not in flow\n" );
        return;
    }
    if( stage1->outStage != stage2 && stage2->inStage != stage1 ) {
        fprintf( stderr, "flw_disconnect(): stage not connected\n" );
        return;
    }
    stage1->outStage = NULL;
    stage2->inStage  = NULL;
    stage1->outq =stage2->inq;
}

void
flw_setFrequency( flw_t* flw, int freq ) {
    assert( flw != NULL );
    if( flw->running ) return;

    flw->frequency =freq;
}

void
flw_start( flw_t* flw ) {
    assert( flw != NULL );
    if( flw->running ) return;

    atomic_store( &flw->stop, false );
    flw->running =true;
    thrd_create( &flw->thread, flw_main, flw );
}

void
flw_stop( flw_t* flw ) {
    assert( flw != NULL );
    if( flw->running ) {
        atomic_store( &flw->stop, true );
        int res;
        thrd_join( flw->thread, &res );
        flw->running =false;
    }
}

void
flw_printGraph( flw_t* flw ) {
    assert( flw != NULL );
    if( flw->stages == NULL ) return;

    for( int i =0; i < flw->stages_size; i++ ) {
        flw_stage_t* stage1 =flw->stages[i];
        // Find stages that are not NULL and have no input ('sources')
        if( stage1 == NULL || stage1->inStage != NULL ) continue;
        printf( "(%s)", stage1->name );

        flw_stage_t* stage2 =stage1->outStage;
        while( stage2 != NULL ) {
            printf( " => (%s)", stage2->name );
            stage2 =stage2->outStage;
        }
        printf( "\n" );
    }
}

