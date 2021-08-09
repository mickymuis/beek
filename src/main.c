#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <threads.h>

#include "window.h"
#include "scope.h"
#include "sampler.h"
#include "cfifo.h"
#include "atomq.h"
#include "flow.h"
#include "gaussian.h"
#include "stages/passthrough.h"
#include "stages/lowpass.h"
#include "stages/source.h"
#include "stages/sink.h"
#include "stages/scalespace.h"

flw_t* FLOW;

void fakeThread( scope_t* scope ) {
    static int freq      =100;
    static uint64_t time =0UL;
    uint64_t now         =SDL_GetPerformanceCounter();
    double elapsed;
    if( time == 0UL ) {
        elapsed =1.f;
    } else {
        elapsed =(double)(now - time) / (double)SDL_GetPerformanceFrequency();
    }

    time =SDL_GetPerformanceCounter();

    int delta =floorf(freq * elapsed);

    for( int i=0; i < delta; i++ ) {
/*        flw_tick( FLOW );
        scope_pushChannelFifo( scope, 0, FLOW->stages[0]->out[0] );
        scope_pushChannelFifo( scope, 1, FLOW->stages[1]->out[0] );
        scope_pushChannelFifo( scope, 1, FLOW->stages[1]->out[0] -10. );
        scope_pushChannelFifo( scope, 1, FLOW->stages[1]->out[0] -20. );
        scope_pushChannelFifo( scope, 1, FLOW->stages[1]->out[0] -30. );
        scope_pushChannelFifo( scope, 1, FLOW->stages[1]->out[0] -40. );
        scope_pushChannelFifo( scope, 1, FLOW->stages[1]->out[0] -50. );
        scope_pushChannelFifo( scope, 1, FLOW->stages[1]->out[0] -60. );
        scope_pushChannelFifo( scope, 1, FLOW->stages[1]->out[0] -70. );*/
    }
}

void
update( window_t* win, SDL_Rect area ) {
    scope_t *s = (scope_t*)win->user;
 //   s->elapsed =.01f;

    fakeThread( s );

   // scope_pushChannelFifo( s, 0, sampler_getNext( FLOW->sampler ) );

   // scope_pushChannelFifo( s, 1, (double)rand() / (double)RAND_MAX - .5);

    scope_updateDrawingArea( s, area );
    scope_update( s );
};

int
main( int argc, char**argv ) {

    if( argc < 2 ) return 0;
    sampler_t *s;
    FILE* f =fopen( argv[1], "r" );
    if( !f ) {
        perror( "main()" );
        return -1;
    }
    if( (s = sampler_fromFile( f )) == NULL ) return -1;
    fclose( f );

    /*for( size_t i=0; i < s.n; i++ ){
        printf( "%lf\n", s.data[i] );
    }

    free( s.data);*/

    sampler_setCyclic( s, true );

    
    window_t win;
    window_init( &win );
    if( window_create( &win ) != 0 ) {
        fprintf( stderr, "Could not open a new window" );
        return -1;
    }

    scope_t* scope = scope_create( 6, win.render );
    scope_setFrequency( scope, 200 );
    /*scope_initChannel( scope, 0, SCOPE_MODE_STATIC );
    scope_setChannelBuffer( scope, 0, s );
    scope_initChannel( scope, 0, SCOPE_MODE_STREAM );
    scope_setChannelDrawStyle( scope, 0, SCOPE_DRAW_LINES );
    scope_setChannelVerticalMode( scope, 0, SCOPE_VRANGE_AUTO_OPTIMAL );
    scope_setChannelSampleSize( scope, 0, 4 );
    cope_initChannel( scope, 1, SCOPE_MODE_STREAM );
    scope_setChannelDrawStyle( scope, 1, SCOPE_DRAW_LINES );
    scope_setChannelVerticalMode( scope, 1, SCOPE_VRANGE_AUTO_OPTIMAL );
    scope_lockChannelRange( scope, 1, 0 );
    scope_setChannelSampleSize( scope, 1, 8 );*/
    for( int i =0; i < 6; i++ ) {
        scope_initChannel( scope, i, SCOPE_MODE_STREAM );
        scope_setChannelDrawStyle( scope, i, SCOPE_DRAW_LINES );
//        scope_setChannelVerticalMode( scope, i SCOPE_VRANGE_AUTO_OPTIMAL );
        scope_setChannelVerticalRange( scope, i, -1, 1 );
    }
    
    FLOW =flw_create();
    flw_setFrequency( FLOW, 100 );
    //FLOW->sampler =s;
    flw_stage_t* source      =flw_createSampleSource( s, 1 );
    //flw_stage_t* passthrough =flw_createPassThrough( 1 );
    //flw_stage_t* lowpass     =flw_createLowPass( 1 );
    flw_stage_t* sst         =flw_createScaleSpaceTransform( 6 );
    flw_stage_t* sink        =flw_createScopeSink( scope, 0, 6 );
    flw_addStage( FLOW, source );
    //flw_addStage( FLOW, passthrough );
    //flw_addStage( FLOW, lowpass );
    flw_addStage( FLOW, sst );
    flw_addStage( FLOW, sink );
    //flw_connect( FLOW, source, passthrough );
    //flw_connect( FLOW, passthrough, lowpass );
    //flw_connect( FLOW, lowpass, sink );
    flw_connect( FLOW, source, sst );
    flw_connect( FLOW, sst, sink );
    //flw_connect( FLOW, source, sink );

    flw_printGraph( FLOW );

    win.on_redraw = (window_renderfunc_t)&update;
    win.user =scope;

    flw_start( FLOW );
    window_mainloop( &win );
    flw_stop( FLOW );

    // Cleanup routine
    window_destroy( &win );
    scope_destroy( scope );
    sampler_destroy( s );
    flw_destroy( FLOW );

    return 0;
}
