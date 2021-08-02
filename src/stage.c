#include "stage.h"
#include "stage_private.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

void
flw_destroyStage( flw_stage_t* stage ) {
    if( stage == NULL ) return;
    if( stage->destroy != NULL ) stage->destroy( stage );
    free( stage );
}

void
flw_initStage( flw_stage_t* st ) {
    assert( st != NULL );

    memset( st, 0, sizeof(flw_stage_t) );
    atomic_flag_test_and_set( &st->syncIdle );
    atomic_init( &st->stop, false ); 
}
