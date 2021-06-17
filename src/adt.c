#include "adt.h"

#include <string.h>
#include <assert.h>

/*
 *  Some random test code
 *
    cfifo_t *buf =cfifo_create( 10 );
    for( int i =1; i <= 10; i++ ) {
        cfifo_push( buf, (double)i );
    }
    for( int i =0; i < 15; i++ ) {
        double d =cfifo_read( buf, i );
        printf( "%lf ", d );
    }
    printf( "\n length = %ld\n", cfifo_length( buf ) );
    for( int i =0; i < 5; i++ ) {
        double d =cfifo_take( buf );
        printf( "%lf ", d );
    }
    printf( "\n length = %ld\n", cfifo_length( buf ) );
    for( int i =1; i <= 5; i++ ) {
        cfifo_push( buf, (double)i );
    }
    printf( "\n length = %ld\n", cfifo_length( buf ) );
    for( int i =0; i < 15; i++ ) {
        double d =cfifo_take( buf );
        printf( "%lf ", d );
    }
    cfifo_destroy( buf );*/

struct cfifo_t {
    double* data;
    size_t  capacity;
    size_t  head;
    size_t  tail;
};

cfifo_t* 
cfifo_create( size_t capacity ) {
    cfifo_t *buf =malloc( sizeof(cfifo_t) );
    memset( buf, 0, sizeof(cfifo_t) );
    // Reserve one extra element to mark the 'buffer full' condition
    buf->data =malloc( (capacity+1) * sizeof(double) );
    memset( buf->data, 0, (capacity+1) * sizeof(double) );
    buf->capacity =capacity;

    return buf;
}

void
cfifo_destroy( cfifo_t* buf ) {
    free( buf );
}

void
advance_ptr( size_t *ptr, size_t capacity ) {
    if( ++*ptr == capacity+1 )
        *ptr =0;
}

void
cfifo_push( cfifo_t* buf, double d ) {
    assert( buf != NULL );

    if( cfifo_full( buf ) )
        advance_ptr( &buf->tail, buf->capacity );       
    
    buf->data[buf->head] =d;
    advance_ptr( &buf->head, buf->capacity );
}

double
cfifo_take( cfifo_t* buf ) {
    assert( buf != NULL );

    double d =0.;
    if( cfifo_length( buf ) > 0 ) {
        d =buf->data[buf->tail];
        advance_ptr( &buf->tail, buf->capacity );
    }
    return d;
}

double
cfifo_read( cfifo_t* buf, size_t index ) {
    assert( buf != NULL );
    
    if( index >= cfifo_length( buf ) ) return 0.;
    size_t ptr =index + buf->tail;
    if( ptr > buf->capacity ) // data is one bigger than capacity!
        ptr -= buf->capacity+1;
    return buf->data[ptr];
}

size_t
cfifo_length( cfifo_t* buf ) {
    assert( buf != NULL );
    if( buf->tail <=  buf->head ) return buf->head - buf->tail;
  /*if( buf->tail > buf->head )*/ return buf->capacity - (buf->tail - buf->head - 1);
}

size_t
cfifo_capacity( cfifo_t* buf ) {
    assert( buf != NULL );
    return buf->capacity;
}

bool
cfifo_full( cfifo_t* buf ) {
    assert( buf != NULL );
    return cfifo_length( buf ) == buf->capacity;
}

void
cfifo_clear( cfifo_t* buf ) {
    assert( buf != NULL );
    buf->head = buf->tail =0;
    buf->data[0] =0.;

}
