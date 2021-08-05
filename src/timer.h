#ifndef TIMER_H
#define TIMER_H

#include <inttypes.h>
#include <stdbool.h>
#include <threads.h>

/** Threaded timer that can provide a reference clock to other threads.
 *
 * tmr_t uses conditional variables to wake up a thread once the interval has elapsed.
 * This provides an alternative for busy wait/spin locks when synchronizing threads to a clock.
 *
 * Example usage:
 *
 * mtx_t mutex;
 * mtx_init( &mutex, mtx_plain );
 * tmr_t* timer =tmr_create();
 * tmr_setFrequency( 10.f );
 * tmr_start( timer );
 * ...
 * while(1) {
 *      mtx_lock( &mutex );
 *      tmr_wait( timer, &mutex );
 *      // One tick has elapsed
 *      // Do work
 * }
 */

typedef struct tmr tmr_t;
typedef uint64_t ttick_t;

/** Allocates a timer object */
tmr_t*
tmr_create();

/** Frees all resources asociated with a timer object. 
    If the timer is still running, it will call tmr_stop() and wait until it is terminated first.*/
void
tmr_destroy( tmr_t* );

/** Sets the frequency to @freq / second.
    The new value only takes effect after calling tmr_start() */
void
tmr_setFrequency( tmr_t*, float freq );

/** Sleeps the current thread until the timer interval has elapsed.
    @mutex must be initialized and locked by the current thread.
    If the timer is not running, this function returns immediately. 
*/
void
tmr_wait( tmr_t*, mtx_t* mutex );

/** Returns the number of ticks elapsed since tmr_start() was called */
ttick_t
tmr_ticksElapsed( tmr_t* );

/** Creates a new thread for the timer and returns immediately.
    Nothing happens if the timer is already running. */
bool
tmr_start( tmr_t* );

/** Waits for the timer thread to join, which takes at most one interval.
    If the timer is not running it returns immediately.
 */
void
tmr_stop( tmr_t* );

#endif
