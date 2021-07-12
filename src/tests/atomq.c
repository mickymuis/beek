#include <stdio.h>
#include "../atomq.h"
#include <threads.h>

struct foo {
    atomq_t *buf;
    int o, n;
};

int
add10( void *arg ) {
    struct foo* foo =(struct foo*)arg;

    atomq_t* buf =foo->buf;
    
    for( int i =0; i < foo->n; i++ ) {
        while( !atomq_enqueue( buf, (double)(i + foo->o)) );
    }
}

int
main( int argc, char**argv ) {

    const int N = 1000000;
    atomq_t *buf =atomq_create( 10 );

    int c1 =0, c2 =N;
    thrd_t t1, t2;
    struct foo a1 = {buf, c1, N};
    struct foo a2 = {buf, c2, N};

    thrd_create( &t2, add10, &a2 );
    thrd_create( &t1, add10, &a1 );
    
    while( 1 ) { 
        double d;
        if( atomq_dequeue( buf, &d ) ) {
            int i =(int)d;
            if( i == c1 )
                c1++;
            else if( i == c2 )
                c2++;
            else {
                printf( "ERROR: non-matching number received c1=%d, c2=%d, i=%d\n", c1,c2,i );
                break;
            }

            printf( "c1=%d, c2=%d, i=%d\n", c1,c2,i );
            if( c1 == N && c2 == 2*N ) {
                printf( "No errors.\n" );
                break;
            }
        }

    }

    thrd_join( t1, NULL );
    thrd_join( t2, NULL );
    atomq_destroy( buf );

    return 0;
}

