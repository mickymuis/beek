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
#include "dsp.h"
#include "dsp/passthrough.h"
#include "dsp/lowpass.h"

dsp_t* DSP;

void fakeThread( scope_t* scope ) {
    static int freq      =500;
    static uint64_t time =0UL;
    uint64_t now         =SDL_GetPerformanceCounter();
    double elapsed;
    if( time == 0UL ) {
        elapsed =1.f;
    } else {
        elapsed =(double)(now - time) / (double)SDL_GetPerformanceFrequency();
    }

    time =SDL_GetPerformanceCounter();

    int delta =ceilf(freq * elapsed);

    for( int i=0; i < delta; i++ ) {
        dsp_tick( DSP );
        scope_pushChannelFifo( scope, 0, DSP->workers[0]->out[0] );
        scope_pushChannelFifo( scope, 1, DSP->workers[1]->out[0] );
    }
}

void
update( window_t* win, SDL_Rect area ) {
    scope_t *s = (scope_t*)win->user;
 //   s->elapsed =.01f;

    fakeThread( s );

   // scope_pushChannelFifo( s, 0, sampler_getNext( DSP->sampler ) );

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

    DSP =dsp_create();
    DSP->sampler =s;
    dsp_worker_t* passthrough =dsp_createPassThrough( 1 );
    dsp_worker_t* lowpass     =dsp_createLowPass( 1 );
    dsp_addWorker( DSP, passthrough );
    dsp_addWorker( DSP, lowpass );
    
    window_t win;
    window_init( &win );
    if( window_create( &win ) != 0 ) {
        fprintf( stderr, "Could not open a new window" );
        return -1;
    }

    scope_t* scope = scope_create( 2, win.render );
    scope_setFrequency( scope, 100 );
    //scope_initChannel( scope, 0, SCOPE_MODE_STATIC );
    //scope_setChannelBuffer( scope, 0, s );
    scope_initChannel( scope, 0, SCOPE_MODE_STREAM );
    scope_setChannelDrawStyle( scope, 0, SCOPE_DRAW_LINES );
    scope_setChannelVerticalMode( scope, 0, SCOPE_VRANGE_AUTO_OPTIMAL );
    scope_initChannel( scope, 1, SCOPE_MODE_STREAM );
    scope_setChannelDrawStyle( scope, 1, SCOPE_DRAW_LINES );
    scope_lockChannelRange( scope, 1, 0 );

    win.on_redraw = (window_renderfunc_t)&update;
    win.user =scope;
    window_mainloop( &win );
    window_destroy( &win );
    scope_destroy( scope );
    sampler_destroy( s );
    dsp_destroy( DSP );

    return 0;
}
