#include "atomq.h"

#include <string.h>
#include <assert.h>
#include <stdatomic.h>
#include <threads.h>

struct atomq_t {
    double* data;
    uint32_t  capacity;
    /* Convention: push write at the head position, take removes from the tail 
       There are two separate head pointers to protect the reading operation from unfinished writing operations */
    _Atomic uint64_t read_head;
    _Atomic uint64_t write_head;
    _Atomic uint64_t tail;
};

atomq_t* 
atomq_create( uint32_t capacity ) {
    atomq_t *q =malloc( sizeof(atomq_t) );
    // Reserve one extra element to mark the 'buffer full' condition
    q->data =malloc( (capacity+1) * sizeof(double) );
    memset( q->data, 0, (capacity+1) * sizeof(double) );
    q->capacity =capacity;

    q->read_head  = ATOMIC_VAR_INIT( 0UL );
    q->write_head = ATOMIC_VAR_INIT( 0UL );
    q->tail       = ATOMIC_VAR_INIT( 0UL );

    return q;
}

void
atomq_destroy( atomq_t* q ) {
    free( q );
}

/* Convert a uint64_t pointer to an index within the queue's buffer */
static inline uint64_t
wrap_ptr( uint64_t ptr, uint32_t capacity ) {
    return (ptr) % (capacity+1);
}

bool
atomq_enqueue( atomq_t* q, double d ) {
    assert( q != NULL );

    // Obtain the current state of the queue by loading head & tail pointers
    uint64_t orig_head =atomic_load( &q->write_head ), tail;
    do {
        tail =atomic_load( &q->tail );

        // Fail if the buffer is full
        if( wrap_ptr( tail, q->capacity ) == wrap_ptr( orig_head+1, q->capacity ) )
            return false;

        // Try to increment the current head pointer using compare & exchange
        // If this fails, the obtain a new (current) head pointer and try again
    } while( !atomic_compare_exchange_weak( &q->write_head, &orig_head, orig_head+1 ) );

    // At this point we have a head pointer that is unique to this thread/call
    // Since we did not update read_head yet, this change is not yet visible to dequeue()
    // (we can take our time to write the data)
    q->data[wrap_ptr( orig_head, q->capacity )] =d;

    uint64_t head  =orig_head;

    // Make the operation visible to dequeue()
    // We try to update read_head with *the same* value over and over.
    // This ensures that read_head is updated in the same order as write_head was incremented.
    while( !atomic_compare_exchange_weak( &q->read_head, &head, head+1 ) ) {
        thrd_yield();
        head =orig_head;
    }
    return true;
}

bool
atomq_dequeue( atomq_t* q, double* d ) {
    assert( q != NULL );

    // Obtain the current state of the queue
    uint64_t tail=atomic_load( &q->tail ), head;
    do {
        head     =atomic_load( &q->read_head );

        // Fail if empty
        if( tail == head )
            return false;

        // Read the data in advance, if the compare & exchange succees we will have the correct data.
        // (otherwise, the entire queue may wrap-around in the meanwhile, although chances of that are slim).
        *d =q->data[wrap_ptr( tail, q->capacity )];
    } while( !atomic_compare_exchange_weak( &q->tail, &tail, tail+1 ) );

    return true;
}

uint64_t
atomq_estimateLength( atomq_t* q ) {
    assert( q != NULL );
    uint64_t head = wrap_ptr( atomic_load( &q->read_head ), q->capacity ),
             tail = wrap_ptr( atomic_load( &q->tail ), q->capacity );
    if( tail <=  head ) return head - tail;
  /*if( tail > head )*/ return q->capacity - (tail - head - 1);
}

uint32_t
atomq_capacity( atomq_t* q ) {
    assert( q != NULL );
    return q->capacity;
}

bool
atomq_wasFull( atomq_t* q ) {
    assert( q != NULL );
    uint64_t head = atomic_load( &q->write_head ),
             tail = atomic_load( &q->tail );
    return wrap_ptr( head+1, q->capacity ) == wrap_ptr( tail, q->capacity );
}

bool
atomq_wasEmpty( atomq_t* q ) {
    assert( q != NULL );
    uint64_t head = atomic_load( &q->write_head ),
             tail = atomic_load( &q->tail );
    return head == tail;
}

