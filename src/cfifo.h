#ifndef CFIFO_H
#define CFIFO_H

#include <stdlib.h>
#include <stdbool.h>

/** A circular buffer/FIFO that stores elements of the double type */
typedef struct cfifo_t cfifo_t;

/** Creates a circular buffer that can hold at most @capacity doubles */
cfifo_t* 
cfifo_create( size_t capacity );

/** Frees a given circular buffer and all associated mememory */
void
cfifo_destroy( cfifo_t* );

/** Push an item at the end. If the buffer is not full, the length will be increased by one.
    If the buffer is full, this function returns false and the buffer is not modified. */
bool
cfifo_push( cfifo_t*, double );

/** Push an item at the end. If the buffer is not full, the length will be increased by one.
    If the buffer is full, the least recent item is removed first. */
void
cfifo_push2( cfifo_t*, double );

/** Takes the first item and decreases the length by one. Returns 0. if there is no such item */
double
cfifo_take( cfifo_t* );

/** Reads the buffer at position @index from the current first item, but does not modify the buffer.
    The item returned for a certain @index is influenced by calls to cfifo_take() and cfifo_push(). */
double
cfifo_read( cfifo_t*, size_t index );

/** Returns the number of the items current in the buffer. */
size_t
cfifo_length( cfifo_t* );

/** Returns the maximum number of items that can be in the buffer. */
size_t
cfifo_capacity( cfifo_t* );

/** Returns true if the buffer is filled to capacity. In this case, cfifo_push2() will overwrite the first item */
bool
cfifo_full( cfifo_t* );

/** Clears the buffer and sets the length to zero */
void
cfifo_clear( cfifo_t* );

#endif
