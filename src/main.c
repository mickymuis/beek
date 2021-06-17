#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#include "window.h"
#include "scope.h"
#include "sampler.h"
#include "adt.h"

void
update( window_t* win, SDL_Rect area ) {
    scope_t *s = (scope_t*)win->user;
 //   s->elapsed =.01f;

    scope_pushChannelFifo( s, 1, (double)rand() / (double)RAND_MAX - .5);

    scope_update( s, win->render, area );
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

    scope_t* scope = scope_create( 2 );
    scope_setSpeed( scope, 500 );
    scope_initChannel( scope, 0, SCOPE_CHANNEL_STATIC );
    scope_setChannelBuffer( scope, 0, s );
    scope_setChannelDrawStyle( scope, 0, SCOPE_CHANNEL_LINES );
    scope_initChannel( scope, 1, SCOPE_CHANNEL_STREAM );

    window_t win;
    window_init( &win );
    win.on_redraw = (window_renderfunc_t)&update;
    win.user =scope;

    if( window_create( &win ) != 0 ) {
        fprintf( stderr, "Could not open a new window" );
        return -1;
    }
    window_mainloop( &win );
    window_destroy( &win );
    scope_destroy( scope );
    sampler_destroy( s );

    return 0;
}
