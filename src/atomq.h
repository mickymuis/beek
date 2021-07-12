#ifndef ATOMQ_H
#define ATOMQ_H

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

/** 'Atomic' Queue for double floating point elements. 
    This datastructure can be used with a multiple producer, multiple consumer concurrent access pattern.

atomq_t:
    - Uses a contiguous memory circular buffer.
    - Is lock-free on platforms where the C11 atomics are lock-free
    - Uses C11 atomics and 64-bit 'compare & exchange' to support concurrent access.
*/
typedef struct atomq_t atomq_t;

/** Creates a queue that can hold at most @capacity doubles.
    This function is reentrant.*/
atomq_t* 
atomq_create( uint32_t capacity );

/** Frees a given circular buffer and all associated mememory. */
void
atomq_destroy( atomq_t* );

/** Push a value at the end of the queue. The length will be increased by one.
    If the buffer is full, this function returns false and the buffer is not modified. 
    
    Reentrant. It is safe to call this function on the same object from different thread simultaneously. */
bool
atomq_enqueue( atomq_t*, double );

/** Takes the first item and decreases the length by one. Returns false if there is no such item. 
 
    Reentrant. It is safe to call this function on the same object from different thread simultaneously. */
bool
atomq_dequeue( atomq_t*, double* );

/** Returns the number of the items that have been completely written to the queue, at the time of the call.
    This figure does not include items that are currently being written (if any). */
size_t
atomq_estimateLength( atomq_t* );

/** Returns the maximum number of items that can be in the buffer. */
uint32_t
atomq_capacity( atomq_t* );

/** Returns true if the buffer was filled to capacity at the time of the call. 
    Due to the concurrent nature of this data structure, 
    the return value of this function can be invalid. */
bool
atomq_wasFull( atomq_t* );

/** Returns true if the buffer was empty at the time of the call. */
bool
atomq_wasEmpty( atomq_t* );

#endif

